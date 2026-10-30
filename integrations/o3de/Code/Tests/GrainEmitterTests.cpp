#include <GrainSynthesizer/GrainEmitterComponent.h>
#include <MiniAudio/MiniAudioBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/AzTest.h>
#include <miniaudio.h>
#include <cmath>
#include <vector>

namespace GrainSynthesizer::Tests
{
    class TestSound final : public MiniAudio::SoundAsset
    {
    public:
        void Status(AssetStatus status) { m_status = status; }
    };
    class SoundHandler final : public AZ::Data::AssetHandler
    {
    public:
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId&, const AZ::Data::AssetType&) override { return aznew TestSound; }
        void DestroyAsset(AZ::Data::AssetPtr asset) override { delete asset; }
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& types) override
        { types.push_back(azrtti_typeid<MiniAudio::SoundAsset>()); }
        LoadResult LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>&,
            AZStd::shared_ptr<AZ::Data::AssetDataStream>, const AZ::Data::AssetFilterCB&) override
        { return LoadResult::Error; }
    };
    class HeadlessAudio final : public MiniAudio::MiniAudioRequests
    {
    public:
        ma_engine engine{};
        ma_engine* GetSoundEngine() override { return &engine; }
        void SetGlobalVolume(float gain) override { ma_engine_set_volume(&engine, gain); }
        float GetGlobalVolume() const override { return 1; }
        void SetGlobalVolumeInDecibels(float db) override { SetGlobalVolume(ma_volume_db_to_linear(db)); }
        AZ::u32 GetChannelCount() const override { return 2; }
    };
    class GrainEmitterTests : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();
            AZ::Data::AssetManager::Create(AZ::Data::AssetManager::Descriptor{});
            AZ::Data::AssetManager::Instance().RegisterHandler(&m_handler, azrtti_typeid<MiniAudio::SoundAsset>());
            m_grainDescriptor.reset(GrainEmitterComponent::CreateDescriptor());
            m_transformDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            auto config = ma_engine_config_init();
            config.noDevice = MA_TRUE; config.channels = 2; config.sampleRate = 48000;
            ASSERT_EQ(ma_engine_init(&config, &m_audio.engine), MA_SUCCESS);
            MiniAudio::MiniAudioInterface::Register(&m_audio);
        }
        void TearDown() override
        {
            m_entity.reset();
            MiniAudio::MiniAudioInterface::Unregister(&m_audio);
            ma_engine_uninit(&m_audio.engine);
            AZ::Data::AssetManager::Instance().DispatchEvents();
            AZ::Data::AssetManager::Instance().UnregisterHandler(&m_handler);
            AZ::Data::AssetManager::Destroy();
            m_transformDescriptor.reset(); m_grainDescriptor.reset();
            UnitTest::LeakDetectionFixture::TearDown();
        }
        AZ::Data::Asset<MiniAudio::SoundAsset> Clip(bool ready)
        {
            auto asset = AZ::Data::AssetManager::Instance().CreateAsset<MiniAudio::SoundAsset>(
                AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            auto& bytes = asset->m_data;
            bytes.resize(44 + 4096);
            auto write = [&](size_t offset, unsigned value, unsigned count)
            { for (unsigned i = 0; i < count; ++i) bytes[offset + i] = static_cast<AZ::u8>(value >> (8 * i)); };
            auto tag = [&](size_t offset, const char* text)
            { for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = text[i]; };
            tag(0, "RIFF"); write(4, static_cast<unsigned>(bytes.size() - 8), 4); tag(8, "WAVE");
            tag(12, "fmt "); write(16, 16, 4); write(20, 1, 2); write(22, 1, 2);
            write(24, 48000, 4); write(28, 96000, 4); write(32, 2, 2); write(34, 16, 2);
            tag(36, "data"); write(40, 4096, 4);
            for (size_t i = 44; i < bytes.size(); i += 2) write(i, 4096, 2);
            static_cast<TestSound*>(asset.Get())->Status(ready ? AZ::Data::AssetData::AssetStatus::Ready : AZ::Data::AssetData::AssetStatus::Loading);
            return asset;
        }
        GrainEmitterComponent* Activate(const GrainEmitterConfig& config, float x = 0)
        {
            m_entity = AZStd::make_unique<AZ::Entity>();
            auto* transform = m_entity->CreateComponent<AzFramework::TransformComponent>();
            auto* grain = m_entity->CreateComponent<GrainEmitterComponent>(config);
            m_entity->Init();
            static_cast<AZ::TransformInterface*>(transform)->SetWorldTranslation(AZ::Vector3(x, 0, 0));
            m_entity->Activate();
            return grain;
        }
        void Ready(AZ::Data::Asset<MiniAudio::SoundAsset> asset)
        {
            static_cast<TestSound*>(asset.Get())->Status(AZ::Data::AssetData::AssetStatus::Ready);
            AZ::Data::AssetBus::Event(asset.GetId(), &AZ::Data::AssetEvents::OnAssetReady, asset);
        }
        double Energy()
        {
            std::vector<float> out(48000 * 2);
            EXPECT_EQ(ma_engine_read_pcm_frames(&m_audio.engine, out.data(), 48000, nullptr), MA_SUCCESS);
            double energy = 0;
            for (float value : out) energy += double(value) * value;
            return energy;
        }
        GrainEmitterConfig Config(AZ::Data::Asset<MiniAudio::SoundAsset> clip)
        {
            GrainEmitterConfig config; config.sources = {clip}; config.settings.density = 100;
            return config;
        }
        SoundHandler m_handler;
        HeadlessAudio m_audio;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_grainDescriptor, m_transformDescriptor;
        AZStd::unique_ptr<AZ::Entity> m_entity;
    };
    TEST_F(GrainEmitterTests, AlreadyReadySourcesStartWithoutAnyFrameDispatch)
    {
        auto* grain = Activate(Config(Clip(true)));
        EXPECT_TRUE(grain->SetParameter(0, 0.25f, 0, 0));
        EXPECT_GT(Energy(), 0);
    }
    TEST_F(GrainEmitterTests, StartsOnlyWhenAllSourcesNotifyReadyIncludingDuplicateReferences)
    {
        auto first = Clip(false), second = Clip(false);
        auto config = Config(first); config.sources = {first, second, first};
        auto* grain = Activate(config);
        EXPECT_FALSE(grain->SetParameter(0, 0.25f, 0, 0));
        EXPECT_TRUE(grain->Play()); // Repeated pending requests do not duplicate subscriptions.
        Ready(first);
        EXPECT_FALSE(grain->SetParameter(0, 0.25f, 0, 0));
        Ready(second);
        EXPECT_TRUE(grain->SetParameter(0, 0.25f, 0, 0));
        EXPECT_GT(Energy(), 0);
    }
    TEST_F(GrainEmitterTests, SynchronousReadyNotificationDuringConnectionStartsPlayback)
    {
        auto loaded = Clip(true);
        AZ::Data::Asset<MiniAudio::SoundAsset> reference(loaded.GetId(), azrtti_typeid<MiniAudio::SoundAsset>(), "");
        ASSERT_FALSE(reference.IsReady());
        auto* grain = Activate(Config(reference));
        EXPECT_TRUE(grain->SetParameter(0, 0.25f, 0, 0));
        EXPECT_GT(Energy(), 0);
    }
    TEST_F(GrainEmitterTests, StopDuringLoadingPreventsLateNotificationFromStartingPlayback)
    {
        auto clip = Clip(false);
        auto* grain = Activate(Config(clip));
        EXPECT_TRUE(grain->Stop()); Ready(clip);
        EXPECT_FALSE(grain->SetParameter(0, 0.25f, 0, 0));
        EXPECT_EQ(Energy(), 0);
        EXPECT_TRUE(grain->Play());
        EXPECT_TRUE(grain->SetParameter(0, 0.25f, 0, 0));
    }
    TEST_F(GrainEmitterTests, DisabledAutoPlayWaitsForExplicitRequest)
    {
        auto config = Config(Clip(true)); config.autoPlay = false;
        auto* grain = Activate(config);
        EXPECT_FALSE(grain->SetParameter(0, 0.25f, 0, 0));
        EXPECT_TRUE(grain->Play());
        EXPECT_TRUE(grain->SetParameter(0, 0.25f, 0, 0));
    }
    TEST_F(GrainEmitterTests, InvalidSourceFailsWithoutLeavingAWaitingSubscription)
    {
        GrainEmitterConfig config;
        config.sources.emplace_back();
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto* grain = Activate(config);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(grain->SetParameter(0, 0.25f, 0, 0));
        EXPECT_EQ(Energy(), 0);
    }
    TEST_F(GrainEmitterTests, ErrorCancelsPendingStartAndDisconnectsNotifications)
    {
        auto clip = Clip(false);
        auto* grain = Activate(Config(clip));
        AZ_TEST_START_TRACE_SUPPRESSION;
        AZ::Data::AssetBus::Event(clip.GetId(), &AZ::Data::AssetEvents::OnAssetError, clip);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        Ready(clip);
        EXPECT_FALSE(grain->SetParameter(0, 0.25f, 0, 0));
        EXPECT_EQ(Energy(), 0);
    }
    TEST_F(GrainEmitterTests, DeactivationDisconnectsAssetsAndTransformEvents)
    {
        auto clip = Clip(false);
        auto config = Config(clip); config.spatial = true;
        auto* grain = Activate(config);
        m_entity->Deactivate(); Ready(clip);
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformInterface::SetWorldTranslation, AZ::Vector3(20, 0, 0));
        EXPECT_FALSE(grain->Play());
        EXPECT_FALSE(grain->SetParameter(0, 0.25f, 0, 0));
        EXPECT_EQ(Energy(), 0);
    }
    TEST_F(GrainEmitterTests, SpatialPositionIsInitializedAndChangesThroughTransformNotifications)
    {
        auto config = Config(Clip(true)); config.spatial = true; config.settings.panSpread = 0;
        Activate(config, 50);
        const double distant = Energy();
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformInterface::SetWorldTranslation, AZ::Vector3::CreateZero());
        const double close = Energy();
        EXPECT_GT(close, distant * 10);
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformInterface::SetWorldTranslation, AZ::Vector3(50, 0, 0));
        EXPECT_LT(Energy(), close / 10);
    }
}
