#include <GrainSynthesizer/GrainEmitterComponent.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>

namespace GrainSynthesizer
{
    void GrainSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<GrainSettings>()->Version(1)
                ->Field("Gain", &GrainSettings::gain)
                ->Field("Density", &GrainSettings::density)
                ->Field("Position", &GrainSettings::position)
                ->Field("Pitch", &GrainSettings::pitch)
                ->Field("GrainMs", &GrainSettings::grainMs)
                ->Field("PositionJitter", &GrainSettings::positionJitter)
                ->Field("PitchJitter", &GrainSettings::pitchJitter)
                ->Field("PanCenter", &GrainSettings::panCenter)
                ->Field("PanSpread", &GrainSettings::panSpread)
                ->Field("WindowParam", &GrainSettings::windowParam)
                ->Field("BlendRate", &GrainSettings::blendRate)
                ->Field("BlendDepth", &GrainSettings::blendDepth)
                ->Field("Normalize", &GrainSettings::normalize)
                ->Field("MaxGrains", &GrainSettings::maxGrains)
                ->Field("Seed", &GrainSettings::seed);
            if (auto* ec = sc->GetEditContext())
            {
                ec->Class<GrainSettings>("Grain Settings", "Wozzits Gaussian grain cloud")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::gain, "Gain", "Linear amplitude")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::density, "Density", "Grains per second: 0..10000")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::position, "Position", "Normalized source position: 0..1")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::pitch, "Pitch", "Playback multiplier: 0..16")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::grainMs, "Grain Length (ms)", "1..10000 ms")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::positionJitter, "Position Jitter", "Normalized spread: 0..1")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::pitchJitter, "Pitch Jitter", "Semitone spread: 0..48")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::panCenter, "Pan Center", "-1 left to +1 right")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::panSpread, "Pan Spread", "0..1; use zero for point emitters")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::windowParam, "Gaussian Width", "Sigma: 0.001..1")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::blendRate, "Blend Rate", "Source-weight oscillator Hz; 0 disables")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::blendDepth, "Blend Depth", "0..1")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::normalize, "Normalization", "Reference overlap; 0 disables")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::maxGrains, "Grain Capacity", "1..256 simultaneous grains")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainSettings::seed, "Seed", "Deterministic random seed");
            }
        }
    }
    void GrainEmitterConfig::Reflect(AZ::ReflectContext* context)
    {
        GrainSettings::Reflect(context);
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<GrainEmitterConfig>()->Version(1)
                ->Field("Sources", &GrainEmitterConfig::sources)
                ->Field("Weights", &GrainEmitterConfig::weights)
                ->Field("Settings", &GrainEmitterConfig::settings)
                ->Field("AutoPlay", &GrainEmitterConfig::autoPlay)
                ->Field("Spatial", &GrainEmitterConfig::spatial)
                ->Field("MinDistance", &GrainEmitterConfig::minDistance)
                ->Field("MaxDistance", &GrainEmitterConfig::maxDistance);
            if (auto* ec = sc->GetEditContext())
                ec->Class<GrainEmitterConfig>("Grain Emitter", "Granular synthesis configuration")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainEmitterConfig::sources, "Sources", "One to eight MiniAudio sound assets (WAV, FLAC, MP3)")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainEmitterConfig::weights, "Weights", "Source selection weights; missing entries default to 1")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainEmitterConfig::settings, "Grain Settings", "Initial synthesis parameters")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainEmitterConfig::autoPlay, "Auto Play", "Start when runtime entity activates")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainEmitterConfig::spatial, "Spatial", "Mono point emitter; otherwise stereo ambience")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainEmitterConfig::minDistance, "Minimum Distance", "Attenuation near distance")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GrainEmitterConfig::maxDistance, "Maximum Distance", "Attenuation far distance");
        }
    }
}
