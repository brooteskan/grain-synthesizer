#include <GrainSynthesizer/GrainEmitterComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzCore/Module/Module.h>

namespace GrainSynthesizer
{
    class EditorGrainEmitterComponent final : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorGrainEmitterComponent, "{97322BEF-5369-4654-876D-75C87C0184C2}");
        static void Reflect(AZ::ReflectContext* context)
        {
            // The module reflects the runtime descriptor (and shared config) first.
            if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
            {
                sc->Class<EditorGrainEmitterComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(1)->Field("Configuration", &EditorGrainEmitterComponent::m_config);
                if (auto* ec = sc->GetEditContext())
                    ec->Class<EditorGrainEmitterComponent>("Grain Emitter", "Wozzits granular synthesis through MiniAudio")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorGrainEmitterComponent::m_config, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
        void BuildGameEntity(AZ::Entity* entity) override
        {
            entity->CreateComponent<GrainEmitterComponent>(m_config);
        }
    private:
        GrainEmitterConfig m_config;
    };
    class GrainSynthesizerEditorModule final : public AZ::Module
    {
    public:
        AZ_RTTI(GrainSynthesizerEditorModule, "{A02BF0E8-231C-4245-B8F8-8D3F103074FB}", AZ::Module);
        AZ_CLASS_ALLOCATOR(GrainSynthesizerEditorModule, AZ::SystemAllocator);
        GrainSynthesizerEditorModule()
        {
            m_descriptors.push_back(GrainEmitterComponent::CreateDescriptor());
            m_descriptors.push_back(EditorGrainEmitterComponent::CreateDescriptor());
        }
    };
}
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), GrainSynthesizer::GrainSynthesizerEditorModule)
