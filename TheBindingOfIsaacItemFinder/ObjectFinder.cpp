#include "ObjectFinder.h"

ObjectFinder::ObjectFinder(const std::string& windowtitle)
    : _windowTitle(windowtitle), _hGameWnd(nullptr)
{
    _hGameWnd = FindWindowA(NULL, _windowTitle.c_str());
    if (!_hGameWnd)
        std::cerr << "Nie odnaleziono okna gry o tytule: " << _windowTitle << std::endl;
}

bool ObjectFinder::findObjectInGame(double ratioThresh)
{
    // 1. Zrób zrzut ekranu okna gry
    cv::Mat screenBGR = captureScreen();
    if (screenBGR.empty())
    {
        std::cerr << "Blad przechwytywania ekranu.\n";
        return false;
    }

    // 2. Konwertuj do szaroœci, bo wzorzec mamy w szaroœci
    cv::Mat screenGray;
    cv::cvtColor(screenBGR, screenGray, cv::COLOR_BGR2GRAY);

    // 3. Wykryj cechy w obrazie ekranu
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create(20000);
    std::vector<cv::KeyPoint> keypointsScene;
    cv::Mat descriptorsScene;
    sift->detectAndCompute(screenGray, cv::noArray(),
        keypointsScene, descriptorsScene);

    if (keypointsScene.empty() || descriptorsScene.empty())
    {
        std::cerr << "Za malo cech w obrazie z ekranu.\n";
        return false;
    }

    // 4. Dopasowanie deskryptorow (K=2, BFMatcher L2)
    cv::BFMatcher matcher(cv::NORM_L2);
    std::vector<std::vector<cv::DMatch>> knnMatches;
    matcher.knnMatch(_descriptorsTemplate, descriptorsScene, knnMatches, 2);

    // 5. Filtrowanie dopasowan (ratio test)
    std::vector<cv::DMatch> goodMatches;
    for (size_t i = 0; i < knnMatches.size(); i++)
    {
        if (knnMatches[i].size() == 2)
        {
            const cv::DMatch& m1 = knnMatches[i][0];
            const cv::DMatch& m2 = knnMatches[i][1];
            if (m1.distance < ratioThresh * m2.distance)
            {
                goodMatches.push_back(m1);
            }
        }
    }

    std::cout << "[ObjectFinder] Dobre dopasowania: "
        << goodMatches.size() << std::endl;

    // (Opcjonalnie) zobacz dopasowania w obrazie:
    //  {
    //      cv::Mat imgMatches;
    //      cv::drawMatches(m_templateImg, m_keypointsTemplate,
    //                      screenGray,    keypointsScene,
    //                      goodMatches,   imgMatches,
    //                      cv::Scalar::all(-1), cv::Scalar::all(-1),
    //                      std::vector<char>(),
    //                      cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
    //      cv::imshow("Dopasowania (drawMatches)", imgMatches);
    //      cv::waitKey(1);
    //  }

    // 6. Jezeli mamy wystarczajaco duzo dopasowan, sprobujmy obliczyc homografie
    if (goodMatches.size() < 4)
    {
        std::cerr << "Zbyt malo dopasowan do wyznaczenia homografii.\n";
        return false;
    }

    std::vector<cv::Point2f> objPoints, scenePoints;
    for (auto& match : goodMatches)
    {
        objPoints.push_back(_keypointsTemplate[match.queryIdx].pt);
        scenePoints.push_back(keypointsScene[match.trainIdx].pt);
    }

    // RANSAC do obliczenia homografii
    cv::Mat H = cv::findHomography(objPoints, scenePoints, cv::RANSAC);

    if (H.empty())
    {
        std::cerr << "Nie udalo sie obliczyc homografii (H pusta).\n";
        return false;
    }

    // 7. Rysujemy obrys znalezionego obiektu - na kopii screenGray, np.
    // (Zamieniamy screenGray na BGR, by zobaczyc kolorowa linie)
    cv::Mat imgResult;
    cv::cvtColor(screenGray, imgResult, cv::COLOR_GRAY2BGR);

    // Rogi oryginalnego wzorca
    std::vector<cv::Point2f> objCorners = {
        {0.f, 0.f},
        {(float)_templateImg.cols, 0.f},
        {(float)_templateImg.cols, (float)_templateImg.rows},
        {0.f, (float)_templateImg.rows}
    };
    std::vector<cv::Point2f> sceneCorners(4);

    cv::perspectiveTransform(objCorners, sceneCorners, H);

    // Rysowanie - zielone linie
    cv::line(imgResult, sceneCorners[0], sceneCorners[1], cv::Scalar(0, 255, 0), 2);
    cv::line(imgResult, sceneCorners[1], sceneCorners[2], cv::Scalar(0, 255, 0), 2);
    cv::line(imgResult, sceneCorners[2], sceneCorners[3], cv::Scalar(0, 255, 0), 2);
    cv::line(imgResult, sceneCorners[3], sceneCorners[0], cv::Scalar(0, 255, 0), 2);

    cv::imshow("Znaleziony obiekt w oknie gry", imgResult);
    cv::waitKey(1);

    // Jesli doszlismy tu, uznajemy ze sukces
    return true;
}

cv::Mat ObjectFinder::captureScreen()
{
    if (!_hGameWnd)
    {
        std::cerr << "Brak uchwytu do okna gry.\n";
        return cv::Mat();
    }

    // Pobieramy rozmiar okna
    RECT rect;
    if (!GetWindowRect(_hGameWnd, &rect))
    {
        std::cerr << "Nie udalo sie pobrac rozmiaru okna.\n";
        return cv::Mat();
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    HDC hWindowDC = GetDC(_hGameWnd);
    if (!hWindowDC)
    {
        std::cerr << "Nie udalo sie pobrac DC okna.\n";
        return cv::Mat();
    }

    HDC hMemoryDC = CreateCompatibleDC(hWindowDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hWindowDC, width, height);
    HGDIOBJ hOld = SelectObject(hMemoryDC, hBitmap);

    if (!BitBlt(hMemoryDC, 0, 0, width, height, hWindowDC, 0, 0, SRCCOPY))
    {
        std::cerr << "BitBlt nie powiodl sie.\n";
    }

    ReleaseDC(_hGameWnd, hWindowDC);

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    cv::Mat matImage(bmp.bmHeight, bmp.bmWidth, CV_8UC4);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bmp.bmWidth;
    bmi.bmiHeader.biHeight = -bmp.bmHeight;
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
