#include <iostream>
#include <windows.h>
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#include <opencv2/opencv.hpp>

cv::Mat CaptureScreen()
{
    // Pobierz rozmiar ekranu
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Kontekst urządzenia dla całego ekranu
    HDC hScreenDC = GetDC(NULL);

    // Utwórz kompatybilny kontekst i bitmapę
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
    HGDIOBJ hOld = SelectObject(hMemoryDC, hBitmap);

    // Skopiuj ekran do hMemoryDC
    BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

    // Zwolnij DC ekranu
    ReleaseDC(NULL, hScreenDC);

    // Skonwertuj HBITMAP do cv::Mat
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    // bmp.bmWidth, bmp.bmHeight zawierają rozmiar w pikselach
    // bmp.bmBitsPixel -> typowo 32 bity

    // Utwórz strukturę OpenCV: 8-bitowa, 4 kanały (BGRA)
    cv::Mat matImage(bmp.bmHeight, bmp.bmWidth, CV_8UC4);

    // Skopiuj bity z HBITMAP do matImage.data
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bmp.bmWidth;
    bmi.bmiHeader.biHeight = -bmp.bmHeight; // ujemna wysokość, aby odwrócić bitmapę
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    GetDIBits(hMemoryDC, hBitmap, 0, bmp.bmHeight, matImage.data, &bmi, DIB_RGB_COLORS);

    // Przywróć stary obiekt i zwolnij zasoby GDI
    SelectObject(hMemoryDC, hOld);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);

    // Obraz jest w formacie BGRA; jeśli potrzebujemy BGR, można przekonwertować:
    cv::Mat matScreen;
    cv::cvtColor(matImage, matScreen, cv::COLOR_BGRA2BGR);

    return matScreen;
}

int main()
{
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    cv::Mat screen = CaptureScreen();
    cv::imwrite("screen_capture.png", screen);

    // Można też wyświetlić w oknie OpenCV:
    cv::imshow("Captured Screen", screen);
    cv::waitKey(0);
}
