#pragma once
#include <Windows.h>
#include <string>

class IsaacMovementController
{
public:
	IsaacMovementController(const std::string& windowtitle = "Binding of Isaac: Repentance");
	bool FocusGame();
	void PressEscape();
	void PressW(int pressTimeMs);
	void PressA(int pressTimeMs);
	void MoveIsaacToShop();
	void ResetGame();

private:
	std::string _windowTitle;
	HWND        _hGameWnd;    
};

