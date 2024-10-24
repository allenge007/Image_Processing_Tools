#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

cv::Mat readImage(const std::string& path);
cv::Mat readPPM(const std::string& path);
void writePPM(const std::string& path, const cv::Mat& image);
cv::Mat convertToGrayscale(const cv::Mat& image);
cv::Mat resizeImage(const cv::Mat& image, int width, int height);
std::vector<uchar> compressImage(const cv::Mat& image);

#endif // IMAGE_UTILS_H