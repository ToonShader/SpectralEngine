#pragma once

#include <Windows.h>
#include <string>

class WindowManager
{
public:
	WindowManager(HINSTANCE appInstance, const std::wstring& caption);
	~WindowManager();

	// For now, a single method will be responsible for creating
	// and presenting the window (to focus on more important things)
	bool InitWindow(int width, int height, WNDPROC wndProc);
	//void DestroyWindow();

	HWND getHandle();

protected:
	HINSTANCE mhAppInstance;
	HWND mhMainWindow;
	bool mMinimized;
	bool mMaximized;
	//bool mResizing; // NOTE: REAL-TIME: Will resizing be fluid or static?
	bool mFullScreen;

	int mWindowWidth;
	int mWindowHeight;
	std::wstring mWindowCaption;
};

