cmake_minimum_required(VERSION 3.10)
project(ImageProcessing)

# 设置OpenCV和Qt的安装路径
set(CMAKE_PREFIX_PATH /opt/homebrew/opt/opencv /opt/homebrew/opt/qt)

find_package(OpenCV REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS} ${Python3_INCLUDE_DIRS} include)

add_executable(ImageProcessing src/main.cpp src/image_utils.cpp)
target_link_libraries(ImageProcessing ${OpenCV_LIBS} Qt6::Widgets Python3::Python)