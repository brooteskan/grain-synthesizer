#include <GrainSynthesizer/RealtimeCloud.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace GrainSynthesizer
{
    void RealtimeCloud::Configure(const wz::audio::GrainCloudDesc& desc)
    {
        // Bound work and reject invalid authored numbers before the callback.
        const float values[] = {desc.gain, desc.density, desc.position, desc.pitch,
            desc.position_jitter, desc.pitch_jitter_semitones, desc.pan_center,
            desc.pan_spread, desc.grain_ms, desc.window_param, desc.blend_rate,
            desc.blend_depth, desc.normalize};
        for (float value : values)
            if (!std::isfinite(value)) throw std::invalid_argument("Non-finite grain parameter");
        if (desc.max_grains == 0 || desc.max_grains > 256 || desc.source_count > 8
            || desc.grain_ms < 1 || desc.grain_ms > 10000 || desc.density < 0
            || desc.density > 10000 || desc.pitch < 0 || desc.pitch > 16
            || desc.pitch_jitter_semitones < 0 || desc.pitch_jitter_semitones > 48
            || desc.window_param < 0.001f || desc.window_param > 1)
            throw std::invalid_argument("Grain configuration exceeds supported bounds");
        for (uint32_t i = 0; i < desc.source_count; ++i)
            if (!desc.sources[i].valid() || !std::isfinite(desc.weights[i]) || desc.weights[i] < 0)
                throw std::invalid_argument("Invalid source or source weight");
        m_cloud.configure({desc.max_grains, desc.seed});
        m_cloud.reset(desc);
        m_write.store(0, std::memory_order_relaxed);
        m_read.store(0, std::memory_order_relaxed);
    }

    bool RealtimeCloud::Post(const Command& command) noexcept
    {
        const size_t write = m_write.load(std::memory_order_relaxed);
        const size_t next = (write + 1) % QueueSize;
        if (next == m_read.load(std::memory_order_acquire)) return false;
        m_commands[write] = command;
        m_write.store(next, std::memory_order_release);
        return true;
    }
    bool RealtimeCloud::Start() noexcept { return Post({1}); }
    bool RealtimeCloud::Stop() noexcept { return Post({2}); }
    bool RealtimeCloud::SetParameter(wz::audio::GrainParam param, float value,
        float rampSeconds, uint32_t sourceIndex) noexcept
    {
        if (!std::isfinite(value) || !std::isfinite(rampSeconds) || rampSeconds < 0
            || rampSeconds > 60 || static_cast<unsigned>(param) > 13 || sourceIndex >= 8)
            return false;
        using P = wz::audio::GrainParam;
        if ((param == P::Density && (value < 0 || value > 10000))
            || (param == P::Pitch && (value < 0 || value > 16))
            || (param == P::PitchJitter && (value < 0 || value > 48))
            || (param == P::GrainMs && (value < 1 || value > 10000))
            || (param == P::WindowParam && (value < 0.001f || value > 1))
            || (param == P::Window && value != 0)) return false;
        return Post({0, param, value, rampSeconds, sourceIndex});
    }
    void RealtimeCloud::Apply(const Command& c, uint32_t rate) noexcept
    {
        if (c.kind == 1) { m_cloud.start(); return; }
        if (c.kind == 2) { m_cloud.stop(); return; }
        const auto ramp = static_cast<uint32_t>(std::min<double>(c.rampSeconds * double(rate), UINT32_MAX));
        using P = wz::audio::GrainParam;
        switch (c.param)
        {
        case P::Gain: m_cloud.set_gain(c.value, ramp); break;
        case P::Density: m_cloud.set_density(c.value, ramp); break;
        case P::Position: m_cloud.set_position(c.value, ramp); break;
        case P::Pitch: m_cloud.set_pitch(c.value, ramp); break;
        case P::BlendRate: m_cloud.set_blend_rate(c.value, ramp); break;
        case P::BlendDepth: m_cloud.set_blend_depth(c.value, ramp); break;
        case P::SourceWeight: m_cloud.set_source_weight(c.sourceIndex, c.value, ramp); break;
        case P::PositionJitter: m_cloud.set_position_jitter(c.value, ramp); break;
        case P::PitchJitter: m_cloud.set_pitch_jitter(c.value, ramp); break;
        case P::PanCenter: m_cloud.set_pan_center(c.value, ramp); break;
        case P::PanSpread: m_cloud.set_pan_spread(c.value, ramp); break;
        case P::GrainMs: m_cloud.set_grain_size(c.value, ramp); break;
        case P::WindowParam: m_cloud.set_window_param(c.value, ramp); break;
        case P::Window: m_cloud.set_window_shape(wz::audio::GrainWindow::Gaussian); break;
        }
    }
    void RealtimeCloud::Render(float* output, uint32_t frames, uint32_t channels, uint32_t sampleRate) noexcept
    {
        if (!output || !frames || channels == 0 || channels > 2 || sampleRate == 0) return;
        std::fill_n(output, size_t(frames) * channels, 0.0f);
        // Snapshot the producer index so producer traffic cannot prolong a block.
        const size_t end = m_write.load(std::memory_order_acquire);
        size_t read = m_read.load(std::memory_order_relaxed);
        while (read != end)
        {
            Apply(m_commands[read], sampleRate);
            read = (read + 1) % QueueSize;
        }
        m_read.store(read, std::memory_order_release);
        m_cloud.render_add(output, frames, channels, sampleRate);
    }
}
