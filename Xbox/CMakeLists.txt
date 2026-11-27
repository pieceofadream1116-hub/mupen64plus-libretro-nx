# N64: Mupen64Plus-Next (Nintendo 64) in a standalone UWP app for Xbox Dev Mode (and Windows), made the same way as the
# Genesis app.
#
# Configure with the Visual Studio generator for WindowsStore, after building SDL3-uwp into ../out/uwp-deps:
#   cmake -S Xbox -B out/xbox -G "Visual Studio 17 2022" -A x64 -DCMAKE_SYSTEM_NAME=WindowsStore
#         -DCMAKE_SYSTEM_VERSION=10.0 -DCMAKE_PREFIX_PATH=out/uwp-deps
cmake_minimum_required(VERSION 3.24)
project(N64Xbox VERSION 0.1.0 LANGUAGES C CXX)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    message(FATAL_ERROR "The Xbox build is a UWP app: configure with -DCMAKE_SYSTEM_NAME=WindowsStore")
endif ()

set(APP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(APP_PACKAGE_VERSION "" CACHE STRING "Four-part package version (e.g. 0.1.0.12); CI sets it from the run number")

# Visual Studio's UWP projects turn on SDL checks (/sdl), which make CRT deprecation warnings errors
add_compile_options(/sdl-)
# Xbox Series X|S (Zen 2)
add_compile_options(/arch:AVX2)

include(FetchContent)

## Dear ImGui (menus)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

find_package(SDL3 REQUIRED CONFIG)

## The core (Mupen64Plus-Next with GLideN64), built beforehand with clang-cl by core/CMakeLists.txt, and ANGLE (OpenGL ES
## on Direct3D 11 for UWP, from Microsoft's ANGLE.WindowsStore package)
set(N64_CORE_LIB "" CACHE FILEPATH "n64_core.lib made by core/CMakeLists.txt")
set(ANGLE_ROOT "" CACHE PATH "The ANGLE.WindowsStore package folder")
if (NOT EXISTS "${N64_CORE_LIB}" OR NOT EXISTS "${ANGLE_ROOT}/Include/EGL/egl.h")
    message(FATAL_ERROR "Build the core first and give N64_CORE_LIB and ANGLE_ROOT (see .github/workflows/n64.yml)")
endif ()
set(ANGLE_BIN "${ANGLE_ROOT}/bin/UAP/x64")

## The app
add_executable(N64 WIN32
    src/main.cpp
    src/App.cpp
    src/App.h
    src/Core.cpp
    src/Core.h
    src/GlesContext.cpp
    src/GlesContext.h
    src/XboxAudio.cpp
    src/XboxAudio.h
    src/XboxInput.cpp
    src/XboxInput.h
    src/EdgeScaler.cpp
    src/EdgeScaler.h
    src/Covers.cpp
    src/Covers.h
    src/KnownCheats.cpp
    src/KnownCheats.h
    src/Net.cpp
    src/Net.h
    src/Library.cpp
    src/Library.h
    src/Settings.cpp
    src/Settings.h
    src/UwpPlatform.cpp
    src/UwpPlatform.h
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp"
)
target_include_directories(N64 PRIVATE src "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")
target_link_libraries(N64 PRIVATE "${N64_CORE_LIB}" "${ANGLE_BIN}/libEGL.lib" "${ANGLE_BIN}/libGLESv2.lib"
    SDL3::SDL3 xaudio2)
target_include_directories(N64 PRIVATE "${APP_ROOT}/libretro" "${APP_ROOT}/libretro-common/include" "${ANGLE_ROOT}/Include")
# ANGLE's DLLs go in the package next to the app
set(ANGLE_DLLS "${ANGLE_BIN}/libEGL.dll" "${ANGLE_BIN}/libGLESv2.dll")
target_sources(N64 PRIVATE ${ANGLE_DLLS})
set_source_files_properties(${ANGLE_DLLS} PROPERTIES VS_DEPLOYMENT_CONTENT 1)
set_target_properties(N64 PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_compile_options(N64 PRIVATE /utf-8 /MP /W3 /wd4996 /EHsc)
target_compile_definitions(N64 PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_link_options(N64 PRIVATE /STACK:8388608 "/MAP:${CMAKE_BINARY_DIR}/N64.map")

## UWP package
if (APP_PACKAGE_VERSION)
    set(APP_VERSION4 "${APP_PACKAGE_VERSION}")
else ()
    set(APP_VERSION4 "${PROJECT_VERSION}.0")
endif ()
message(STATUS "N64: package version ${APP_VERSION4}")
target_compile_definitions(N64 PRIVATE "APP_VERSION=\"${APP_VERSION4}\"")

configure_file(Package.appxmanifest.in "${CMAKE_CURRENT_BINARY_DIR}/Package.appxmanifest" @ONLY)
target_sources(N64 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/Package.appxmanifest")

file(GLOB APP_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.png")
target_sources(N64 PRIVATE ${APP_ASSETS})
set_source_files_properties(${APP_ASSETS} PROPERTIES VS_DEPLOYMENT_CONTENT 1 VS_DEPLOYMENT_LOCATION "Assets")

# Font, and Mupen64Plus-Next's license (it must travel with the app)
set(APP_RES
    "${CMAKE_CURRENT_SOURCE_DIR}/res/SplineSans-Bold.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/SplineSans-OFL.txt"
    "${APP_ROOT}/LICENSE")
target_sources(N64 PRIVATE ${APP_RES})
set_source_files_properties(${APP_RES} PROPERTIES VS_DEPLOYMENT_CONTENT 1 VS_DEPLOYMENT_LOCATION "res")

set(APP_CERT "${CMAKE_CURRENT_BINARY_DIR}/PS1_TemporaryKey.pfx")
configure_file("${CMAKE_ROOT}/Templates/Windows/Windows_TemporaryKey.pfx" "${APP_CERT}" COPYONLY)
target_sources(N64 PRIVATE "${APP_CERT}")

get_target_property(APP_SDL_DLL SDL3::SDL3-shared IMPORTED_LOCATION_RELEASE)
if (NOT APP_SDL_DLL)
    get_target_property(APP_SDL_DLL SDL3::SDL3-shared IMPORTED_LOCATION)
endif ()
if (NOT APP_SDL_DLL OR NOT EXISTS "${APP_SDL_DLL}")
    message(FATAL_ERROR "SDL3.dll not found; build SDL3-uwp first (Xbox/build.ps1)")
endif ()
target_sources(N64 PRIVATE "${APP_SDL_DLL}")
set_source_files_properties("${APP_SDL_DLL}" PROPERTIES VS_DEPLOYMENT_CONTENT 1)

set_target_properties(N64 PROPERTIES VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION "10.0.17763.0")
