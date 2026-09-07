cmake_minimum_required(VERSION 3.24)
project(MineLongwallSimulation VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release")

option(MINE_BUILD_APP "Build the OpenGL application" ON)
option(MINE_BUILD_TESTS "Build deterministic logic tests" ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glm)

add_library(mine_simulation STATIC
  src/scene/Transform.cpp
  src/simulation/EventLog.cpp
  src/particles/ParticleSystem.cpp
  src/equipment/Shearer.cpp
  src/equipment/HydraulicSupport.cpp
  src/equipment/ScraperConveyor.cpp
  src/simulation/SimulationController.cpp)
target_include_directories(mine_simulation PUBLIC src)
target_link_libraries(mine_simulation PUBLIC glm::glm)
target_compile_definitions(mine_simulation PUBLIC GLM_ENABLE_EXPERIMENTAL)
if(MSVC)
  target_compile_options(mine_simulation PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(mine_simulation PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MINE_BUILD_APP)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(glfw)

  FetchContent_Declare(glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(glad)
  find_package(Python COMPONENTS Interpreter REQUIRED)
  execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "import glad"
    RESULT_VARIABLE GLAD_PYTHON_IMPORT_RESULT
    OUTPUT_QUIET ERROR_QUIET)
  if(NOT GLAD_PYTHON_IMPORT_RESULT EQUAL 0)
    message(STATUS "Installing the pinned GLAD generator from the fetched source tree")
    execute_process(
      COMMAND "${Python_EXECUTABLE}" -m pip install --disable-pip-version-check "${glad_SOURCE_DIR}"
      RESULT_VARIABLE GLAD_PYTHON_INSTALL_RESULT
      OUTPUT_VARIABLE GLAD_PYTHON_INSTALL_OUTPUT
      ERROR_VARIABLE GLAD_PYTHON_INSTALL_ERROR)
    if(NOT GLAD_PYTHON_INSTALL_RESULT EQUAL 0)
      message(FATAL_ERROR "Unable to install the pinned GLAD generator: ${GLAD_PYTHON_INSTALL_ERROR}")
    endif()
  endif()
  include("${glad_SOURCE_DIR}/cmake/GladConfig.cmake")
  glad_add_library(glad_gl_core_33 REPRODUCIBLE API gl:core=3.3)

  FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(imgui)
  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
  target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(imgui PUBLIC glfw glad_gl_core_33)
  target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD2)

  add_executable(MineLongwallSimulation
    src/main.cpp
    src/core/Application.cpp
    src/graphics/Camera.cpp
    src/graphics/Shader.cpp
    src/graphics/Mesh.cpp
    src/graphics/PostProcessor.cpp
    src/graphics/Renderer.cpp
    src/ui/UIManager.cpp)
  target_include_directories(MineLongwallSimulation PRIVATE src)
  target_link_libraries(MineLongwallSimulation PRIVATE mine_simulation glfw glad_gl_core_33 imgui)
  target_compile_definitions(MineLongwallSimulation PRIVATE
    MINE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
  if(MSVC)
    target_compile_options(MineLongwallSimulation PRIVATE /W4 /permissive- /utf-8)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MineLongwallSimulation)
  endif()
endif()

if(MINE_BUILD_TESTS)
  enable_testing()
  add_executable(mine_logic_tests tests/LogicTests.cpp)
  target_link_libraries(mine_logic_tests PRIVATE mine_simulation)
  add_test(NAME mine_logic_tests COMMAND mine_logic_tests)
endif()
