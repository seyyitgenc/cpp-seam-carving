cmake_minimum_required(VERSION 3.5)
project(seam_carving)


add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC vendor)
