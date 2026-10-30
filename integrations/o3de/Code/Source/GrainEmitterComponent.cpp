#include <GrainSynthesizer/GrainEmitterComponent.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <MiniAudio/MiniAudioBus.h>
#include <CloudDataSource.h>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <vector>

namespace GrainSynthesizer
{
    struct GrainEmitterComponent::Runtime
    {
        std::vector<std::vector<float>> pcm;
        CloudDataSource source;
        ma_sound sound{};
        bool sourceReady = false;
        bool soundReady = false;
        ~Runtime()
        {
            // Detaching waits for in-flight graph processing before PCM release.
            if (soundReady) ma_sound_uninit(&sound);
            if (sourceReady) ma_data_source_uninit(&source.base);
        }
    };
    GrainEmitterComponent::GrainEmitterComponent() = default;
    GrainEmitterComponent::GrainEmitterComponent(const GrainEmitterConfig& config) : m_config(config) {}
    GrainEmitterComponent::~GrainEmitterComponent() = default;

    void GrainEmitterComponent::Reflect(AZ::ReflectContext* context)
    {
        GrainEmitterConfig::Reflect(context);
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
            sc->Class<GrainEmitterComponent, AZ::Component>()->Version(1)
                ->Field("Configuration", &GrainEmitterComponent::m_config);
        if (auto* bc = azrtti_cast<AZ::BehaviorContext*>(context))
            bc->EBus<GrainEmitterRequestBus>("GrainEmitterRequestBus")
                ->Event("Play", &GrainEmitterRequests::Play)
                ->Event("Stop", &GrainEmitterRequests::Stop)
                ->Event("SetParameter", &GrainEmitterRequests::SetParameter);
    }
    void GrainEmitterComponent::Activate()
    {
        GrainEmitterRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TickBus::Handler::BusConnect();
        m_playRequested = m_config.autoPlay;
        for (auto& asset : m_config.sources) if (asset.GetId().IsValid()) asset.QueueLoad();
    }
    void GrainEmitterComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        GrainEmitterRequestBus::Handler::BusDisconnect();
        m_playRequested = false;
        m_runtime.reset();
    }
    bool GrainEmitterComponent::Play()
    {
        if (m_runtime) return m_runtime->source.cloud.Start();
        m_playRequested = true;
        for (auto& asset : m_config.sources) if (asset.GetId().IsValid()) asset.QueueLoad();
        return true; // Accepted; asset loading/decoding can still fail with a log.
    }
    bool GrainEmitterComponent::Stop()
    {
        m_playRequested = false;
        return !m_runtime || m_runtime->source.cloud.Stop();
    }
    bool GrainEmitterComponent::SetParameter(AZ::u32 parameter, float value, float seconds, AZ::u32 index)
    {
        if (!m_runtime || parameter > 13) return false;
        return m_runtime->source.cloud.SetParameter(static_cast<wz::audio::GrainParam>(parameter), value, seconds, index);
    }
    void GrainEmitterComponent::OnTick(float, AZ::ScriptTimePoint)
    {
        if (m_playRequested && !m_runtime)
        {
            for (const auto& asset : m_config.sources)
            {
                if (!asset.GetId().IsValid() || asset.IsError())
                {
                    AZ_Error("GrainSynthesizer", false, "A grain source asset is invalid or failed to load");
                    m_playRequested = false;
                    return;
                }
                if (!asset.IsReady()) return;
            }
            m_playRequested = false;
            if (!CreateRuntime()) return;
            m_runtime->source.cloud.Start();
        }
        if (m_runtime && m_config.spatial)
        {
            auto tm = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(tm, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            const auto p = tm.GetTranslation();
            ma_sound_set_position(&m_runtime->sound, p.GetX(), p.GetY(), p.GetZ());
        }
    }
    bool GrainEmitterComponent::CreateRuntime()
    {
        auto* api = MiniAudio::MiniAudioInterface::Get();
        auto* engine = api ? api->GetSoundEngine() : nullptr;
        if (!engine || m_config.sources.empty() || m_config.sources.size() > 8)
        {
            AZ_Error("GrainSynthesizer", false, "MiniAudio engine and one to eight sources are required");
            return false;
        }
        if (!std::isfinite(m_config.minDistance) || !std::isfinite(m_config.maxDistance)
            || m_config.minDistance <= 0 || m_config.maxDistance < m_config.minDistance)
        {
            AZ_Error("GrainSynthesizer", false, "Invalid attenuation distances");
            return false;
        }
        try
        {
            auto runtime = AZStd::make_unique<Runtime>();
            runtime->pcm.resize(m_config.sources.size());
            wz::audio::GrainCloudDesc desc;
            desc.source_count = static_cast<uint32_t>(m_config.sources.size());
            const auto rate = ma_engine_get_sample_rate(engine);
            size_t totalSamples = 0;
            for (size_t i = 0; i < m_config.sources.size(); ++i)
            {
                const auto& encoded = m_config.sources[i]->m_data;
                ma_decoder decoder{};
                auto config = ma_decoder_config_init(ma_format_f32, 1, rate);
                if (ma_decoder_init_memory(encoded.data(), encoded.size(), &config, &decoder) != MA_SUCCESS)
                    throw std::runtime_error("Cannot decode grain clip (use WAV, FLAC or MP3)");
                struct DecoderGuard { ma_decoder* value; ~DecoderGuard() { ma_decoder_uninit(value); } } guard{&decoder};
                ma_uint64 length = 0;
                constexpr size_t sampleBudget = 64 * 1024 * 1024; // 256 MiB per emitter.
                if (ma_decoder_get_length_in_pcm_frames(&decoder, &length) != MA_SUCCESS
                    || !length || length > sampleBudget - totalSamples)
                    throw std::runtime_error("Invalid clip length or emitter PCM budget exceeded");
                auto& pcm = runtime->pcm[i];
                pcm.resize(static_cast<size_t>(length));
                ma_uint64 read = 0;
                const auto result = ma_decoder_read_pcm_frames(&decoder, pcm.data(), length, &read);
                if ((result != MA_SUCCESS && result != MA_AT_END) || !read)
                    throw std::runtime_error("Failed to decode grain clip");
                pcm.resize(static_cast<size_t>(read));
                totalSamples += pcm.size();
                desc.sources[i] = {pcm.data(), read, rate, 1};
                desc.weights[i] = i < m_config.weights.size() ? m_config.weights[i] : 1.0f;
            }
            const auto& s = m_config.settings;
            desc.gain = s.gain; desc.density = s.density; desc.position = s.position;
            desc.pitch = s.pitch; desc.grain_ms = s.grainMs; desc.position_jitter = s.positionJitter;
            desc.pitch_jitter_semitones = s.pitchJitter; desc.pan_center = s.panCenter;
            desc.pan_spread = s.panSpread; desc.window_param = s.windowParam;
            desc.blend_rate = s.blendRate; desc.blend_depth = s.blendDepth;
            desc.normalize = s.normalize; desc.max_grains = s.maxGrains; desc.seed = s.seed;
            if (runtime->source.Initialize(desc, rate, m_config.spatial) != MA_SUCCESS)
                throw std::runtime_error("Failed to initialize grain source");
            runtime->sourceReady = true;
            const ma_uint32 flags = m_config.spatial ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;
            if (ma_sound_init_from_data_source(engine, &runtime->source, flags, nullptr, &runtime->sound) != MA_SUCCESS)
                throw std::runtime_error("Failed to attach grain sound");
            runtime->soundReady = true;
            ma_sound_set_min_distance(&runtime->sound, m_config.minDistance);
            ma_sound_set_max_distance(&runtime->sound, m_config.maxDistance);
            if (ma_sound_start(&runtime->sound) != MA_SUCCESS)
                throw std::runtime_error("Failed to start grain sound");
            m_runtime = AZStd::move(runtime);
            return true;
        }
        catch (const std::exception& error)
        {
            AZ_Error("GrainSynthesizer", false, "%s", error.what());
            return false;
        }
    }
}
