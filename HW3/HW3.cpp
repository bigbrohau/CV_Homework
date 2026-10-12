#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <string>

using namespace cv;
using namespace std;

const int GRAY_LEVELS = 256;

// ------------------------------------------------------------
// Manually calculate the histogram.
// OpenCV histogram functions are NOT used.
// ------------------------------------------------------------
void calculateHistogram(const Mat& image, int histogram[GRAY_LEVELS])
{
    for (int i = 0; i < GRAY_LEVELS; i++)
        histogram[i] = 0;

    for (int row = 0; row < image.rows; row++)
    {
        for (int col = 0; col < image.cols; col++)
        {
            int gray = image.at<uchar>(row, col);
            histogram[gray]++;
        }
    }
}

// ------------------------------------------------------------
// Save histogram values for checking / report use.
// ------------------------------------------------------------
void saveHistogramCSV(const int histogram[GRAY_LEVELS], const string& filename)
{
    ofstream fout(filename.c_str());

    if (!fout.is_open())
    {
        cout << "Cannot write " << filename << endl;
        return;
    }

    fout << "Intensity,Count\n";
    for (int i = 0; i < GRAY_LEVELS; i++)
        fout << i << "," << histogram[i] << "\n";

    fout.close();
}

// ------------------------------------------------------------
// Draw histogram manually into an image buffer.
// No cv::calcHist(), normalize(), rectangle(), line(), etc.
// OpenCV is only used here as the image container and for imwrite().
// ------------------------------------------------------------
Mat drawHistogram(const int histogram[GRAY_LEVELS])
{
    const int width = 800;
    const int height = 450;
    const int leftMargin = 35;
    const int rightMargin = 15;
    const int topMargin = 15;
    const int bottomMargin = 30;

    const int plotWidth = width - leftMargin - rightMargin;
    const int plotHeight = height - topMargin - bottomMargin;
    const int baseline = topMargin + plotHeight;

    Mat histImage(height, width, CV_8UC1, Scalar(255));

    int maxCount = 0;
    for (int i = 0; i < GRAY_LEVELS; i++)
    {
        if (histogram[i] > maxCount)
            maxCount = histogram[i];
    }

    // y-axis
    for (int y = topMargin; y <= baseline; y++)
        histImage.at<uchar>(y, leftMargin) = 0;

    // x-axis
    for (int x = leftMargin; x < width - rightMargin; x++)
        histImage.at<uchar>(baseline, x) = 0;

    if (maxCount == 0)
        return histImage;

    // Draw 256 histogram bars.
    for (int i = 0; i < GRAY_LEVELS; i++)
    {
        int xStart = leftMargin + (i * plotWidth) / GRAY_LEVELS;
        int xEnd = leftMargin + ((i + 1) * plotWidth) / GRAY_LEVELS - 1;

        if (xEnd < xStart)
            xEnd = xStart;

        int barHeight = static_cast<int>(
            static_cast<double>(histogram[i]) * (plotHeight - 1) / maxCount
            );

        int yTop = baseline - barHeight;

        for (int x = xStart; x <= xEnd && x < width - rightMargin; x++)
        {
            for (int y = baseline - 1; y >= yTop && y >= topMargin; y--)
                histImage.at<uchar>(y, x) = 0;
        }
    }

    // Small tick marks at 0, 64, 128, 192, 255.
    const int ticks[5] = { 0, 64, 128, 192, 255 };
    for (int t = 0; t < 5; t++)
    {
        int x = leftMargin + (ticks[t] * plotWidth) / 255;
        for (int y = baseline; y < baseline + 6 && y < height; y++)
            histImage.at<uchar>(y, x) = 0;
    }

    return histImage;
}

// ------------------------------------------------------------
// Find minimum and maximum gray values manually.
// ------------------------------------------------------------
void findGrayRange(const Mat& image, int& minValue, int& maxValue)
{
    minValue = 255;
    maxValue = 0;

    for (int row = 0; row < image.rows; row++)
    {
        for (int col = 0; col < image.cols; col++)
        {
            int value = image.at<uchar>(row, col);

            if (value < minValue)
                minValue = value;
            if (value > maxValue)
                maxValue = value;
        }
    }
}

int main()
{
    // Put lena.bmp in the Eclipse project working directory.
    Mat original = imread("lena.bmp", IMREAD_GRAYSCALE);

    if (original.empty())
    {
        cout << "Cannot open lena.bmp" << endl;
        cout << "Please put lena.bmp in the project working directory." << endl;
        return -1;
    }

    cout << "Input image: " << original.cols << " x " << original.rows << endl;

    // ========================================================
    // (a) Original image and its histogram
    // ========================================================
    int histA[GRAY_LEVELS];
    calculateHistogram(original, histA);

    Mat histImageA = drawHistogram(histA);

    imwrite("hw3_a_original.bmp", original);
    imwrite("hw3_a_histogram.bmp", histImageA);
    saveHistogramCSV(histA, "hw3_a_histogram.csv");

    // ========================================================
    // (b) Image with intensity divided by 3 and histogram
    // ========================================================
    Mat divided(original.rows, original.cols, CV_8UC1);

    for (int row = 0; row < original.rows; row++)
    {
        for (int col = 0; col < original.cols; col++)
        {
            int value = original.at<uchar>(row, col);
            divided.at<uchar>(row, col) = static_cast<uchar>(value / 3);
        }
    }

    int histB[GRAY_LEVELS];
    calculateHistogram(divided, histB);

    Mat histImageB = drawHistogram(histB);

    imwrite("hw3_b_divided_by_3.bmp", divided);
    imwrite("hw3_b_histogram.bmp", histImageB);
    saveHistogramCSV(histB, "hw3_b_histogram.csv");

    // ========================================================
    // (c) Histogram equalization applied to (b)
    //     s(r_k) = (L - 1) * sum_{j=0}^{k} p_r(r_j)
    // ========================================================
    const int totalPixels = divided.rows * divided.cols;

    double probability[GRAY_LEVELS];
    double cdf[GRAY_LEVELS];
    int mapping[GRAY_LEVELS];

    // Normalized histogram p_r(r_k) = n_k / (M*N)
    for (int i = 0; i < GRAY_LEVELS; i++)
        probability[i] = static_cast<double>(histB[i]) / totalPixels;

    // Discrete CDF
    cdf[0] = probability[0];
    for (int i = 1; i < GRAY_LEVELS; i++)
        cdf[i] = cdf[i - 1] + probability[i];

    // Build the intensity mapping table.
    for (int i = 0; i < GRAY_LEVELS; i++)
    {
        int newValue = static_cast<int>(255.0 * cdf[i] + 0.5);

        if (newValue < 0)
            newValue = 0;
        if (newValue > 255)
            newValue = 255;

        mapping[i] = newValue;
    }

    // Apply the mapping to every pixel.
    Mat equalized(divided.rows, divided.cols, CV_8UC1);

    for (int row = 0; row < divided.rows; row++)
    {
        for (int col = 0; col < divided.cols; col++)
        {
            int oldValue = divided.at<uchar>(row, col);
            equalized.at<uchar>(row, col) = static_cast<uchar>(mapping[oldValue]);
        }
    }

    int histC[GRAY_LEVELS];
    calculateHistogram(equalized, histC);

    Mat histImageC = drawHistogram(histC);

    imwrite("hw3_c_equalized.bmp", equalized);
    imwrite("hw3_c_histogram.bmp", histImageC);
    saveHistogramCSV(histC, "hw3_c_histogram.csv");

    // ========================================================
    // Simple result check printed in Eclipse console
    // ========================================================
    int minA, maxA, minB, maxB, minC, maxC;
    findGrayRange(original, minA, maxA);
    findGrayRange(divided, minB, maxB);
    findGrayRange(equalized, minC, maxC);

    cout << endl;
    cout << "Gray-level range" << endl;
    cout << "(a) original      : " << minA << " ~ " << maxA << endl;
    cout << "(b) divided by 3  : " << minB << " ~ " << maxB << endl;
    cout << "(c) equalized     : " << minC << " ~ " << maxC << endl;

    cout << endl;
    cout << "Output files:" << endl;
    cout << "  hw3_a_original.bmp" << endl;
    cout << "  hw3_a_histogram.bmp" << endl;
    cout << "  hw3_b_divided_by_3.bmp" << endl;
    cout << "  hw3_b_histogram.bmp" << endl;
    cout << "  hw3_c_equalized.bmp" << endl;
    cout << "  hw3_c_histogram.bmp" << endl;
    cout << "  hw3_a_histogram.csv" << endl;
    cout << "  hw3_b_histogram.csv" << endl;
    cout << "  hw3_c_histogram.csv" << endl;

    return 0;
}
