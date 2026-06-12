cmake_minimum_required(VERSION 3.10)
project(TowerDefense)

set(CMAKE_CXX_STANDARD 17)

# Szukamy domyślnego, wbudowanego w Linuksa SFML-a
find_package(SFML COMPONENTS graphics window system REQUIRED)

add_executable(TowerDefense main.cpp)

# Łączymy projekt
target_link_libraries(TowerDefense sfml-graphics sfml-window sfml-system)
