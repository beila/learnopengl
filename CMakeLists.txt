cmake_minimum_required(VERSION 3.19)

set(HUNTER_PACKAGES glfw)

include(FetchContent)
FetchContent_Declare(SetupHunter GIT_REPOSITORY https://github.com/cpp-pm/gate)
FetchContent_MakeAvailable(SetupHunter)

project(learnopengl)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 REQUIRED)

add_subdirectory(glad)

add_executable(learnopengl main.cpp)
target_link_libraries(learnopengl PRIVATE glfw glad)