#include "image_utils.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>
#include <Python.h>

cv::Mat readImage(const std::string& path) {
    if (path.substr(path.find_last_of(".") + 1) == "ppm") {
        return readPPM(path);
    } else {
        return cv::imread(path, cv::IMREAD_COLOR);
    }
}

cv::Mat readPPM(const std::string& path) {
    // 初始化Python解释器
    Py_Initialize();

    // 嵌入Python代码
    const char* pythonCode = R"(
import sys
from PIL import Image

def convert_ppm_to_png(ppm_path, png_path):
    with Image.open(ppm_path) as img:
        img.save(png_path, 'PNG')
)";

    // 执行嵌入的Python代码
    PyRun_SimpleString(pythonCode);

    // 获取Python函数
    PyObject* pModule = PyImport_AddModule("__main__");
    PyObject* pFunc = PyObject_GetAttrString(pModule, "convert_ppm_to_png");
    if (pFunc && PyCallable_Check(pFunc)) {
        // 设置函数参数
        PyObject* pArgs = PyTuple_Pack(2, PyUnicode_FromString(path.c_str()), PyUnicode_FromString((path.substr(0, path.find_last_of(".")) + ".png").c_str()));

        // 调用Python函数
        PyObject* pValue = PyObject_CallObject(pFunc, pArgs);
        Py_DECREF(pArgs);

        if (pValue != nullptr) {
            Py_DECREF(pValue);
        } else {
            Py_DECREF(pFunc);
            PyErr_Print();
            std::cerr << "Failed to call Python function" << std::endl;
            Py_Finalize();
            return cv::Mat();
        }
    } else {
        if (PyErr_Occurred()) PyErr_Print();
        std::cerr << "Cannot find function 'convert_ppm_to_png'" << std::endl;
    }
    Py_XDECREF(pFunc);

    // 终止Python解释器
    Py_Finalize();

    // 读取PNG图像
    std::string pngPath = path.substr(0, path.find_last_of(".")) + ".png";
    return cv::imread(pngPath, cv::IMREAD_COLOR);
}

void writePPM(const std::string& path, const cv::Mat& image) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "无法写入PPM文件" << std::endl;
        return;
    }

    // PPM格式的图像数据是RGB格式，需要转换为RGB格式以便保存
    cv::Mat rgbImage;
    cv::cvtColor(image, rgbImage, cv::COLOR_BGR2RGB);

    file << "P6\n" << rgbImage.cols << " " << rgbImage.rows << "\n255\n";
    file.write(reinterpret_cast<const char*>(rgbImage.data), rgbImage.cols * rgbImage.rows * 3);
    file.close();
}

cv::Mat convertToGrayscale(const cv::Mat& image) {
    cv::Mat grayImage;
    cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);
    return grayImage;
}

cv::Mat resizeImage(const cv::Mat& image, int width, int height) {
    cv::Mat resizedImage;
    cv::resize(image, resizedImage, cv::Size(width, height));
    return resizedImage;
}

std::vector<uchar> compressImage(const cv::Mat& image) {
    std::vector<uchar> compressedData;
    // 简单的RLE压缩算法
    for (int i = 0; i < image.rows; ++i) {
        const uchar* row = image.ptr<uchar>(i);
        for (int j = 0; j < image.cols; ++j) {
            uchar value = row[j];
            int count = 1;
            while (j + 1 < image.cols && row[j + 1] == value) {
                ++count;
                ++j;
            }
            compressedData.push_back(value);
            compressedData.push_back(count);
        }
    }
    return compressedData;
}