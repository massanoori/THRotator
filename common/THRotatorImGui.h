// (c) 2017 massanoori

#pragma once

#include <imgui.h>

#ifdef TOUHOU_ON_D3D8
using THRotatorImGui_D3DDeviceInterface = IDirect3DDevice8;
#else
using THRotatorImGui_D3DDeviceInterface = IDirect3DDevice9;
#endif

bool THRotatorImGui_Initialize(HWND hWnd, THRotatorImGui_D3DDeviceInterface* device);
void THRotatorImGui_Shutdown();
void THRotatorImGui_NewFrame();
void THRotatorImGui_RenderDrawLists(ImDrawData* drawData);
void THRotatorImGui_EndFrame();

void THRotatorImGui_InvalidateDeviceObjects();
bool THRotatorImGui_CreateDeviceObjects();
LRESULT THRotatorImGui_WindowProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
