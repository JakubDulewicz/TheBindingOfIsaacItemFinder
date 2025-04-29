#pragma once
#include <opencv2/opencv.hpp>
#include <Windows.h>
#include <string>
#include <vector>
#include<iostream>
class ObjectFinder
{
public:

	explicit ObjectFinder(const std::string& windowtitle = "Binding of Isaac: Repentance");
	bool findObjectInGame(double ratioThresh = 0.7);

private:
	cv::Mat captureScreen();
    cv::Mat captureFullScreen();

    std::string _windowTitle;
    HWND _hGameWnd;

    std::string _templatePath;
    cv::Mat _templateImg;
    cv::Mat _descriptorsTemplate;
    std::vector<cv::KeyPoint> _keypointsTemplate;

    bool _isTemplateLoaded;
    bool _isWindowFound;     

};

