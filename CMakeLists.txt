cmake_minimum_required(VERSION 3.10)

project(std-spanstream LANGUAGES CXX)

add_library (std-spanstream INTERFACE)

target_include_directories(std-spanstream
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
