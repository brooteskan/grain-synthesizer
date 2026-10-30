#include <CloudDataSource.h>
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <cmath>

TEST(MiniAudioBridge, RendersThroughDeviceLessEngineAndDetachesSafely)
{
    ma_engine engine{};
    auto engineConfig = ma_engine_config_init();
    engineConfig.noDevice = MA_TRUE;
    engineConfig.channels = 2;
    engineConfig.sampleRate = 48000;
    ASSERT_EQ(ma_engine_init(&engineConfig, &engine), MA_SUCCESS);
    std::vector<float> pcm(48000, 0.1f);
    wz::audio::GrainCloudDesc desc;
    desc.sources[0] = {pcm.data(), pcm.size(), 48000, 1};
    desc.weights[0] = 1;
    desc.source_count = 1;
    desc.density = 100;
    for (bool spatial : {false, true})
    {
        GrainSynthesizer::CloudDataSource source;
        ASSERT_EQ(source.Initialize(desc, 48000, spatial), MA_SUCCESS);
        ma_sound sound{};
        ASSERT_EQ(ma_sound_init_from_data_source(&engine, &source,
            spatial ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, &sound), MA_SUCCESS);
        ASSERT_TRUE(source.cloud.Start());
        ASSERT_EQ(ma_sound_start(&sound), MA_SUCCESS);
        std::vector<float> out(8192);
        ASSERT_EQ(ma_engine_read_pcm_frames(&engine, out.data(), 4096, nullptr), MA_SUCCESS);
        EXPECT_TRUE(std::any_of(out.begin(), out.end(), [](float x) { return std::abs(x) > 0.00001f; }));
        EXPECT_TRUE(std::all_of(out.begin(), out.end(), [](float x) { return std::isfinite(x); }));
        ma_sound_uninit(&sound);
        ma_data_source_uninit(&source.base);
        std::fill(out.begin(), out.end(), 1.0f);
        ASSERT_EQ(ma_engine_read_pcm_frames(&engine, out.data(), 4096, nullptr), MA_SUCCESS);
        EXPECT_TRUE(std::all_of(out.begin(), out.end(), [](float x) { return x == 0; }));
    }
    ma_engine_uninit(&engine);
}
