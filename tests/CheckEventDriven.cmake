# Enforce the component architecture independently of an installed O3DE SDK.
file(GLOB_RECURSE sources "${CMAKE_CURRENT_LIST_DIR}/../integrations/o3de/Code/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/../integrations/o3de/Code/*.cpp")
foreach(source IN LISTS sources)
    file(READ "${source}" contents)
    if(contents MATCHES "(TickBus|SystemTickBus|OnTick[ \t]*\\()")
        message(FATAL_ERROR "Frame polling is forbidden in the audio Gem: ${source}")
    endif()
endforeach()
