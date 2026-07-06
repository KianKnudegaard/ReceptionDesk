cmake_minimum_required(VERSION 3.20)
project(ReceptionDesk VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3  CONFIG REQUIRED)
find_package(glad   CONFIG REQUIRED)
find_package(glm    CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

function(add_gl_target TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE
        glfw glad::glad glm::glm OpenGL::GL
    )
endfunction()

add_gl_target(ReceptionDesk  main.cpp)
add_gl_target(Pyramids       pyramids.cpp)
add_gl_target(Mesh           mesh.cpp)
add_gl_target(Scene          scene.cpp)