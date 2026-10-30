o3de_gem_setup("GrainSynthesizer")
if(NOT TARGET GrainSynthesizer::core)
    set(GRAIN_SYNTHESIZER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." "${CMAKE_CURRENT_BINARY_DIR}/grain-core" EXCLUDE_FROM_ALL)
endif()
ly_add_target(
    NAME ${gem_name}.Static STATIC NAMESPACE Gem
    FILES_CMAKE grain_synthesizer_files.cmake
    INCLUDE_DIRECTORIES PUBLIC Code/Include PRIVATE ../miniaudio
    BUILD_DEPENDENCIES PUBLIC AZ::AzCore Gem::MiniAudio.API
        PRIVATE GrainSynthesizer::core 3rdParty::miniaudio
)
if(MSVC)
    target_compile_options(${gem_name}.Static PRIVATE /EHsc)
else()
    target_compile_options(${gem_name}.Static PRIVATE -fexceptions)
endif()
ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE} NAMESPACE Gem
    FILES_CMAKE grain_synthesizer_shared_files.cmake
    BUILD_DEPENDENCIES PRIVATE Gem::${gem_name}.Static
)
ly_add_source_properties(SOURCES Code/Source/GrainSynthesizerModule.cpp
    PROPERTY COMPILE_DEFINITIONS VALUES O3DE_GEM_NAME=${gem_name})
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE NAMESPACE Gem
        FILES_CMAKE grain_synthesizer_editor_files.cmake
        BUILD_DEPENDENCIES PRIVATE Gem::${gem_name}.Static AZ::AzToolsFramework
    )
    ly_add_source_properties(SOURCES Code/Source/EditorGrainEmitterComponent.cpp
        PROPERTY COMPILE_DEFINITIONS VALUES O3DE_GEM_NAME=${gem_name})
    ly_create_alias(NAME ${gem_name}.Tools NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
endif()
o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name}
    VARIANTS Builders Tools Clients Unified)
