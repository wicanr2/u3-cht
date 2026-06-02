cmake_minimum_required(VERSION 3.16)
project(ultima3_cht C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

set(U3COMPAT_SDL_INCLUDES ${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})

# --- P0 smoke: hello-SDL，驗證工具鏈 ---
add_executable(hello_sdl tests/hello_sdl.c)
target_include_directories(hello_sdl PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(hello_sdl PRIVATE ${SDL2_LIBRARIES})

# --- compat 層 (QuickDraw/Mac shim) ---
add_library(u3compat STATIC
    src/compat/qd_geometry.c
    src/compat/qd_port.c
    src/compat/qd_draw.c
    src/compat/qd_text.c
)
target_include_directories(u3compat PUBLIC src/compat ${U3COMPAT_SDL_INCLUDES})
target_link_libraries(u3compat PUBLIC ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})

# --- compat 單元測試 ---
add_executable(test_geometry tests/test_geometry.c)
target_link_libraries(test_geometry PRIVATE u3compat)
enable_testing()
add_test(NAME geometry COMMAND test_geometry)

add_executable(test_qd_blit tests/test_qd_blit.c)
target_include_directories(test_qd_blit PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_libraries(test_qd_blit PRIVATE u3compat ${SDL2_IMAGE_LIBRARIES})

add_executable(test_qd_text tests/test_qd_text.c)
target_include_directories(test_qd_text PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_libraries(test_qd_text PRIVATE u3compat ${SDL2_IMAGE_LIBRARIES})

# --- 字串表讀取器 ---
add_library(u3text STATIC src/text/strings.c)
target_include_directories(u3text PUBLIC src/text)

add_executable(test_strings_render tests/test_strings_render.c)
target_include_directories(test_strings_render PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_libraries(test_strings_render PRIVATE u3compat u3text ${SDL2_IMAGE_LIBRARIES})

# --- P1 垂直切片 PoC ---
add_executable(poc
    src/poc/main.c
)
target_include_directories(poc PRIVATE
    ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
target_link_libraries(poc PRIVATE
    ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_TTF_LIBRARIES})
