#include <AzCore/Module/Module.h>
#include <GrainSynthesizer/GrainEmitterComponent.h>

namespace GrainSynthesizer
{
    class GrainSynthesizerModule : public AZ::Module
    {
    public:
        AZ_RTTI(GrainSynthesizerModule, "{66E89CEF-71A6-43B8-B1D4-CB43CD49BA64}", AZ::Module);
        AZ_CLASS_ALLOCATOR(GrainSynthesizerModule, AZ::SystemAllocator);
        GrainSynthesizerModule() { m_descriptors.push_back(GrainEmitterComponent::CreateDescriptor()); }
    };
}
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), GrainSynthesizer::GrainSynthesizerModule)
