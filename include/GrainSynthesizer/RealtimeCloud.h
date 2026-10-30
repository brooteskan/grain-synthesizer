#pragma once

#include <audio/grain_cloud.h>
#include <array>
#include <atomic>

namespace GrainSynthesizer
{
    // One control-thread producer, one audio-thread consumer. PCM borrowed by
    // Configure must survive until the callback is detached. Never configure
    // or destroy while rendering. All control methods report queue overflow.
    class RealtimeCloud
    {
    public:
        void Configure(const wz::audio::GrainCloudDesc& desc);
        bool Start() noexcept;
        bool Stop() noexcept;
        bool SetParameter(wz::audio::GrainParam param, float value,
            float rampSeconds = 0.05f, uint32_t sourceIndex = 0) noexcept;
        void Render(float* output, uint32_t frames, uint32_t channels, uint32_t sampleRate) noexcept;

    private:
        struct Command
        {
            uint8_t kind = 0;
            wz::audio::GrainParam param{};
            float value = 0;
            float rampSeconds = 0;
            uint32_t sourceIndex = 0;
        };
        bool Post(const Command& command) noexcept;
        void Apply(const Command& command, uint32_t sampleRate) noexcept;
        static constexpr size_t QueueSize = 128;
        std::array<Command, QueueSize> m_commands{};
        std::atomic<size_t> m_write{0};
        std::atomic<size_t> m_read{0};
        wz::audio::GrainCloud m_cloud;
    };
}
