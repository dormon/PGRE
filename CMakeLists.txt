cmake_minimum_required(VERSION 3.18)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1 ) 

project(PGRE)

option(SDL_SHARED "" OFF)
option(SDL_STATIC "" ON)
add_subdirectory(libs/SDL2-2.0.12)

add_executable(${PROJECT_NAME} main.cpp)

add_subdirectory(libs/glm-0.9.9.8)

add_subdirectory(libs/geGL)

target_link_libraries(${PROJECT_NAME} SDL2-static SDL2main geGL::geGL glm)
