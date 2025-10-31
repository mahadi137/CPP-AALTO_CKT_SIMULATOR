cmake_minimum_required(VERSION 3.24)

project(restricted_pointer
    VERSION 0.1
    LANGUAGES CXX
)

# --- language setup ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- dependencies via FetchContent ------------------------------------------
include(FetchContent)

#
# Eigen (header-only linear algebra)
#
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
)
FetchContent_MakeAvailable(eigen)
# This defines an interface target called 'eigen'.

# --- our executable ---------------------------------------------------------

# grab all .cpp in src/
file(GLOB_RECURSE PROJECT_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

# Your own project headers (yours should *not* be SYSTEM, we want warnings here)
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Pull Eigen's include directory so we can mark it SYSTEM
get_target_property(EIGEN_INCLUDE_DIR eigen INTERFACE_INCLUDE_DIRECTORIES)

# Mark third-party headers as SYSTEM to mute their warnings
target_include_directories(${PROJECT_NAME}
    SYSTEM PRIVATE
        ${EIGEN_INCLUDE_DIR}
)

# Link ImGui-SFML (which also links imgui + SFML + OpenGL)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        eigen
)

# --- warnings / nice-to-haves ----------------------------------------------
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
