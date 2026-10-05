# Portable device core. Builds as an ESP-IDF component or a plain CMake library.

# The face: the shipped mascot, or faces/<name>.cpp from `hermes-gadget face`,
# chosen with CONFIG_HG_FACE (ESP-IDF) or -DHG_FACE=<name> (host builds).
set(HG_FACE "" CACHE STRING "Face to build from faces/<name>.cpp; empty for the shipped mascot")
set(HG_FACE_NAME "${HG_FACE}")
if(ESP_PLATFORM AND NOT "${CONFIG_HG_FACE}" STREQUAL "")
  set(HG_FACE_NAME "${CONFIG_HG_FACE}")
endif()
if(HG_FACE_NAME)
  set(HG_FACE_SOURCE faces/${HG_FACE_NAME}.cpp)
  if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${HG_FACE_SOURCE}")
    message(FATAL_ERROR "No face '${HG_FACE_NAME}': expected ${CMAKE_CURRENT_LIST_DIR}/${HG_FACE_SOURCE}")
  endif()
else()
  set(HG_FACE_SOURCE src/mascot_data.cpp)
endif()

set(HG_CORE_SOURCES
    src/app.cpp
    src/app_settings.cpp
    src/app_power.cpp
    src/canvas.cpp
    src/crypto.cpp
    src/font5x7.cpp
    src/json.cpp
    ${HG_FACE_SOURCE}
    src/protocol.cpp
    src/qr.cpp
    src/setup.cpp
    src/touch.cpp
    src/ui.cpp
    src/vad.cpp
)

if(ESP_PLATFORM)
  idf_component_register(SRCS ${HG_CORE_SOURCES} INCLUDE_DIRS include)
  return()
endif()

add_library(hg_core STATIC ${HG_CORE_SOURCES})
target_include_directories(hg_core PUBLIC include)
target_compile_features(hg_core PUBLIC cxx_std_17)
set_target_properties(hg_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(hg_core PRIVATE /W4 /utf-8)
else()
  target_compile_options(hg_core PRIVATE -Wall -Wextra -fno-exceptions -fno-rtti)
endif()
