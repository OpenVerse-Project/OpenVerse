# OVUI Framework — Root CMake
cmake_minimum_required(VERSION 3.22)
project(OVUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(OVUI_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

option(OVUI_BUILD_TESTS "Build OVUI tests" ON)
option(OVUI_ENGINE_MODE "Build with OpenVerse engine integration" OFF)

# ============================================================
# ovui_core — header-only, zero dependencies
# ============================================================
add_library(ovui_core INTERFACE)
target_include_directories(ovui_core INTERFACE ${OVUI_ROOT})
target_sources(ovui_core INTERFACE Core/Types.h)

# ============================================================
# ovui_reactive — header-only, depends on core
# ============================================================
add_library(ovui_reactive INTERFACE)
target_include_directories(ovui_reactive INTERFACE ${OVUI_ROOT})
target_sources(ovui_reactive INTERFACE Reactive/Reactive.h)
target_link_libraries(ovui_reactive INTERFACE ovui_core)

# ============================================================
# ovui_layout — compiled, depends on core
# ============================================================
add_library(ovui_layout STATIC)
target_sources(ovui_layout PRIVATE
    Layout/FlexEngine.h
    Layout/FlexEngine.cpp
)
target_include_directories(ovui_layout PUBLIC ${OVUI_ROOT})
target_link_libraries(ovui_layout PUBLIC ovui_core)

# ============================================================
# ovui_styling — compiled, depends on core + reactive
# ============================================================
add_library(ovui_styling STATIC)
target_sources(ovui_styling PRIVATE
    Styling/Styling.h
    Styling/Styling.cpp
)
target_include_directories(ovui_styling PUBLIC ${OVUI_ROOT})
target_link_libraries(ovui_styling PUBLIC ovui_core ovui_reactive)

# ============================================================
# ovui_widgets — compiled, depends on all
# ============================================================
add_library(ovui_widgets STATIC)
target_sources(ovui_widgets PRIVATE
    Widgets/Widgets.h
    Widgets/Widgets.cpp
    Widgets/Advanced.h
    Widgets/Advanced.cpp
)
target_include_directories(ovui_widgets PUBLIC ${OVUI_ROOT})
target_link_libraries(ovui_widgets PUBLIC ovui_core ovui_layout ovui_styling ovui_reactive)

# ============================================================
# ovui_runtime — compiled, depends on widgets
# ============================================================
add_library(ovui_runtime STATIC)
target_sources(ovui_runtime PRIVATE
    Runtime/Scheduler.h
    Runtime/HotReload.h
    Runtime/HotReload.cpp
)
target_include_directories(ovui_runtime PUBLIC ${OVUI_ROOT})
target_link_libraries(ovui_runtime PUBLIC ovui_widgets stdc++fs)

# ============================================================
# ovui_graphics — compiled, depends on widgets, freetype, harfbuzz, vulkan, xcb
# ============================================================
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)

find_package(Vulkan QUIET)
if(NOT Vulkan_FOUND)
    find_library(VULKAN_LIB vulkan)
    find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h)
    if(VULKAN_LIB AND VULKAN_INCLUDE_DIR)
        set(OVUI_HAS_VULKAN ON)
        set(Vulkan_LIBRARY ${VULKAN_LIB})
        set(Vulkan_INCLUDE_DIR ${VULKAN_INCLUDE_DIR})
        message(STATUS "OVUI: Vulkan found (manual) — enabling Vulkan render backend")
    endif()
else()
    set(OVUI_HAS_VULKAN ON)
    set(VULKAN_INCLUDE_DIR ${Vulkan_INCLUDE_DIR})
    message(STATUS "OVUI: Vulkan found (cmake module) — enabling Vulkan render backend")
endif()

add_library(ovui_graphics STATIC)
target_sources(ovui_graphics PRIVATE
    Graphics/TextEngine.h
    Graphics/TextEngine.cpp
    Graphics/Effects.h
    Graphics/Effects.cpp
    Graphics/RenderBackend.h
    Graphics/RenderBackend.cpp
)

if(OVUI_HAS_VULKAN)
    target_sources(ovui_graphics PRIVATE
        Graphics/Vulkan/VulkanBackend.h
        Graphics/Vulkan/VulkanBackend.cpp
    )
    target_compile_definitions(ovui_graphics PUBLIC OVUI_HAS_VULKAN=1 VK_USE_PLATFORM_XCB_KHR)
endif()

target_include_directories(ovui_graphics PUBLIC ${OVUI_ROOT} ${FREETYPE_INCLUDE_DIRS} ${HARFBUZZ_INCLUDE_DIRS})
target_link_libraries(ovui_graphics PUBLIC ovui_widgets ${FREETYPE_LIBRARIES} ${HARFBUZZ_LIBRARIES})
if(OVUI_HAS_VULKAN)
    target_include_directories(ovui_graphics PUBLIC ${VULKAN_INCLUDE_DIR})
    target_link_libraries(ovui_graphics PUBLIC ${Vulkan_LIBRARY} xcb)
endif()

# ============================================================
# ovui_animation — compiled, depends on core + reactive
# ============================================================
add_library(ovui_animation STATIC)
target_sources(ovui_animation PRIVATE
    Animation/Animation.h
    Animation/Animation.cpp
)
target_include_directories(ovui_animation PUBLIC ${OVUI_ROOT})
target_link_libraries(ovui_animation PUBLIC ovui_core ovui_reactive)

# ============================================================
# ovui_designer — compiled, depends on widgets
# ============================================================
add_library(ovui_designer STATIC)
target_sources(ovui_designer PRIVATE
    Designer/DesignSurface.h
    Designer/DesignSurface.cpp
    Designer/PropertyInspector.h
    Designer/PropertyInspector.cpp
    Designer/AnimationTimeline.h
    Designer/AnimationTimeline.cpp
    Designer/ThemeEditor.h
    Designer/ThemeEditor.cpp
)
target_include_directories(ovui_designer PUBLIC ${OVUI_ROOT})
target_link_libraries(ovui_designer PUBLIC ovui_widgets ovui_styling)

# ============================================================
# ovui_desktop — compiled, depends on widgets + designer
# ============================================================
add_library(ovui_desktop STATIC)
target_sources(ovui_desktop PRIVATE
    Desktop/WindowManager.h
    Desktop/WindowManager.cpp
)
target_include_directories(ovui_desktop PUBLIC ${OVUI_ROOT})
target_link_libraries(ovui_desktop PUBLIC ovui_widgets ovui_designer)

# ============================================================
# ovui_compiler — compiled, depends on core
# ============================================================
add_library(ovui_compiler STATIC)
target_sources(ovui_compiler PRIVATE
    Compiler/OVMLParser.h
    Compiler/OVMLParser.cpp
    Compiler/OVSSCompiler.h
    Compiler/OVSSCompiler.cpp
    Compiler/OVMLCodeGen.h
    Compiler/OVMLCodeGen.cpp
    Compiler/Bytecode.h
    Compiler/Bytecode.cpp
    Compiler/BytecodeVM.h
    Compiler/BytecodeVM.cpp
)
target_include_directories(ovui_compiler PUBLIC ${OVUI_ROOT})
target_link_libraries(ovui_compiler PUBLIC ovui_core ovui_widgets)

# ============================================================
# Tests
# ============================================================
if(OVUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Testing)
endif()
