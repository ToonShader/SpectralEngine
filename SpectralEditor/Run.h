#pragma once

#include <concrt.h>
#include "Scene\SceneManager.h"

// Temporary connection
extern SceneManager gScene;
extern Concurrency::critical_section gUpdateLock;

void Initialize(Windows::UI::Xaml::Controls::SwapChainPanel^ panel);
void AppDraw();

void OnMouseDown(int btnState, int x, int y);
void OnMouseUp(int btnState, int x, int y);
void OnMouseMove(int btnState, int x, int y);