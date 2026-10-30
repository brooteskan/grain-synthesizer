#pragma once
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

namespace GrainSynthesizer
{
    // Main-thread only. Parameter ordinals match wz::audio::GrainParam.
    class GrainEmitterRequests : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        virtual bool Play() = 0;
        virtual bool Stop() = 0;
        virtual bool SetParameter(AZ::u32 parameter, float value, float rampSeconds, AZ::u32 sourceIndex) = 0;
    };
    using GrainEmitterRequestBus = AZ::EBus<GrainEmitterRequests>;
}
