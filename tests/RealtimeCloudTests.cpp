#include <GrainSynthesizer/RealtimeCloud.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
    struct Fixture : testing::Test
    {
        std::vector<float> pcm = std::vector<float>(48000, 0.2f);
        wz::audio::GrainCloudDesc desc;
        Fixture()
        {
            desc.sources[0] = {pcm.data(), pcm.size(), 48000, 1};
            desc.weights[0] = 1;
            desc.source_count = 1;
            desc.density = 100;
            desc.grain_ms = 20;
        }
        static bool Audible(const std::vector<float>& output)
        {
            return std::any_of(output.begin(), output.end(), [](float s) { return std::abs(s) > 0.00001f; });
        }
    };
    TEST_F(Fixture, SilentBeforeStartAndReplacesOutput)
    {
        GrainSynthesizer::RealtimeCloud cloud;
        cloud.Configure(desc);
        std::vector<float> out(4096, 1);
        cloud.Render(out.data(), 2048, 2, 48000);
        EXPECT_FALSE(Audible(out));
    }
    TEST_F(Fixture, StartAndStopPreserveNaturalTails)
    {
        GrainSynthesizer::RealtimeCloud cloud;
        cloud.Configure(desc);
        std::vector<float> out(4096);
        ASSERT_TRUE(cloud.Start());
        cloud.Render(out.data(), 2048, 2, 48000);
        EXPECT_TRUE(Audible(out));
        ASSERT_TRUE(cloud.Stop());
        cloud.Render(out.data(), 2048, 2, 48000);
        EXPECT_TRUE(Audible(out));
        cloud.Render(out.data(), 2048, 2, 48000);
        EXPECT_FALSE(Audible(out));
    }
    TEST_F(Fixture, RejectsInvalidNumbersAndUnboundedWork)
    {
        GrainSynthesizer::RealtimeCloud cloud;
        desc.density = std::numeric_limits<float>::quiet_NaN();
        EXPECT_THROW(cloud.Configure(desc), std::invalid_argument);
        desc.density = 100;
        desc.max_grains = 257;
        EXPECT_THROW(cloud.Configure(desc), std::invalid_argument);
        EXPECT_FALSE(cloud.SetParameter(wz::audio::GrainParam::Pitch, 100));
        EXPECT_FALSE(cloud.SetParameter(wz::audio::GrainParam::GrainMs, 0));
        EXPECT_FALSE(cloud.SetParameter(wz::audio::GrainParam::Gain, 1, -1));
        EXPECT_FALSE(cloud.SetParameter(wz::audio::GrainParam::SourceWeight, 1, 0, 8));
    }
    TEST_F(Fixture, QueueOverflowIsReportedAndRecovers)
    {
        GrainSynthesizer::RealtimeCloud cloud;
        cloud.Configure(desc);
        unsigned accepted = 0;
        while (accepted < 1000 && cloud.Start()) ++accepted;
        EXPECT_EQ(accepted, 127u);
        float out[2]{};
        cloud.Render(out, 1, 2, 48000);
        EXPECT_TRUE(cloud.Stop());
    }
    TEST_F(Fixture, GainRampsReachSilence)
    {
        GrainSynthesizer::RealtimeCloud cloud;
        cloud.Configure(desc);
        std::vector<float> out(4096);
        ASSERT_TRUE(cloud.Start());
        cloud.Render(out.data(), 2048, 2, 48000);
        ASSERT_TRUE(Audible(out));
        ASSERT_TRUE(cloud.SetParameter(wz::audio::GrainParam::Gain, 0, 0.01f));
        cloud.Render(out.data(), 2048, 2, 48000);
        EXPECT_TRUE(Audible(out));
        cloud.Render(out.data(), 2048, 2, 48000);
        EXPECT_FALSE(Audible(out));
    }
    TEST_F(Fixture, MonoAndStereoAreFinite)
    {
        for (uint32_t channels : {1u, 2u})
        {
            GrainSynthesizer::RealtimeCloud cloud;
            cloud.Configure(desc);
            ASSERT_TRUE(cloud.Start());
            std::vector<float> out(2048 * channels);
            cloud.Render(out.data(), 2048, channels, 44100);
            EXPECT_TRUE(Audible(out));
            EXPECT_TRUE(std::all_of(out.begin(), out.end(), [](float x) { return std::isfinite(x); }));
        }
    }
    TEST_F(Fixture, SingleProducerAndConsumerCanRunConcurrently)
    {
        GrainSynthesizer::RealtimeCloud cloud;
        cloud.Configure(desc);
        ASSERT_TRUE(cloud.Start());
        std::atomic<bool> done{false};
        std::thread producer([&] {
            for (unsigned i = 0; i < 2000; ++i)
                while (!cloud.SetParameter(wz::audio::GrainParam::Gain, float(i % 10) / 10, 0.01f))
                    std::this_thread::yield();
            done.store(true, std::memory_order_release);
        });
        // Keep consuming until producer signals completion, avoiding assumptions
        // about relative scheduling speeds on CI workers.
        float out[128]{};
        do { cloud.Render(out, 64, 2, 48000); }
        while (!done.load(std::memory_order_acquire));
        producer.join();
        cloud.Render(out, 64, 2, 48000);
        EXPECT_TRUE(std::all_of(std::begin(out), std::end(out), [](float x) { return std::isfinite(x); }));
    }
}
