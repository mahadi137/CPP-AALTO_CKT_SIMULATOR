    cmake_minimum_required(VERSION 3.11.0)

    project(circuit-simulator VERSION 0.1.0 LANGUAGES CXX)

    # C++17 standard
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED True)

    # Set build type to Debug if not specified
    if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
    endif()

    # --- Header-Only Library Targets (Modern CMake Approach) ---
    add_library(Eigen INTERFACE)
    target_include_directories(Eigen INTERFACE
        "${CMAKE_SOURCE_DIR}/libs/eigen"
    )

    add_library(Odeint INTERFACE)
    target_include_directories(Odeint INTERFACE
        "${CMAKE_SOURCE_DIR}/libs/odeint-v2/include"
    )

# --- SFML ---
include(FetchContent)

# Pull SFML 2.6.1 at configure time (>= 2.5 as you requested, still 2.x)
FetchContent_Declare(
  SFML2
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG        2.6.1
)
# Optional: speed up build / avoid unused libs
set(SFML_BUILD_AUDIO   OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SFML2)

# --- ImPlot Setup (via FetchContent) ---
FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG v0.16
)
FetchContent_MakeAvailable(implot)

# Create ImPlot library from fetched content
set(IMPLOT_SOURCES
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)

add_library(implot STATIC ${IMPLOT_SOURCES})
target_include_directories(implot PUBLIC 
    ${implot_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libs/imgui
)

# --- OpenGL (needed by ImGui-SFML on macOS/arm64) ---
find_package(OpenGL REQUIRED)

    # --- ImGui Setup ---
    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/libs/imgui")
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
    )

    # --- ImGui-SFML Setup ---
    set(IMGUI_SFML_DIR "${CMAKE_SOURCE_DIR}/libs/imgui-sfml")
    set(IMGUI_SFML_SOURCES
        ${IMGUI_SFML_DIR}/imgui-SFML.cpp
    )

    # Create ImGui+SFML library
    add_library(ImGui-SFML STATIC
        ${IMGUI_SOURCES}
        ${IMGUI_SFML_SOURCES}
    )

target_include_directories(ImGui-SFML PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_SFML_DIR}
    ${implot_SOURCE_DIR}
)

target_link_libraries(ImGui-SFML PUBLIC
    implot
    sfml-graphics
    sfml-window
    sfml-system
    OpenGL::GL            # <--- this is the important part
)

# macOS sometimes prefers the framework name explicitly
if(APPLE)
    target_link_libraries(ImGui-SFML PUBLIC "-framework OpenGL")
endif()

target_compile_definitions(ImGui-SFML PUBLIC
    IMGUI_USER_CONFIG="${IMGUI_SFML_DIR}/imconfig-SFML.h"
)

# --- Project Sources (Modular GUI) ---
set(PROJECT_SOURCES
    src/main.cpp
    src/Circuit.cpp
    src/timedomainGraph.cpp
    src/MNASolver.cpp
    src/theme.cpp
    src/components/Resistor.cpp
    src/components/Capacitor.cpp
    src/components/Inductor.cpp
    src/components/VoltageSource.cpp
    src/components/DCCurrentSource.cpp
    src/components/ACCurrentSource.cpp
    src/components/ACVoltageSource.cpp
)

    # --- Executable Target (Backend CLI) ---
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

    # --- Target Properties ---
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
    )

# Link the executable against our header-only library targets.
# This automatically adds the include directories from Eigen and Odeint
# to our executable's build process.
target_link_libraries(${PROJECT_NAME} PRIVATE
    Eigen
    Odeint
    ImGui-SFML
    sfml-graphics
    sfml-window
    sfml-system
)

# Note: The main executable now includes all GUI functionality through main.cpp
# which uses the modular simulatorGUI classes

# --- GoogleTest Setup ---
# Disable GoogleTest install to avoid polluting system
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
# Use C++17 for GoogleTest as well
set(CMAKE_CXX_STANDARD 17)

        # Include backend sources (needed for simulation)
        src/Circuit.cpp
        src/MNASolver.cpp
        src/components/Resistor.cpp
        src/components/Capacitor.cpp
        src/components/Inductor.cpp
        src/components/VoltageSource.cpp
        src/components/CurrentSource.cpp
    )

    target_include_directories(circuit_gui PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
    )

# Test sources (without main.cpp)
set(TEST_COMMON_SOURCES
    src/Circuit.cpp
    src/MNASolver.cpp
    src/components/Resistor.cpp
    src/components/Capacitor.cpp
    src/components/Inductor.cpp
    src/components/VoltageSource.cpp
    src/components/DCCurrentSource.cpp
    src/components/ACCurrentSource.cpp
    src/components/ACVoltageSource.cpp
)

    # --- GoogleTest Setup ---
    # Disable GoogleTest install to avoid polluting system
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    # Use C++17 for GoogleTest as well
    set(CMAKE_CXX_STANDARD 17)

    # Add GoogleTest subdirectory
    add_subdirectory(libs/googletest)

    # --- Tests ---
    enable_testing()

# Discover tests automatically
include(GoogleTest)
gtest_discover_tests(circuit_tests)
