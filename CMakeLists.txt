cmake_minimum_required(VERSION 3.20)

project(nomi_eye_simulator LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)

add_subdirectory(third_party/lvgl)

add_executable(nomi_eye_simulator
    src/main.c
    src/nomi_eyes.c
)

target_include_directories(nomi_eye_simulator PRIVATE src)
target_link_libraries(nomi_eye_simulator PRIVATE lvgl user32 gdi32 shell32 ole32 uuid)
target_compile_definitions(nomi_eye_simulator PRIVATE UNICODE _UNICODE)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(nomi_eye_simulator PRIVATE -Wall -Wextra -Wpedantic)
endif()

