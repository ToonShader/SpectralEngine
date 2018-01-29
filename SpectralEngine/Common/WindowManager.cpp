#include "WindowManager.h"
#include <cassert>

WindowManager::WindowManager(HINSTANCE appInstance, const std::wstring& caption)
	: mhAppInstance(appInstance),
	mWindowCaption(caption)
{
	assert(appInstance != nullptr);
}

WindowManager::~WindowManager()
{
	//DestroyWindow();
}

bool WindowManager::InitWindow(int width, int height, WNDPROC wndProc)
{
	mWindowWidth = width;
	mWindowHeight = height;

	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = wndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = mhAppInstance;
	wcex.hIcon = LoadIcon(mhAppInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // (HBRUSH)GetStockObject(NULL_BRUSH);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = mWindowCaption.c_str();
	wcex.hIconSm = LoadIcon(mhAppInstance, IDI_APPLICATION);
	assert(RegisterClassEx(&wcex) != 0);

	// Create window
	RECT rect = { 0, 0, mWindowWidth, mWindowHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

	mhMainWindow = CreateWindow(mWindowCaption.c_str(), mWindowCaption.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, mhAppInstance, nullptr);

	if (!mhMainWindow)
	{
		MessageBox(nullptr, L"CreateWindow Failed.", nullptr, 0);
		return false;
	}

	ShowWindow(mhMainWindow, SW_SHOW);
	UpdateWindow(mhMainWindow);

	return true;
}

HWND WindowManager::getHandle()
{
	return mhMainWindow;
}
