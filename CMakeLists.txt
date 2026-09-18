cmake_minimum_required(VERSION 3.16)
project(DreamVillage LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(DV_BUILD_APP      "Build the interactive GLUT application (needs OpenGL + GLUT)" ON)
option(DV_BUILD_HEADLESS "Build the head-less renderer / regression test on softgl (no GPU needed)" ON)

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
endif()

# third_party/stb_image_write.h calls sprintf() in its HDR writer (unused here);
# MSVC's /sdl check and UCRT-based MinGW builds treat that as an insecure-API
# error (C4996).  The deprecation is silenced project-wide.
if(MSVC OR MINGW)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# --- shared pieces -----------------------------------------------------------
# The scene is compiled twice on purpose: once against the system GL headers
# for the application and once against softgl's headers for the test binary.
set(DV_SCENE_SOURCES src/scene.cpp src/png_writer.cpp)

# --- interactive application --------------------------------------------------
if(DV_BUILD_APP)
  find_package(OpenGL)
  find_package(GLUT)
  if(OPENGL_FOUND AND GLUT_FOUND)
    add_executable(dream_village src/main.cpp ${DV_SCENE_SOURCES})
    target_include_directories(dream_village PRIVATE src ${GLUT_INCLUDE_DIRS})
    target_include_directories(dream_village SYSTEM PRIVATE third_party)
    target_link_libraries(dream_village PRIVATE OpenGL::GL ${GLUT_LIBRARIES})
    if(APPLE)
      target_compile_definitions(dream_village PRIVATE GL_SILENCE_DEPRECATION=1)
    endif()
    install(TARGETS dream_village RUNTIME DESTINATION bin)
  else()
    message(WARNING "OpenGL/GLUT not found - skipping the interactive application "
                    "(install freeglut3-dev on Debian/Ubuntu, or use the Visual Studio solution on Windows)")
  endif()
endif()

# --- head-less renderer + regression test ------------------------------------
if(DV_BUILD_HEADLESS)
  add_library(softgl STATIC tools/softgl/softgl.c)
  target_include_directories(softgl PUBLIC tools/softgl/include)
  target_compile_definitions(softgl PUBLIC DV_SOFTGL=1)
  if(UNIX)
    target_link_libraries(softgl PUBLIC m)
  endif()

  add_executable(render_headless
    tools/render_headless.cpp
    tools/legacy_shim.cpp
    ${DV_SCENE_SOURCES})
  target_include_directories(render_headless PRIVATE src)
  target_include_directories(render_headless SYSTEM PRIVATE third_party)
  target_compile_definitions(render_headless PRIVATE DV_WITH_LEGACY=1)
  target_link_libraries(render_headless PRIVATE softgl)
  if(NOT WIN32)
    # legacy/main.cpp includes <windows.h>/<mmsystem.h> without using them.
    set_source_files_properties(tools/legacy_shim.cpp PROPERTIES
      INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/tools/softgl/include/compat)
  endif()
  # legacy/main.cpp is compiled verbatim and is not warning-clean; keep the
  # port and the tools at full warnings but silence the legacy unit.
  if(MSVC)
    set_source_files_properties(tools/legacy_shim.cpp PROPERTIES COMPILE_OPTIONS "/w")
  else()
    set_source_files_properties(tools/legacy_shim.cpp PROPERTIES COMPILE_OPTIONS "-w")
  endif()

  enable_testing()
  # Renders the four scenes with softgl, checks for GL errors and coverage, and
  # requires the port to be pixel-identical to the legacy program.
  add_test(NAME headless_render_matches_legacy
           COMMAND render_headless --out ${CMAKE_BINARY_DIR} --compare --max-diff-ppm 0)
  # The rasteriser itself has a small self-test.
  add_executable(softgl_selftest tools/softgl/selftest.c)
  target_link_libraries(softgl_selftest PRIVATE softgl)
  add_test(NAME softgl_selftest COMMAND softgl_selftest)
endif()
