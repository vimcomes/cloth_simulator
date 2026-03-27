cmake_minimum_required(VERSION 3.20)
project(cloth_sim CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)

add_executable(cloth_sim
    src/main.cpp
    src/cloth.cpp
    src/renderer.cpp
    src/interaction.cpp
)

target_include_directories(cloth_sim PRIVATE
    src/
    ${EXTERNAL_DIR}
    ${EXTERNAL_DIR}/SDL2
    ${EXTERNAL_DIR}/glm
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
)

target_link_directories(cloth_sim PRIVATE
    ${EXTERNAL_DIR}/lib
)

target_link_libraries(cloth_sim PRIVATE
    SDL2
    ${OPENGL_LIBRARIES}
    GLEW::GLEW
)

# Copy shaders next to binary
add_custom_command(TARGET cloth_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:cloth_sim>/shaders
)
