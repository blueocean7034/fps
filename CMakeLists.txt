cmake_minimum_required(VERSION 3.16)
project(FPSGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Bullet REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR} /usr/include ${BULLET_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} include)

file(GLOB SRC_FILES src/*.cpp)

add_executable(fps ${SRC_FILES})

target_link_libraries(fps ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${BULLET_LIBRARIES} ${GLEW_LIBRARIES})
