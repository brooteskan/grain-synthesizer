#pragma once
#include <GrainSynthesizer/RealtimeCloud.h>
#include <miniaudio.h>
#include <algorithm>
#include <limits>

namespace GrainSynthesizer
{
    // Initialize and uninitialize off the callback; detach the owning ma_sound
    // before destroying this object or its source PCM. No second audio device.
    struct CloudDataSource
    {
        ma_data_source_base base{};
        RealtimeCloud cloud;
        ma_uint32 channels = 2;
        ma_uint32 sampleRate = 48000;
        ma_uint64 cursor = 0;

        static ma_result Read(ma_data_source* source, void* output, ma_uint64 frames, ma_uint64* read)
        {
            auto& self = *static_cast<CloudDataSource*>(source);
            if (read) *read = 0;
            if (!output || frames > std::numeric_limits<uint32_t>::max()) return MA_INVALID_ARGS;
            self.cloud.Render(static_cast<float*>(output), static_cast<uint32_t>(frames), self.channels, self.sampleRate);
            self.cursor += frames;
            if (read) *read = frames;
            return MA_SUCCESS;
        }
        static ma_result Format(ma_data_source* source, ma_format* format, ma_uint32* channels,
            ma_uint32* rate, ma_channel* map, size_t mapCapacity)
        {
            auto& self = *static_cast<CloudDataSource*>(source);
            if (format) *format = ma_format_f32;
            if (channels) *channels = self.channels;
            if (rate) *rate = self.sampleRate;
            if (map) ma_channel_map_init_standard(ma_standard_channel_map_default, map, mapCapacity, self.channels);
            return MA_SUCCESS;
        }
        static ma_result Cursor(ma_data_source* source, ma_uint64* cursor)
        {
            if (!cursor) return MA_INVALID_ARGS;
            *cursor = static_cast<CloudDataSource*>(source)->cursor;
            return MA_SUCCESS;
        }
        static ma_result Seek(ma_data_source*, ma_uint64) { return MA_NOT_IMPLEMENTED; }
        static ma_result Length(ma_data_source*, ma_uint64*) { return MA_NOT_IMPLEMENTED; }

        ma_result Initialize(const wz::audio::GrainCloudDesc& desc, ma_uint32 rate, bool spatial)
        {
            if (!rate) return MA_INVALID_ARGS;
            cloud.Configure(desc);
            channels = spatial ? 1u : 2u;
            sampleRate = rate;
            cursor = 0;
            static const ma_data_source_vtable vtable{Read, Seek, Format, Cursor, Length, nullptr, 0};
            auto config = ma_data_source_config_init();
            config.vtable = &vtable;
            return ma_data_source_init(&config, &base);
        }
    };
}
