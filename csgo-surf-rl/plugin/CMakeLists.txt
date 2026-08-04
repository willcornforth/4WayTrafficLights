cmake_minimum_required(VERSION 3.16)
project(surf_telemetry_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source plugins are 32-bit on the CS:GO-era engine. Force -m32 unless the
# integrator overrides for a 64-bit engine build.
option(SURF_BUILD_32BIT "Build a 32-bit module (CS:GO-era engine)" ON)

# Point this at a checked-out HL2SDK to compile against the real engine
# headers/types; leave empty to build with the self-contained shims (useful
# for a compile smoke-check of the transport/hook logic).
set(HL2SDK_ROOT "" CACHE PATH "Path to an HL2SDK checkout (optional)")

# Point this at a Dear ImGui checkout to compile the real debug-menu UI.
# Leave empty to build with the no-op ImGui shim (the in-world IVDebugOverlay
# still works either way).
set(IMGUI_ROOT "" CACHE PATH "Path to a Dear ImGui checkout (optional)")

add_library(surf_telemetry SHARED
  src/plugin.cpp
  src/session.cpp
  src/game_adapter.cpp
  src/createmove_hook.cpp
  src/ipc_client.cpp
  src/debug_overlay.cpp
  src/debug_menu.cpp
  src/imgui_backend.cpp
)

target_include_directories(surf_telemetry PRIVATE include src)

if(HL2SDK_ROOT)
  target_compile_definitions(surf_telemetry PRIVATE SURF_USE_HL2SDK=1)
  target_include_directories(surf_telemetry PRIVATE
    ${HL2SDK_ROOT}/public
    ${HL2SDK_ROOT}/public/tier0
    ${HL2SDK_ROOT}/public/tier1
  )
endif()

if(IMGUI_ROOT)
  target_compile_definitions(surf_telemetry PRIVATE SURF_USE_IMGUI=1)
  target_include_directories(surf_telemetry PRIVATE ${IMGUI_ROOT} ${IMGUI_ROOT}/backends)
  target_sources(surf_telemetry PRIVATE
    ${IMGUI_ROOT}/imgui.cpp
    ${IMGUI_ROOT}/imgui_draw.cpp
    ${IMGUI_ROOT}/imgui_tables.cpp
    ${IMGUI_ROOT}/imgui_widgets.cpp
  )
endif()

if(SURF_BUILD_32BIT AND NOT MSVC)
  target_compile_options(surf_telemetry PRIVATE -m32)
  target_link_options(surf_telemetry PRIVATE -m32)
endif()

# Match Source's module naming: no "lib" prefix; .dll on Windows, .so on Linux.
set_target_properties(surf_telemetry PROPERTIES
  PREFIX ""
  OUTPUT_NAME "surf_telemetry"
)

if(WIN32)
  target_link_libraries(surf_telemetry PRIVATE ws2_32)
endif()
