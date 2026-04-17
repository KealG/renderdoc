if(NOT WIN32)
    return()
endif()

set(sources
    vulkan_proxy.cpp
)

add_library(renderdocproxy SHARED ${sources})

target_include_directories(renderdocproxy
    PRIVATE ${CMAKE_SOURCE_DIR}/renderdoc
    PRIVATE ${CMAKE_SOURCE_DIR}/renderdoc/driver/vulkan/official
)

target_compile_definitions(renderdocproxy
    PRIVATE VK_USE_PLATFORM_WIN32_KHR
    PRIVATE NOMINMAX
    PRIVATE WIN32_LEAN_AND_MEAN
)

set_target_properties(renderdocproxy PROPERTIES OUTPUT_NAME "vulkan-1")

install(TARGETS renderdocproxy DESTINATION bin)
