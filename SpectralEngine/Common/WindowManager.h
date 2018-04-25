#pragma once

#include <Windows.h>
#include <string>

class WindowManager
{
public:
	WindowManager(HINSTANCE appInstance, const std::wstring& caption);
	virtual ~WindowManager();

	WindowManager(const WindowManager& copy) = delete;
	WindowManager& operator=(const WindowManager& rhs) = delete;

	// For now, a single method will be responsible for creating
	// and presenting the window (to focus on more important things)
	bool InitWindow(int width, int height, WNDPROC wndProc);
	void GetDimensions(int& width, int& height);
	void SetDimensions(int width, int height);
	//void DestroyWindow();

	HWND getHandle();

protected:
	HINSTANCE mhAppInstance;
	HWND mhMainWindow;
	bool mMinimized;
	bool mMaximized;
	bool mFullScreen;

	int mWindowWidth;
	int mWindowHeight;
	std::wstring mWindowCaption;
};

