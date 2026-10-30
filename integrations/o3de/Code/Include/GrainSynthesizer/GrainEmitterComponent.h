#pragma once
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <MiniAudio/SoundAsset.h>
#include <GrainSynthesizer/GrainEmitterBus.h>

namespace GrainSynthesizer
{
    struct GrainSettings
    {
        AZ_TYPE_INFO(GrainSettings, "{80341F85-24AF-4E76-AB5E-6A632D33A269}");
        static void Reflect(AZ::ReflectContext* context);
        float gain = 0.25f;
        float density = 24;
        float position = 0.5f;
        float pitch = 1;
        float grainMs = 100;
        float positionJitter = 0.4f;
        float pitchJitter = 0;
        float panCenter = 0;
        float panSpread = 0.7f;
        float windowParam = 0.4f;
        float blendRate = 0;
        float blendDepth = 0;
        float normalize = 2;
        AZ::u32 maxGrains = 32;
        AZ::u32 seed = 1;
    };

    struct GrainEmitterConfig
    {
        AZ_TYPE_INFO(GrainEmitterConfig, "{9AF02FF3-57D0-4BCE-AC31-D12974CE1D76}");
        static void Reflect(AZ::ReflectContext* context);
        AZStd::vector<AZ::Data::Asset<MiniAudio::SoundAsset>> sources;
        AZStd::vector<float> weights;
        GrainSettings settings;
        bool autoPlay = true;
        bool spatial = false;
        float minDistance = 1;
        float maxDistance = 100;
    };

    class GrainEmitterComponent final
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private GrainEmitterRequestBus::Handler
    {
    public:
        AZ_COMPONENT(GrainEmitterComponent, "{CBF1F393-6E16-4EEA-A7DB-31AB1E2DB55E}");
        GrainEmitterComponent();
        explicit GrainEmitterComponent(const GrainEmitterConfig& config);
        ~GrainEmitterComponent() override;
        static void Reflect(AZ::ReflectContext* context);
        void Activate() override;
        void Deactivate() override;
        bool Play() override;
        bool Stop() override;
        bool SetParameter(AZ::u32 parameter, float value, float rampSeconds, AZ::u32 sourceIndex) override;
    private:
        void OnTick(float, AZ::ScriptTimePoint) override;
        bool CreateRuntime();
        struct Runtime;
        AZStd::unique_ptr<Runtime> m_runtime;
        GrainEmitterConfig m_config;
        bool m_playRequested = false;
    };
}
