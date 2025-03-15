#include "IsaacMovementController.h"
#include <iostream>

IsaacMovementController::IsaacMovementController(const std::string& windowtitle)
	: _windowTitle(windowtitle), _hGameWnd(nullptr)
{
	_hGameWnd = FindWindowA(NULL, _windowTitle.c_str());
	if (!_hGameWnd)
		std::cerr << "Nie odnaleziono okna gry o tytule: " << _windowTitle << std::endl;
}

bool IsaacMovementController::FocusGame()
{
    if (!_hGameWnd)
    {
        std::cerr << "Brak uchwytu do okna gry.\n";
        return false;
    }

    // Przywróæ okno, jeœli jest zminimalizowane
    ShowWindow(_hGameWnd, SW_SHOWNORMAL);
    SetWindowPos(_hGameWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    
    if (!SetForegroundWindow(_hGameWnd))
    {
        std::cerr << "SetForegroundWindow nie powiodlo sie.\n";
        return false;
    }

    return true;
}

void IsaacMovementController::PressEscape()
{
    PostMessage(_hGameWnd, WM_KEYDOWN, VK_ESCAPE, 0);
    Sleep(20);
    PostMessage(_hGameWnd, WM_KEYUP, VK_ESCAPE, 0);
}

void IsaacMovementController::PressW(int pressTimeMs)
{
    PostMessage(_hGameWnd, WM_KEYDOWN, 'W', 0);
    Sleep(pressTimeMs);
    PostMessage(_hGameWnd, WM_KEYUP, 'W', 0);
}

void IsaacMovementController::PressA(int pressTimeMs)
{
    PostMessage(_hGameWnd, WM_KEYDOWN, 'A', 0);
    Sleep(pressTimeMs);
    PostMessage(_hGameWnd, WM_KEYUP, 'A', 0);
}

void IsaacMovementController::MoveIsaacToShop()
{
    PressW(1100);
    Sleep(100);
    PressA(850); //TODO Adjust for diffrent character;
}

void IsaacMovementController::ResetGame()
{
    PostMessage(_hGameWnd, WM_KEYDOWN, 'R', 0);
    Sleep(2300);
    PostMessage(_hGameWnd, WM_KEYUP, 'R', 0);
}
