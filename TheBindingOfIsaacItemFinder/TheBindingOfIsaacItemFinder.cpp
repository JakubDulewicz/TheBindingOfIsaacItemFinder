#include <iostream>
#include <windows.h>
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#include <opencv2/opencv.hpp>

static const char* GAME_WINDOW_TITLE = "Binding of Isaac: Repentance";
cv::Mat CaptureScreen(HWND hWnd)
{
    RECT rect;
    if (!GetWindowRect(hWnd, &rect)) {
        std::cerr << "Błąd: nie udało się pobrać rozmiaru okna.\n";
        return cv::Mat();
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    HDC hWindowDC = GetDC(hWnd);
    if (!hWindowDC) {
        std::cerr << "Błąd: nie udało się pobrać DC okna.\n";
        return cv::Mat();
    }

    HDC hMemoryDC = CreateCompatibleDC(hWindowDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hWindowDC, width, height);
    HGDIOBJ hOld = SelectObject(hMemoryDC, hBitmap);

    if (!BitBlt(hMemoryDC, 0, 0, width, height, hWindowDC, 0, 0, SRCCOPY)) {
        std::cerr << "Błąd: BitBlt nie powiódł się.\n";
    }

    ReleaseDC(hWnd, hWindowDC);

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    cv::Mat matImage(bmp.bmHeight, bmp.bmWidth, CV_8UC4);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bmp.bmWidth;
    bmi.bmiHeader.biHeight = -bmp.bmHeight;  // Ujemna wysokość -> odwrócenie pionowe
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    GetDIBits(hMemoryDC, hBitmap, 0, bmp.bmHeight,
        matImage.data, &bmi, DIB_RGB_COLORS);

    SelectObject(hMemoryDC, hOld);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);

    cv::Mat matFinal;
    cv::cvtColor(matImage, matFinal, cv::COLOR_BGRA2BGR);

    return matFinal;
}

int main()
{
    cv::Mat imgObject = cv::imread("d20.png", cv::IMREAD_GRAYSCALE);
    cv::Mat imgScene = cv::imread("scene.png", cv::IMREAD_GRAYSCALE);

    if (imgObject.empty() || imgScene.empty()) {
        std::cerr << "Blad wczytania plikow.\n";
        return -1;
    }

    cv::Ptr<cv::SIFT> sift = cv::SIFT::create(20000); 

    std::vector<cv::KeyPoint> keypointsObj, keypointsScene;
    cv::Mat descriptorsObj, descriptorsScene;
    sift->detectAndCompute(imgObject, cv::noArray(), keypointsObj, descriptorsObj);
    sift->detectAndCompute(imgScene, cv::noArray(), keypointsScene, descriptorsScene);

    if (keypointsObj.empty() || keypointsScene.empty()) {
        std::cerr << "Za malo cech w ktoryms z obrazow.\n";
        return -1;
    }

    cv::BFMatcher matcher(cv::NORM_L2);
    std::vector<std::vector<cv::DMatch>> knnMatches;
    matcher.knnMatch(descriptorsObj, descriptorsScene, knnMatches, 2);

    const float ratio_thresh = 0.70f;
    std::vector<cv::DMatch> goodMatches;
    for (size_t i = 0; i < knnMatches.size(); i++) {
        if (knnMatches[i].size() == 2) {
            const cv::DMatch& m1 = knnMatches[i][0];
            const cv::DMatch& m2 = knnMatches[i][1];
            if (m1.distance < ratio_thresh * m2.distance) {
                goodMatches.push_back(m1);
            }
        }
    }

    std::cout << "Dobre dopasowania: " << goodMatches.size() << std::endl;

    {
        cv::Mat imgMatches;
        cv::drawMatches(
            imgObject, keypointsObj,
            imgScene, keypointsScene,
            goodMatches, imgMatches,
            cv::Scalar::all(-1), // kolor linii
            cv::Scalar::all(-1), // kolor punktów
            std::vector<char>(), // maska
            cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS
        );

        cv::imshow("Dopasowania (drawMatches)", imgMatches);
        cv::imwrite("dopasowania.png", imgMatches);
        cv::waitKey(1);
    }

    if (goodMatches.size() >= 4) {
        std::vector<cv::Point2f> objPoints, scenePoints;
        for (auto& match : goodMatches) {
            objPoints.push_back(keypointsObj[match.queryIdx].pt);
            scenePoints.push_back(keypointsScene[match.trainIdx].pt);
        }

        cv::Mat H = cv::findHomography(objPoints, scenePoints, cv::RANSAC);

        if (!H.empty()) 
        {
            std::vector<cv::Point2f> objCorners = {
                {0.f, 0.f},
                {(float)imgObject.cols, 0.f},
                {(float)imgObject.cols, (float)imgObject.rows},
                {0.f, (float)imgObject.rows}
            };

            std::vector<cv::Point2f> sceneCorners(4);
            cv::perspectiveTransform(objCorners, sceneCorners, H);


            cv::Mat imgResult;
            cv::cvtColor(imgScene, imgResult, cv::COLOR_GRAY2BGR);
            cv::line(imgResult, sceneCorners[0], sceneCorners[1], cv::Scalar(0, 255, 0), 2);
            cv::line(imgResult, sceneCorners[1], sceneCorners[2], cv::Scalar(0, 255, 0), 2);
            cv::line(imgResult, sceneCorners[2], sceneCorners[3], cv::Scalar(0, 255, 0), 2);
            cv::line(imgResult, sceneCorners[3], sceneCorners[0], cv::Scalar(0, 255, 0), 2);

            cv::imshow("Wykryta kostka", imgResult);
            cv::waitKey(0);
        }
    }

    return 0;
}
