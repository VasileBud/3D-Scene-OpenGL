cmake_minimum_required(VERSION 4.0)
project(Project)

set(CMAKE_CXX_STANDARD 20)

include_directories(D:/Facultate/AN3/Sem1/PG/OpenGL_dev_libs/include)
link_directories(D:/Facultate/AN3/Sem1/PG/OpenGL_dev_libs/lib)

add_executable(Project main.cpp Window.cpp Shader.cpp Camera.cpp Mesh.cpp Model3D.cpp stb_image.cpp tiny_obj_loader.cpp SkyBox.cpp)

target_link_libraries(Project glfw3 glew opengl32)