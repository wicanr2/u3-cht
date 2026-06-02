cmake_minimum_required(VERSION 3.16)
project(ultima3_cht C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# --- P0 smoke: hello-SDL，驗證工具鏈 ---
add_executable(hello_sdl tests/hello_sdl.c)
target_include_directories(hello_sdl PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(hello_sdl PRIVATE ${SDL2_LIBRARIES})

# --- P1 垂直切片 PoC ---
add_executable(poc
    src/poc/main.c
)
target_include_directories(poc PRIVATE
    ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
target_link_libraries(poc PRIVATE
    ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_TTF_LIBRARIES})
