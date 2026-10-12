#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

// 將座標限制在圖片的合法範圍內
int clampIndex(int value, int upper){
    return std::clamp(value, 0, upper - 1);
}

// 依照相鄰四個像素自行計算雙線性插值，不呼叫 OpenCV resize
cv::Vec3b sampleBilinear(const cv::Mat& image, double x, double y){
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    double dx = x - x0;
    double dy = y - y0;
    int x1 = clampIndex(x0 + 1, image.cols);
    int y1 = clampIndex(y0 + 1, image.rows);
    x0 = clampIndex(x0, image.cols);
    y0 = clampIndex(y0, image.rows);

    cv::Vec3b result;
    for(int c = 0; c < 3; c++){
        double a = image.at<cv::Vec3b>(y0, x0)[c];
        double b = image.at<cv::Vec3b>(y0, x1)[c];
        double d = image.at<cv::Vec3b>(y1, x0)[c];
        double e = image.at<cv::Vec3b>(y1, x1)[c];
        double value = (1.0 - dx) * (1.0 - dy) * a
                     + dx * (1.0 - dy) * b
                     + (1.0 - dx) * dy * d
                     + dx * dy * e;
        result[c] = cv::saturate_cast<uchar>(value);
    }
    return result;
}

// Keys cubic convolution 的權重函數，採常用參數 a = -0.5
// 每個方向取四個像素，合計取 4 x 4 = 16 個鄰近像素
// 參考：Keys, Cubic Convolution Interpolation for Digital Image Processing (1981)
double cubicWeight(double t){
    constexpr double a = -0.5;
    t = std::abs(t);
    if(t < 1.0){
        return (a + 2.0) * t * t * t - (a + 3.0) * t * t + 1.0;
    }
    if(t < 2.0){
        return a * t * t * t - 5.0 * a * t * t + 8.0 * a * t - 4.0 * a;
    }
    return 0.0;
}

// 自行實作雙三次插值，影像邊界採用複製最邊緣像素的處理方式
cv::Vec3b sampleBicubic(const cv::Mat& image, double x, double y){
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    cv::Vec3d total(0.0, 0.0, 0.0);
    for(int j = -1; j <= 2; j++){
        int sy = clampIndex(y0 + j, image.rows);
        double wy = cubicWeight(y - (y0 + j));
        for(int i = -1; i <= 2; i++){
            int sx = clampIndex(x0 + i, image.cols);
            double w = wy * cubicWeight(x - (x0 + i));
            cv::Vec3b pixel = image.at<cv::Vec3b>(sy, sx);
            for(int c = 0; c < 3; c++){
                total[c] += w * pixel[c];
            }
        }
    }
    return cv::Vec3b(cv::saturate_cast<uchar>(total[0]),
                     cv::saturate_cast<uchar>(total[1]),
                     cv::saturate_cast<uchar>(total[2]));
}

// 以目標像素中心反推來源座標，避免整數與非整數倍率時的偏移
cv::Mat scaleImage(const cv::Mat& source, double factor, bool bicubic){
    int width = std::max(1, static_cast<int>(std::lround(source.cols * factor)));
    int height = std::max(1, static_cast<int>(std::lround(source.rows * factor)));
    cv::Mat result(height, width, CV_8UC3);
    for(int y = 0; y < height; y++){
        double sy = (y + 0.5) / factor - 0.5;
        for(int x = 0; x < width; x++){
            double sx = (x + 0.5) / factor - 0.5;
            result.at<cv::Vec3b>(y, x) = bicubic
                ? sampleBicubic(source, sx, sy)
                : sampleBilinear(source, sx, sy);
        }
    }
    return result;
}

// Brown-Conrady 徑向模型：rd = ru * (1 + k1 * ru^2)
// 使用反向映射，對輸出影像各點以牛頓法求對應的來源半徑
// 使用半對角線正規化，使圖片最外角的標準化半徑約為 1
cv::Mat distortImage(const cv::Mat& source, double k1){
    cv::Mat result(source.size(), CV_8UC3, cv::Scalar(0, 0, 0));
    double cx = (source.cols - 1) / 2.0;
    double cy = (source.rows - 1) / 2.0;
    double radiusScale = std::hypot(cx, cy);
    for(int y = 0; y < result.rows; y++){
        for(int x = 0; x < result.cols; x++){
            double xd = (x - cx) / radiusScale;
            double yd = (y - cy) / radiusScale;
            double rd = std::hypot(xd, yd);
            double ru = rd;
            for(int iter = 0; iter < 25 && rd > 1e-12; iter++){
                double f = ru * (1.0 + k1 * ru * ru) - rd;
                double derivative = 1.0 + 3.0 * k1 * ru * ru;
                if(std::abs(derivative) < 1e-8){
                    break;
                }
                double step = f / derivative;
                ru -= step;
                if(std::abs(step) < 1e-10){
                    break;
                }
            }
            double ratio = rd > 1e-12 ? ru / rd : 1.0;
            double sx = cx + xd * ratio * radiusScale;
            double sy = cy + yd * ratio * radiusScale;
            if(sx >= 0.0 && sx <= source.cols - 1 &&
               sy >= 0.0 && sy <= source.rows - 1){
                result.at<cv::Vec3b>(y, x) = sampleBilinear(source, sx, sy);
            }
        }
    }
    return result;
}

int main(int argc, char* argv[]){
    // 可直接執行 HW2.exe，或帶入照片路徑：HW2.exe selfie.jpeg
    std::string inputPath = argc > 1 ? argv[1] : "selfie.jpg";
    fs::path outputDir = argc > 2 ? fs::path(argv[2]) : fs::path("outputs");
    fs::create_directories(outputDir);
    cv::Mat source = cv::imread(inputPath, cv::IMREAD_COLOR);
    if(source.empty()){
        std::cerr << "Cannot open image: " << inputPath << "\n";
        return 1;
    }
    if(source.cols < 512 || source.rows < 512){
        std::cerr << "Warning: assignment requests at least 512 x 512 pixels.\n";
    }

    // Exercise 1：先縮成 0.23 倍，再將縮小結果放大 7 倍
    cv::Mat smallLinear = scaleImage(source, 0.23, false);
    cv::Mat smallCubic = scaleImage(source, 0.23, true);
    cv::Mat largeLinear = scaleImage(smallLinear, 7.0, false);
    cv::Mat largeCubic = scaleImage(smallCubic, 7.0, true);

    // Exercise 2：負 k1 為 barrel；正 k1 為 pincushion，切向係數 p1=p2=0
    cv::Mat barrel = distortImage(source, -0.14);
    cv::Mat pincushion = distortImage(source, 0.25);

    auto save = [&](const std::string& name, const cv::Mat& image){
        fs::path file = outputDir / name;
        if(!cv::imwrite(file.string(), image)){
            std::cerr << "Failed to write: " << file << "\n";
            return false;
        }
        std::cout << file << ": " << image.cols << " x " << image.rows << "\n";
        return true;
    };
    bool ok = true;
    ok &= save("00_original.png", source);
    ok &= save("01_bilinear_down_023.png", smallLinear);
    ok &= save("02_bicubic_down_023.png", smallCubic);
    ok &= save("03_bilinear_up_7.png", largeLinear);
    ok &= save("04_bicubic_up_7.png", largeCubic);
    ok &= save("05_barrel_k-014.png", barrel);
    ok &= save("06_pincushion_k+025.png", pincushion);
    std::cout << "Done. Change input to selfie.jpeg after taking the photo.\n";
    return ok ? 0 : 2;
}
