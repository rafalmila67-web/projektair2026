cmake_minimum_required(VERSION 3.15)

project(MojaGraSFML)

add_executable(${PROJECT_NAME} main.cpp

    Game.h

    Entities.h
    GameObject.h
    Entities.cpp
    Game.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE "C:/SFML/SFML-2.5.0/include")
target_link_directories(${PROJECT_NAME} PRIVATE "C:/SFML/SFML-2.5.0/lib")
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-window sfml-system)