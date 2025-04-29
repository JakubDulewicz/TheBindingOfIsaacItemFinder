#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "winmm.lib") 
#include <opencv2/opencv.hpp>
#include "IsaacMovementController.h"
#include "ObjectFinder.h"

static const char* GAME_WINDOW_TITLE = "Binding of Isaac: Repentance";

int main()
{
	Sleep(1000);
	IsaacMovementController controller;
	ObjectFinder objectFinder;

	if (controller.FocusGame())
	{
		Sleep(500);
		controller.PressEscape();
		Sleep(100);

		while (true)
		{
			controller.MoveIsaacToShop();
			bool result = objectFinder.findObjectInGame(0.95f);
			if (result)
			{
				mciSendStringA("open \"foundSound.mp3\" type mpegvideo alias foundSound", NULL, 0, NULL);
				mciSendStringA("play foundSound wait", NULL, 0, NULL);
				mciSendStringA("close foundSound", NULL, 0, NULL);
				std::cerr << "DZIALLLAAAAA!!!!!";
				return 0;
			}

			controller.ResetGame();
			Sleep(500);
		}
;	}
}
