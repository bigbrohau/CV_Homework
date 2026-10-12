#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

/*
 * NTU CSIE - DC & CV Lab - Homework 1
 *
 * Input:
 *   lena.bmp
 *
 * Output:
 *   (a) lena_upside_down.bmp
 *   (b) lena_right_side_left.bmp
 *   (c) lena_diagonal_flip.bmp
 */

 // (a) upside-down
cv::Mat upsideDown(const cv::Mat& src)
{
    cv::Mat dst(src.rows, src.cols, src.type());

    for (int row = 0; row < src.rows; ++row) {
        for (int col = 0; col < src.cols; ++col) {

            dst.at<cv::Vec3b>(src.rows - 1 - row, col) =
                src.at<cv::Vec3b>(row, col);
        }
    }

    return dst;
}


// (b) right-side-left
cv::Mat rightSideLeft(const cv::Mat& src)
{
    cv::Mat dst(src.rows, src.cols, src.type());

    for (int row = 0; row < src.rows; ++row) {
        for (int col = 0; col < src.cols; ++col) {

            dst.at<cv::Vec3b>(row, src.cols - 1 - col) =
                src.at<cv::Vec3b>(row, col);
        }
    }

    return dst;
}


// (c) diagonal flip
cv::Mat diagonalFlip(const cv::Mat& src)
{
    cv::Mat dst(src.cols, src.rows, src.type());

    for (int row = 0; row < src.rows; ++row) {
        for (int col = 0; col < src.cols; ++col) {

            dst.at<cv::Vec3b>(col, row) =
                src.at<cv::Vec3b>(row, col);
        }
    }

    return dst;
}


int main(int argc, char* argv[])
{
    std::string inputFile = "lena.bmp";

    if (argc >= 2) {
        inputFile = argv[1];
    }

    cv::Mat src = cv::imread(inputFile, cv::IMREAD_COLOR);

    if (src.empty()) {
        std::cerr << "Error: cannot open image: "
            << inputFile << std::endl;

        return 1;
    }

    cv::Mat imageA = upsideDown(src);
    cv::Mat imageB = rightSideLeft(src);
    cv::Mat imageC = diagonalFlip(src);

    cv::imwrite("(a) lena_upside_down.bmp", imageA);
    cv::imwrite("(b) lena_right_side_left.bmp", imageB);
    cv::imwrite("(c) lena_diagonal_flip.bmp", imageC);

    std::cout << "Homework 1 completed." << std::endl;
    std::cout << "(a) lena_upside_down.bmp" << std::endl;
    std::cout << "(b) lena_right_side_left.bmp" << std::endl;
    std::cout << "(c) lena_diagonal_flip.bmp" << std::endl;

    return 0;
}