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

# 安装目标
install(TARGETS ImageProcessing DESTINATION bin)

# 安装依赖库
install(DIRECTORY /opt/homebrew/opt/qt/lib/QtWidgets.framework DESTINATION lib)
install(DIRECTORY /opt/homebrew/opt/python@3.13/Frameworks/Python.framework/Versions/3.13/include/python3.13 DESTINATION include)

# 安装头文件
install(DIRECTORY include/ DESTINATION include)

# CPack配置
include(CPack)
set(CPACK_PACKAGE_NAME "ImageProcessing")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "2438745244@qq.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Image Processing Tool")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-Source")
set(CPACK_SOURCE_IGNORE_FILES "/build/;/\\\\.git/;/\\\\.svn/;/\\\\.hg/;/\\\\.bzr/;/_CPack_Packages/;\\\\.swp$;\\\\.#;/#\\\\.")