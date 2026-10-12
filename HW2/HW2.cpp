#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <direct.h>
using namespace std;
using namespace cv;


// 先用一個 struct 記錄 connected component 的資料
// 後面畫框框跟重心會用到
struct Component
{
    int area;

    int minRow;
    int maxRow;
    int minCol;
    int maxCol;

    double centerRow;
    double centerCol;
};


int main() {
    cout << "Program started" << endl;

    // 先看目前程式是在哪一個資料夾執行，lena.bmp 會從這裡找，輸出的圖片也會存在這裡
    char path[1024];

    if (_getcwd(path, sizeof(path)) != NULL) {
        cout << "Working directory:" << endl;
        cout << path << endl;
    }

    cout << endl;



    // ==================================================
    // 讀 lena.bmp
    // ==================================================

    // 因為這次作業都是處理灰階，所以直接用灰階讀進來
    Mat img = imread("lena.bmp", IMREAD_GRAYSCALE);

    if (img.empty()) {
        cout << "Cannot open lena.bmp" << endl;
        cout << "Please check the working directory." << endl;
        cout << endl;
        cout << "Press Enter to exit..." << endl;
        cin.get();

        return -1;
    }

    cout << "lena.bmp loaded successfully" << endl;
    cout << "Image size = "
        << img.cols << " x " << img.rows << endl;
    cout << endl;



    // ==================================================
    // (a) Binary Image
    // threshold = 128
    // ==================================================

    cout << "Start part (a)..." << endl;


    // 建立一張跟原圖一樣大的圖
    Mat binary(img.rows, img.cols, CV_8UC1);

    // 一個 pixel 一個 pixel 看
    for (int row = 0; row < img.rows; row++) {
        for (int col = 0; col < img.cols; col++) {
            int pixel = img.at<uchar>(row, col);


            // 老師規定：0~127 是黑色、128~255 是白色
            if (pixel >= 128) {
                binary.at<uchar>(row, col) = 255;
            }
            else {
                binary.at<uchar>(row, col) = 0;
            }
        }
    }


    // 把 binary image 存起來
    bool binarySaved = imwrite("binary.bmp", binary);
    if (binarySaved) {
        cout << "(a) binary.bmp finished" << endl;
    }
    else {
        cout << "Failed to save binary.bmp" << endl;
    }

    cout << endl;



    // ==================================================
    // (b) Histogram
    // ==================================================

    cout << "Start part (b)..." << endl;


    // 灰階總共有 0~255，所以需要 256 格
    int histogram[256];

    // 一開始全部清成 0
    for (int i = 0; i < 256; i++) {
        histogram[i] = 0;
    }


    // 掃過原始圖片，如果看到灰階 100，就 histogram[100] + 1
    for (int row = 0; row < img.rows; row++) {
        for (int col = 0; col < img.cols; col++) {
            int pixel = img.at<uchar>(row, col);

            histogram[pixel]++;
        }
    }


    cout << "Histogram counting finished" << endl;


    // 順便把 histogram 數字輸出成 txt
    // 主要是方便檢查數值
    ofstream file("histogram.txt");
    for (int i = 0; i < 256; i++) {
        file << i << " "
            << histogram[i]
            << endl;
    }
    file.close();
    cout << "histogram.txt finished" << endl;



    // 接下來畫 histogram
    int histHeight = 400;
    int histWidth = 512;

    // 先建立白底圖片
    Mat histImg(histHeight, histWidth, CV_8UC1, Scalar(255));


    // 因為 histogram 的值可能比圖片高度還大，所以先找 histogram 裡最大的值
    int maxValue = 0;


    for (int i = 0; i < 256; i++) {
        if (histogram[i] > maxValue) {
            maxValue = histogram[i];
        }
    }


    cout << "Maximum histogram count = "
        << maxValue << endl;

    // 256 個灰階值，圖片寬 512，所以每個灰階值畫 2 pixel 寬
    for (int i = 0; i < 256; i++) {
        int height = (double)histogram[i] / maxValue * (histHeight - 1);

        // histogram 從下面往上畫
        for (int y = histHeight - 1; y >= histHeight - height; y--) {
            histImg.at<uchar>(y, i * 2) = 0;
            histImg.at<uchar>(y, i * 2 + 1) = 0;
        }
    }


    bool histogramSaved =
        imwrite("histogram.bmp", histImg);

    if (histogramSaved) {
        cout << "(b) histogram.bmp finished" << endl;
    }
    else {
        cout << "Failed to save histogram.bmp" << endl;
    }

    cout << endl;


    // ===========================================
    // (c) Connected Components
    // ==================================================

    cout << "Start part (c)..." << endl;

    // visited 是拿來記這個 pixel 有沒有找過，不然 BFS 會一直重複走
    Mat visited = Mat::zeros(
        binary.rows,
        binary.cols,
        CV_8UC1
    );

    vector<Component> components;


    // 我這次用 4-connected 上、下、左、右
    int dr[4] = { -1, 1, 0, 0 };
    int dc[4] = { 0, 0, -1, 1 };

    // 從左上開始把整張 binary image 掃過一次
    for (int row = 0; row < binary.rows; row++) {
        for (int col = 0; col < binary.cols; col++) {
            // 如果是白色，而且之前沒有走過，就代表可能找到一個新的 connected component
            if (binary.at<uchar>(row, col) == 255 && visited.at<uchar>(row, col) == 0) {

                queue<Point> q;


                // Point 裡面是 (x, y)，所以這裡是 (col, row)
                q.push(Point(col, row));

                // 先標記成走過
                visited.at<uchar>(row, col) = 1;

                // 這些等等用來算 component 的資料
                int area = 0;
                long long rowSum = 0;
                long long colSum = 0;
                int minRow = row;
                int maxRow = row;
                int minCol = col;
                int maxCol = col;



                // ------------------------------
                // 開始 BFS
                // ------------------------------

                // queue 還有東西就繼續找
                while (!q.empty()) {
                    Point p = q.front();
                    q.pop();

                    int nowCol = p.x;
                    int nowRow = p.y;

                    // 每找到一個 pixel，面積就 +1
                    area++;

                    // 記錄 row 和 col 的總和，最後拿來算重心
                    rowSum += nowRow;
                    colSum += nowCol;


                    // 順便找 component 最外面的範圍，最後可以拿來做 bounding box
                    if (nowRow < minRow) {
                        minRow = nowRow;
                    }

                    if (nowRow > maxRow) {
                        maxRow = nowRow;
                    }

                    if (nowCol < minCol) {
                        minCol = nowCol;
                    }

                    if (nowCol > maxCol) {
                        maxCol = nowCol;
                    }



                    // 看這個 pixel 的上下左右
                    for (int k = 0; k < 4; k++) {
                        int nextRow = nowRow + dr[k];
                        int nextCol = nowCol + dc[k];



                        // 先確認不要跑到圖片外面
                        if (nextRow < 0 || nextRow >= binary.rows || nextCol < 0 || nextCol >= binary.cols) {
                            continue;
                        }

                        // 如果旁邊也是白色而且之前沒有走過就放進 queue
                        if (binary.at<uchar>(nextRow, nextCol) == 255 && visited.at<uchar>(nextRow, nextCol) == 0) {
                            visited.at<uchar>(nextRow, nextCol) = 1;

                            q.push(
                                Point(nextCol, nextRow)
                            );
                        }
                    }
                }

                // 老師規定小於 500 pixels 的區域不要
                if (area >= 500)
                {
                    Component temp;

                    temp.area = area;
                    temp.minRow = minRow;
                    temp.maxRow = maxRow;
                    temp.minCol = minCol;
                    temp.maxCol = maxCol;



                    // 重心是所有 pixel 座標的平均，不是 bounding box 的正中間
                    temp.centerRow = (double)rowSum / area;
                    temp.centerCol = (double)colSum / area;
                    components.push_back(temp);
                }
            }
        }
    }

    cout << "Connected component search finished" << endl;
    cout << endl;

    // 顯示找到的 component 資料
    cout << "Connected components:" << endl;
    cout << endl;

    for (int i = 0; i < (int)components.size(); i++)
    {
        cout << "Component "
            << i + 1
            << endl;


        cout << "Area = "
            << components[i].area
            << endl;


        cout << "Centroid = ("
            << components[i].centerCol
            << ", "
            << components[i].centerRow
            << ")"
            << endl;


        cout << "Bounding box = ("
            << components[i].minCol
            << ", "
            << components[i].minRow
            << ") ~ ("
            << components[i].maxCol
            << ", "
            << components[i].maxRow
            << ")"
            << endl;


        cout << endl;
    }

    cout << "Total = "
        << components.size()
        << endl;


    // FAQ 說正確應該要得到 5 個
    if (components.size() == 5)
    {
        cout << "The number of components is correct (5)." << endl;
    }
    else
    {
        cout << "Warning: expected 5 components." << endl;
    }

    cout << endl;




    // 畫 bounding box 跟 centroid
    cout << "Drawing bounding boxes..." << endl;

    //Mat result = binary.clone();
    Mat result;
    cvtColor(binary, result, COLOR_GRAY2BGR);
    for (int i = 0; i < (int)components.size(); i++)
    {
        Component c = components[i];

        // 這部分老師 FAQ 說可以用 OpenCV library 畫
        //rectangle(result,Point(c.minCol,c.minRow),Point(c.maxCol,c.maxRow),Scalar(128),2);
        //藍色
        rectangle(result, Point(c.minCol, c.minRow), Point(c.maxCol, c.maxRow), Scalar(255, 0, 0), 2);


        // 重心座標本來是 double
        // 畫圖前先轉成 int
        int centerRow =
            (int)(c.centerRow + 0.5);
        int centerCol =
            (int)(c.centerCol + 0.5);


        // 畫一個 + 號
        int size = 8;


        // 橫線 //Scalar(0, 0, 255) //Scalar(128)
        line(result, Point(centerCol - size, centerRow), Point(centerCol + size, centerRow), Scalar(0, 0, 255), 2);
        // 直線
        line(result, Point(centerCol, centerRow - size), Point(centerCol, centerRow + size), Scalar(0, 0, 255), 2);
    }

    bool componentSaved =
        imwrite(
            "connected_components.bmp",
            result
        );


    if (componentSaved)
    {
        cout << "(c) connected_components.bmp finished"
            << endl;
    }
    else
    {
        cout << "Failed to save connected_components.bmp"
            << endl;
    }


    cout << endl;




    // 顯示三個結果
    cout << "Opening image windows..." << endl;


    imshow(
        "Binary Image",
        binary
    );

    imshow(
        "Histogram",
        histImg
    );
    imshow(
        "Connected Components",
        result
    );


    cout << "Three windows should be opened now." << endl;
    cout << "Press any key inside an image window to finish."
        << endl;



    // 等待鍵盤，不然程式跑完圖片視窗就會馬上關掉
    waitKey(0);



    cout << endl;
    cout << "Program finished." << endl;


    return 0;
}
