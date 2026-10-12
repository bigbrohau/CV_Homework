#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

// Threshold 128: 0~127 -> 0, 128~255 -> 255
Mat binarize128(const Mat& src){
    Mat dst(src.rows, src.cols, CV_8UC1, Scalar(0));

    for (int row = 0; row < src.rows; row++) {
        for (int col = 0; col < src.cols; col++) {
            if (src.at<uchar>(row, col) >= 128)
                dst.at<uchar>(row, col) = 255;
            else
                dst.at<uchar>(row, col) = 0;
        }
    }
    return dst;
}

// Octagonal 3-5-5-5-3 kernel
const int kernel[5][5] = {
    {0, 1, 1, 1, 0},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1},
    {0, 1, 1, 1, 0}
};

Mat dilation(const Mat& src){
    Mat dst(src.rows, src.cols, CV_8UC1, Scalar(0));

    for (int row = 0; row < src.rows; row++) {
        for (int col = 0; col < src.cols; col++) {
            bool hit = false;

            for (int kr = -2; kr <= 2 && !hit; kr++) {
                for (int kc = -2; kc <= 2; kc++) {
                    if (kernel[kr + 2][kc + 2] == 0)
                        continue;

                    int r = row + kr;
                    int c = col + kc;

                    if (r >= 0 && r < src.rows && c >= 0 && c < src.cols && src.at<uchar>(r, c) == 255) {
                        hit = true;
                        break;
                    }
                }
            }

            if (hit)
                dst.at<uchar>(row, col) = 255;
        }
    }
    return dst;
}

Mat erosion(const Mat& src){
    Mat dst(src.rows, src.cols, CV_8UC1, Scalar(0));

    for (int row = 0; row < src.rows; row++) {
        for (int col = 0; col < src.cols; col++) {
            bool fit = true;

            for (int kr = -2; kr <= 2 && fit; kr++) {
                for (int kc = -2; kc <= 2; kc++) {
                    if (kernel[kr + 2][kc + 2] == 0)
                        continue;

                    int r = row + kr;
                    int c = col + kc;

                    // Outside the image is treated as background (black)
                    if (r < 0 || r >= src.rows || c < 0 || c >= src.cols || src.at<uchar>(r, c) != 255) {
                        fit = false;
                        break;
                    }
                }
            }

            if (fit)
                dst.at<uchar>(row, col) = 255;
        }
    }
    return dst;
}

Mat opening(const Mat& src){
    return dilation(erosion(src));
}

Mat closing(const Mat& src){
    return erosion(dilation(src));
}

// Hit-and-miss for detecting upper-right corners.
// According to the textbook example used in class:
// J (foreground): center, left, below must be white.
// K (background): above, upper-right, right must be black.
Mat hitAndMissUpperRight(const Mat& src){
    Mat dst(src.rows, src.cols, CV_8UC1, Scalar(0));

    // row offset, column offset
    const int J[3][2] = {
        { 0,  0},  // center
        { 0, -1},  // left
        { 1,  0}   // below
    };

    const int K[3][2] = {
        {-1,  0},  // above
        {-1,  1},  // upper-right
        { 0,  1}   // right
    };

    for (int row = 1; row < src.rows - 1; row++) {
        for (int col = 1; col < src.cols - 1; col++) {
            bool match = true;

            // J must match foreground (white)
            for (int i = 0; i < 3; i++) {
                int r = row + J[i][0];
                int c = col + J[i][1];
                if (src.at<uchar>(r, c) != 255) {
                    match = false;
                    break;
                }
            }

            // K must match background (black)
            if (match) {
                for (int i = 0; i < 3; i++) {
                    int r = row + K[i][0];
                    int c = col + K[i][1];
                    if (src.at<uchar>(r, c) != 0) {
                        match = false;
                        break;
                    }
                }
            }

            if (match)
                dst.at<uchar>(row, col) = 255;
        }
    }
    return dst;
}

int main(){
    Mat src = imread("lena.bmp", IMREAD_GRAYSCALE);

    if (src.empty()) {
        cerr << "Cannot open lena.bmp" << endl;
        return -1;
    }

    Mat binary = binarize128(src);
    Mat imageA = dilation(binary);
    Mat imageB = erosion(binary);
    Mat imageC = opening(binary);
    Mat imageD = closing(binary);
    Mat imageE = hitAndMissUpperRight(binary);

    // Save images
    imwrite("lena_binary_128.bmp", binary);
    imwrite("(a)_dilation.bmp", imageA);
    imwrite("(b)_erosion.bmp", imageB);
    imwrite("(c)_opening.bmp", imageC);
    imwrite("(d)_closing.bmp", imageD);
    imwrite("(e)_hit_and_miss.bmp", imageE);

    cout << "HW4 completed." << endl;
    cout << "Output files:" << endl;
    cout << "  (a)_dilation.bmp" << endl;
    cout << "  (b)_erosion.bmp" << endl;
    cout << "  (c)_opening.bmp" << endl;
    cout << "  (d)_closing.bmp" << endl;
    cout << "  (e)_hit_and_miss.bmp" << endl;

    // Show results on screen
    imshow("Binary image (threshold = 128)", binary);
    imshow("(a) Dilation", imageA);
    imshow("(b) Erosion", imageB);
    imshow("(c) Opening", imageC);
    imshow("(d) Closing", imageD);
    imshow("(e) Hit-and-Miss", imageE);

    waitKey(0);
    destroyAllWindows();

    return 0;
}
