cmake_minimum_required(VERSION 3.24)

project(TwoOLProceduralPlantLab VERSION 0.5.0 LANGUAGES CXX)

option(TWOL_BUILD_OPENGL_VIEWER "Build the GLFW/OpenGL viewer" ON)
option(TWOL_BUILD_HEADLESS_DUMP "Build the headless exporter" ON)

add_library(twol_core STATIC
    src/twol/math.cpp
    src/twol/random.cpp
    src/twol/lsystem.cpp
    src/twol/presets.cpp
    src/twol/generator.cpp
    src/twol/mesh_builder.cpp
    src/twol/io.cpp
)

target_include_directories(twol_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(twol_core PUBLIC cxx_std_23)

if(MSVC)
    target_compile_options(twol_core PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(twol_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(TWOL_BUILD_HEADLESS_DUMP)
    add_executable(twol_headless_dump examples/headless_dump/main.cpp)
    target_link_libraries(twol_headless_dump PRIVATE twol_core)
    target_compile_features(twol_headless_dump PRIVATE cxx_std_23)
endif()

if(TWOL_BUILD_OPENGL_VIEWER)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)

    add_executable(twol_plant_viewer examples/opengl_glfw/main.cpp)
    target_link_libraries(twol_plant_viewer PRIVATE twol_core glfw OpenGL::GL)
    target_compile_features(twol_plant_viewer PRIVATE cxx_std_23)

    if(MSVC)
        target_compile_options(twol_plant_viewer PRIVATE /W4 /permissive- /Zc:__cplusplus)
    else()
        target_compile_options(twol_plant_viewer PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
