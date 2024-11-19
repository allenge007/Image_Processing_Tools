#include <QApplication>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QSlider>
#include <QLineEdit>
#include <QStyleFactory>
#include <opencv2/opencv.hpp>
#include <fstream>
#include "image_utils.h"
#include <filesystem>

cv::Mat currentImage;
cv::Mat originalImage;
cv::Mat processedImage;

int blurValue = 0;
int saturationValue = 0;
int contrastValue = 0;
int sharpenValue = 0;
bool isGrayscale = false;
bool isResize = false;
int resizeWidth = 100;
int resizeHeight = 100;

void displayImage(QLabel* label, const cv::Mat& image, QWidget* window) {
    cv::Mat rgb;
    cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);

    // 获取QLabel的尺寸
    QSize labelSize = label->size();
    int labelWidth = labelSize.width();
    int labelHeight = labelSize.height();

    // 计算目标尺寸，保持宽高比
    double aspectRatio = static_cast<double>(image.cols) / image.rows;
    int targetWidth, targetHeight;
    if (labelWidth / aspectRatio <= labelHeight) {
        targetWidth = labelWidth;
        targetHeight = static_cast<int>(labelWidth / aspectRatio);
    } else {
        targetHeight = labelHeight;
        targetWidth = static_cast<int>(labelHeight * aspectRatio);
    }

    // 调整图像大小以适应QLabel
    cv::Mat resizedImage;
    cv::resize(rgb, resizedImage, cv::Size(targetWidth, targetHeight), 0, 0, cv::INTER_AREA);

    QImage qimg(resizedImage.data, resizedImage.cols, resizedImage.rows, resizedImage.step, QImage::Format_RGB888);
    label->setPixmap(QPixmap::fromImage(qimg));
    label->setScaledContents(false); // 保持比例

    // 调整窗口高度以适应图像高度
    window->resize(window->width(), targetHeight + 200); // 200是按钮和日志框的高度
}

void logMessage(QTextEdit* log, const QString& message) {
    log->append(message);
    log->ensureCursorVisible();

    // 限制显示最近的3条命令
    QStringList lines = log->toPlainText().split("\n");
    if (lines.size() > 3) {
        log->clear();
        for (int i = lines.size() - 3; i < lines.size(); ++i) {
            log->append(lines[i]);
        }
    }
}

void applyImageProcessing(QLabel* processedLabel, QWidget* window) {
    if (currentImage.empty()) {
        return;
    }

    processedImage = currentImage.clone();

    // 应用高斯模糊
    if (blurValue > 0) {
        int kernelSize = blurValue * 2 + 1; // 确保kernelSize是奇数
        cv::GaussianBlur(processedImage, processedImage, cv::Size(kernelSize, kernelSize), 0);
    }

    // 应用饱和度
    if (saturationValue != 0) {
        cv::Mat hsv;
        cv::cvtColor(processedImage, hsv, cv::COLOR_BGR2HSV);
        std::vector<cv::Mat> channels;
        cv::split(hsv, channels);
        channels[1] += saturationValue;
        cv::merge(channels, hsv);
        cv::cvtColor(hsv, processedImage, cv::COLOR_HSV2BGR);
    }

    // 应用对比度
    if (contrastValue != 0) {
        processedImage.convertTo(processedImage, -1, 1 + contrastValue / 50.0, 0);
    }

    // 应用锐化
    if (sharpenValue != 0) {
        cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
            0, -sharpenValue / 10.0, 0,
            -sharpenValue / 10.0, 1 + 4 * sharpenValue / 10.0, -sharpenValue / 10.0,
            0, -sharpenValue / 10.0, 0);
        cv::filter2D(processedImage, processedImage, processedImage.depth(), kernel);
    }

    // 转换为灰度图像
    if (isGrayscale) {
        cv::cvtColor(processedImage, processedImage, cv::COLOR_BGR2GRAY);
        cv::cvtColor(processedImage, processedImage, cv::COLOR_GRAY2BGR); // 转换回BGR以便显示
    }

    // 改变图像尺寸
    if (isResize) {
        cv::resize(processedImage, processedImage, cv::Size(resizeWidth, resizeHeight));
    }

    displayImage(processedLabel, processedImage, window);
}

void onSelectImage(QLabel* originalLabel, QLabel* processedLabel, QTextEdit* log, QWidget* window) {
    QString fileName = QFileDialog::getOpenFileName(nullptr, "Select Image", "", "Images (*.png *.ppm *.jpg *.jpeg)");
    if (fileName.isEmpty()) {
        return;
    }

    currentImage = readImage(fileName.toStdString());
    if (currentImage.empty()) {
        logMessage(log, "Unable to open or find image");
        return;
    }

    originalImage = currentImage.clone(); // 保存原始图像的副本
    processedImage = currentImage.clone(); // 初始化处理后的图像
    displayImage(originalLabel, currentImage, window);
    applyImageProcessing(processedLabel, window);
    logMessage(log, "Image loaded: " + fileName);
}

void onConvertToGrayscale(QLabel* processedLabel, QTextEdit* log, QWidget* window) {
    if (currentImage.empty()) {
        logMessage(log, "Please select an image first");
        return;
    }

    isGrayscale = !isGrayscale;
    logMessage(log, isGrayscale ? "Grayscale conversion enabled" : "Grayscale conversion disabled");
    applyImageProcessing(processedLabel, window);
}

void onResizeImage(QLabel* processedLabel, QTextEdit* log, QWidget* window, QLineEdit* widthInput, QLineEdit* heightInput) {
    if (currentImage.empty()) {
        logMessage(log, "Please select an image first");
        return;
    }

    bool widthOk, heightOk;
    int width = widthInput->text().toInt(&widthOk);
    int height = heightInput->text().toInt(&heightOk);

    if (!widthOk || !heightOk || width <= 0 || height <= 0) {
        logMessage(log, "Invalid width or height");
        return;
    }

    isResize = true;
    resizeWidth = width;
    resizeHeight = height;
    logMessage(log, "Image resize set to " + QString::number(width) + "x" + QString::number(height));
    applyImageProcessing(processedLabel, window);
}

void onCompressImage(QLabel* processedLabel, QTextEdit* log, QWidget* window) {
    if (currentImage.empty()) {
        logMessage(log, "Please select an image first");
        return;
    }

    // 获取原图像大小
    int originalSize = currentImage.total() * currentImage.elemSize();

    // 压缩图像
    std::vector<uchar> compressedData;
    std::vector<int> compressionParams = {cv::IMWRITE_JPEG_QUALITY, 100}; // 设置JPEG压缩质量为90
    cv::imencode(".jpg", currentImage, compressedData, compressionParams);
    int compressedSize = compressedData.size();

    // 计算压缩率
    double compressionRate = static_cast<double>(compressedSize) / originalSize * 100;

    logMessage(log, "Image compressed, original size: " + QString::number(originalSize) + " bytes, compressed size: " + QString::number(compressedSize) + " bytes, compression rate: " + QString::number(compressionRate, 'f', 2) + "%");

    // 让用户选择保存压缩图像的位置
    QString savePath = QFileDialog::getSaveFileName(nullptr, "Save Compressed Image", "", "Images (*.jpg)");
    if (savePath.isEmpty()) {
        return;
    }

    // 保存压缩后的图像
    std::ofstream outFile(savePath.toStdString(), std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());
    outFile.close();

    logMessage(log, "Compressed image saved to: " + savePath);
}

void onSaveImage(QTextEdit* log) {
    if (processedImage.empty()) {
        logMessage(log, "No processed image to save");
        return;
    }

    // 让用户选择保存图像的位置
    QString savePath = QFileDialog::getSaveFileName(nullptr, "Save Image", "", "Images (*.png *.jpg *.jpeg *.ppm)");
    if (savePath.isEmpty()) {
        return;
    }

    // 保存处理后的图像
    if (savePath.endsWith(".ppm")) {
        writePPM(savePath.toStdString(), processedImage);
    } else {
        cv::imwrite(savePath.toStdString(), processedImage);
    }
    logMessage(log, "Image saved to: " + savePath);
}

void onGaussianBlur(int value, QLabel* processedLabel, QTextEdit* log, QWidget* window) {
    if (currentImage.empty()) {
        logMessage(log, "Please select an image first");
        return;
    }

    blurValue = value;
    logMessage(log, "Gaussian blur intensity set to " + QString::number(value * 5) + "%");
    applyImageProcessing(processedLabel, window);
}

void onSaturationChange(int value, QLabel* processedLabel, QTextEdit* log, QWidget* window) {
    if (currentImage.empty()) {
        logMessage(log, "Please select an image first");
        return;
    }

    saturationValue = value;
    logMessage(log, "Saturation set to " + QString::number(value));
    applyImageProcessing(processedLabel, window);
}

void onContrastChange(int value, QLabel* processedLabel, QTextEdit* log, QWidget* window) {
    if (currentImage.empty()) {
        logMessage(log, "Please select an image first");
        return;
    }

    contrastValue = value;
    logMessage(log, "Contrast set to " + QString::number(value));
    applyImageProcessing(processedLabel, window);
}

void onSharpenChange(int value, QLabel* processedLabel, QTextEdit* log, QWidget* window) {
    if (currentImage.empty()) {
        logMessage(log, "Please select an image first");
        return;
    }

    sharpenValue = value;
    logMessage(log, "Sharpen set to " + QString::number(value));
    applyImageProcessing(processedLabel, window);
}

void onRestoreImage(QLabel* processedLabel, QTextEdit* log, QWidget* window, QSlider* blurSlider, QSlider* saturationSlider, QSlider* contrastSlider, QSlider* sharpenSlider, QLineEdit* widthInput, QLineEdit* heightInput) {
    if (originalImage.empty()) {
        logMessage(log, "No original image to restore");
        return;
    }

    // 重置所有处理参数
    blurValue = 0;
    isGrayscale = false;
    isResize = false;
    contrastValue = 0;
    saturationValue = 0;
    sharpenValue = 0;
    resizeWidth = 100;
    resizeHeight = 100;

    // 重置滑块和输入框
    blurSlider->setValue(0);
    saturationSlider->setValue(0);
    contrastSlider->setValue(0);
    sharpenSlider->setValue(0);

    widthInput->setText("100");
    heightInput->setText("100");

    currentImage = originalImage.clone();
    processedImage = originalImage.clone();
    applyImageProcessing(processedLabel, window);
    logMessage(log, "Image restored to original");
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // 设置macOS风格
    QApplication::setStyle(QStyleFactory::create("macos"));

    QWidget window;
    window.setWindowTitle("Image Processing");
    window.resize(1200, 800); // 设置窗口初始大小

    // 设置样式表
    app.setStyleSheet(R"(
        QWidget {
            background-color: #f0f0f0;
            font-family: Arial, sans-serif;
        }
        QPushButton {
            background-color: #007BFF;
            color: white;
            border: none;
            padding: 10px 20px;
            text-align: center;
            text-decoration: none;
            font-size: 16px;
            margin: 4px 2px;
            border-radius: 12px;
        }
        QPushButton:hover {
            background-color: #A9A9A9; /* 灰色 */
            color: white;
            border: none;
        }
        QLabel {
            font-size: 18px;
            color: #333;
        }
        QTextEdit {
            background-color: #fff;
            border: 1px solid #ccc;
            padding: 10px;
            font-size: 16px;
            font-family: Arial, sans-serif;
            color: black;
        }
        QLineEdit {
            padding: 5px;
            font-size: 16px;
            border: 1px solid #ccc;
            border-radius: 5px;
            color: black;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(&window);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* selectButton = new QPushButton("Select Image");
    QPushButton* grayscaleButton = new QPushButton("Convert to Grayscale");
    QPushButton* resizeButton = new QPushButton("Resize Image");
    QPushButton* compressButton = new QPushButton("Compress Image");
    QPushButton* restoreButton = new QPushButton("Restore Image");
    QPushButton* saveButton = new QPushButton("Save Image");

    buttonLayout->addWidget(selectButton);
    buttonLayout->addWidget(grayscaleButton);
    buttonLayout->addWidget(resizeButton);
    buttonLayout->addWidget(compressButton);
    buttonLayout->addWidget(restoreButton);
    buttonLayout->addWidget(saveButton);

    QHBoxLayout* resizeLayout = new QHBoxLayout();
    QLabel* widthLabel = new QLabel("Width:");
    QLineEdit* widthInput = new QLineEdit();
    widthInput->setText("100"); // 设置默认值为100
    QLabel* heightLabel = new QLabel("Height:");
    QLineEdit* heightInput = new QLineEdit();
    heightInput->setText("100"); // 设置默认值为100
    resizeLayout->addWidget(widthLabel);
    resizeLayout->addWidget(widthInput);
    resizeLayout->addWidget(heightLabel);
    resizeLayout->addWidget(heightInput);

    QHBoxLayout* imageLayout = new QHBoxLayout();
    QLabel* originalLabel = new QLabel("Original Image");
    QLabel* processedLabel = new QLabel("Processed Image");
    originalLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // 使标签扩展以适应窗口大小
    originalLabel->setAlignment(Qt::AlignCenter); // 居中显示图像
    processedLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // 使标签扩展以适应窗口大小
    processedLabel->setAlignment(Qt::AlignCenter); // 居中显示图像

    imageLayout->addWidget(originalLabel);
    imageLayout->addWidget(processedLabel);

    QHBoxLayout* sliderLayout = new QHBoxLayout();

    QVBoxLayout* blurLayout = new QVBoxLayout();
    QSlider* blurSlider = new QSlider(Qt::Horizontal);
    blurSlider->setRange(0, 20); // 设置滑块范围
    blurSlider->setValue(0); // 初始值为0
    QLabel* blurLabel = new QLabel("Gaussian Blur Intensity: 0%");
    blurLayout->addWidget(blurLabel);
    blurLayout->addWidget(blurSlider);

    QVBoxLayout* saturationLayout = new QVBoxLayout();
    QSlider* saturationSlider = new QSlider(Qt::Horizontal);
    saturationSlider->setRange(-100, 100); // 设置滑块范围
    saturationSlider->setValue(0); // 初始值为0
    QLabel* saturationLabel = new QLabel("Saturation: 0");
    saturationLayout->addWidget(saturationLabel);
    saturationLayout->addWidget(saturationSlider);

    QVBoxLayout* contrastLayout = new QVBoxLayout();
    QSlider* contrastSlider = new QSlider(Qt::Horizontal);
    contrastSlider->setRange(-100, 100); // 设置滑块范围
    contrastSlider->setValue(0); // 初始值为0
    QLabel* contrastLabel = new QLabel("Contrast: 0");
    contrastLayout->addWidget(contrastLabel);
    contrastLayout->addWidget(contrastSlider);

    QVBoxLayout* sharpenLayout = new QVBoxLayout();
    QSlider* sharpenSlider = new QSlider(Qt::Horizontal);
    sharpenSlider->setRange(0, 20); // 设置滑块范围
    sharpenSlider->setValue(0); // 初始值为0
    QLabel* sharpenLabel = new QLabel("Sharpen: 0");
    sharpenLayout->addWidget(sharpenLabel);
    sharpenLayout->addWidget(sharpenSlider);

    sliderLayout->addLayout(blurLayout);
    sliderLayout->addLayout(saturationLayout);
    sliderLayout->addLayout(contrastLayout);
    sliderLayout->addLayout(sharpenLayout);

    QTextEdit* log = new QTextEdit();
    log->setReadOnly(true); // 设置日志为只读
    log->setMaximumHeight(100); // 限制日志框的最大高度

    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(resizeLayout);
    mainLayout->addLayout(sliderLayout);
    mainLayout->addLayout(imageLayout);
    mainLayout->addWidget(log);

    QObject::connect(selectButton, &QPushButton::clicked, [&]() { onSelectImage(originalLabel, processedLabel, log, &window); });
    QObject::connect(grayscaleButton, &QPushButton::clicked, [&]() { onConvertToGrayscale(processedLabel, log, &window); });
    QObject::connect(resizeButton, &QPushButton::clicked, [&]() { onResizeImage(processedLabel, log, &window, widthInput, heightInput); });
    QObject::connect(compressButton, &QPushButton::clicked, [&]() { onCompressImage(processedLabel, log, &window); });
    QObject::connect(restoreButton, &QPushButton::clicked, [&]() { onRestoreImage(processedLabel, log, &window, blurSlider, saturationSlider, contrastSlider, sharpenSlider, widthInput, heightInput); });
    QObject::connect(saveButton, &QPushButton::clicked, [&]() { onSaveImage(log); });
    QObject::connect(blurSlider, &QSlider::valueChanged, [&]() {
        int value = blurSlider->value();
        blurLabel->setText(QString("Gaussian Blur Intensity: %1%").arg(value * 5)); // 假设最大值为100%
        onGaussianBlur(value, processedLabel, log, &window);
    });
    QObject::connect(saturationSlider, &QSlider::valueChanged, [&]() {
        int value = saturationSlider->value();
        saturationLabel->setText(QString("Saturation: %1").arg(value));
        onSaturationChange(value, processedLabel, log, &window);
    });
    QObject::connect(contrastSlider, &QSlider::valueChanged, [&]() {
        int value = contrastSlider->value();
        contrastLabel->setText(QString("Contrast: %1").arg(value));
        onContrastChange(value, processedLabel, log, &window);
    });
    QObject::connect(sharpenSlider, &QSlider::valueChanged, [&]() {
        int value = sharpenSlider->value();
        sharpenLabel->setText(QString("Sharpen: %1").arg(value));
        onSharpenChange(value, processedLabel, log, &window);
    });
    window.setLayout(mainLayout);
    window.show();

    return app.exec();
}