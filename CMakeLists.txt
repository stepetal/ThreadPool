cmake_minimum_required(VERSION 3.5)

project(ThreadPoolCmakeApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(ThreadPoolCmakeApp
               main.cpp
               mainwindow.cpp
               taskstablemodel.cpp
               threadpool.cpp
               images.qrc
               )

target_link_libraries(ThreadPoolCmakeApp PRIVATE Qt5::Widgets)
