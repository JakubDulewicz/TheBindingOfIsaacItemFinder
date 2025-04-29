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
    // 0) Wczytaj wzorzec i przygotuj ma³y template 95×95
    cv::Mat templ = cv::imread("d20.png", cv::IMREAD_GRAYSCALE);
    if (templ.empty()) {
        std::cerr << "Nie udalo sie wczytac d20.png\n";
        return false;
    }
    cv::Mat templSmall;
    cv::resize(templ, templSmall, cv::Size(95, 95));

    // 1) Wczytaj screen z pliku i skonwertuj do szaroœci
    cv::Mat screenBGR = captureFullScreen();
    if (screenBGR.empty()) {
        std::cerr << "Nie udalo sie wczytac screen.png\n";
        return false;
    }
    cv::Mat screenGray;
    cv::cvtColor(screenBGR, screenGray, cv::COLOR_BGR2GRAY);

    // 2) Oblicz SIFT-owe cechy i deskryptory dla small template
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create(40000);
    std::vector<cv::KeyPoint> keypointsTempl, keypointsScene;
    cv::Mat descriptorsTempl, descriptorsScene;
    sift->detectAndCompute(templSmall, cv::noArray(),
        keypointsTempl, descriptorsTempl);
    if (descriptorsTempl.empty()) {
        std::cerr << "Brak deskryptorow we wzorcu.\n";
        return false;
    }

    // 3) Oblicz SIFT dla sceny
    sift->detectAndCompute(screenGray, cv::noArray(),
        keypointsScene, descriptorsScene);
    if (descriptorsScene.empty()) {
        std::cerr << "Brak deskryptorow w scenie.\n";
        return false;
    }

    // 4) Dopasowanie deskryptorow (K=2, BFMatcher L2)
    cv::BFMatcher matcher(cv::NORM_L2);
    std::vector<std::vector<cv::DMatch>> knnMatches;
    matcher.knnMatch(descriptorsTempl, descriptorsScene, knnMatches, 2);

    // 5) Ratio test
    std::vector<cv::DMatch> goodMatches;
    for (auto& m : knnMatches) {
        if (m.size() == 2 && m[0].distance < ratioThresh * m[1].distance)
            goodMatches.push_back(m[0]);
    }
    std::cout << "[ObjectFinder] Dobre dopasowania: "
        << goodMatches.size() << std::endl;
    if (goodMatches.size() < 40) {
        std::cerr << "Zbyt malo dopasowan.\n";
        return false;
    }

    // 6) Homografia
    std::vector<cv::Point2f> ptsObj, ptsScene;
    for (auto& d : goodMatches) 
    {
        ptsObj.push_back(keypointsTempl[d.queryIdx].pt);
        ptsScene.push_back(keypointsScene[d.trainIdx].pt);
    }
    cv::Mat H = cv::findHomography(ptsObj, ptsScene, cv::RANSAC);
    if (H.empty()) 
    {
        std::cerr << "Nie udalo sie obliczyc homografii.\n";
        return false;
    }

    // 7) Rysowanie obrysu 95×95
    std::vector<cv::Point2f> objCorners = {
        {0,0}, {95,0}, {95,95}, {0,95}
    }, sceneCorners(4);
    cv::perspectiveTransform(objCorners, sceneCorners, H);

    cv::Mat vis = screenBGR.clone();
    for (int i = 0; i < 4; ++i) 
    {
        cv::line(vis, sceneCorners[i],
            sceneCorners[(i + 1) % 4], cv::Scalar(0, 255, 0), 2);
    }

    // 8) Zapis i wyœwietlenie
    static int cnt = 0;
    cv::imwrite("detected_" + std::to_string(cnt++) + ".png", vis);
    cv::imshow("Detected (SIFT)", vis);
    cv::waitKey(1);

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

cv::Mat ObjectFinder::captureFullScreen()
{
    if (!_hGameWnd) return {};

    // 1) Ustal monitor, na którym jest okno
    HMONITOR hMon = MonitorFromWindow(_hGameWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);

    int x = mi.rcMonitor.left;
    int y = mi.rcMonitor.top;
    int width = mi.rcMonitor.right - mi.rcMonitor.left;
    int height = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // 2) DC ca³ego ekranu
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ oldBmp = SelectObject(hMemDC, hBitmap);

    // 3) BitBlt z regionu monitora
    BitBlt(hMemDC, 0, 0, width, height, hScreenDC, x, y, SRCCOPY);
    ReleaseDC(NULL, hScreenDC);

    // 4) Konwersja do cv::Mat (jak wy¿ej)
    BITMAP bmp;  GetObject(hBitmap, sizeof(bmp), &bmp);
    cv::Mat tmp(bmp.bmHeight, bmp.bmWidth, CV_8UC4);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = bmp.bmWidth;
    bmi.bmiHeader.biHeight = -bmp.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    GetDIBits(hMemDC, hBitmap, 0, bmp.bmHeight, tmp.data, &bmi, DIB_RGB_COLORS);

    SelectObject(hMemDC, oldBmp);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);

    cv::Mat result;
    cv::cvtColor(tmp, result, cv::COLOR_BGRA2BGR);
    return result;
}
