// (c) 2017 massanoori

#include "stdafx.h"
#include "THRotatorImGui.h"
#include "THRotatorSystem.h"
#include "EncodingUtils.h"
#include "THRotatorLog.h"
#include "Direct3DUtils.h"

#include <DirectXMath.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <imstb_truetype.h>

using Microsoft::WRL::ComPtr;

namespace
{

struct THRotatorImGui_UserData
{
	ComPtr<Direct3DDeviceBase> device;
	ComPtr<Direct3DVertexBufferBase> vertexBuffer;
	ComPtr<Direct3DIndexBufferBase> indexBuffer;
	ComPtr<Direct3DTextureBase> fontTexture;
	HWND hDeviceWindow;
	LARGE_INTEGER previousTime;
	LARGE_INTEGER ticksPerSecond;
	std::int32_t vertexBufferCapacity, indexBufferCapacity;
	float cursorPositionScaleX;
	float cursorPositionScaleY;

#ifdef TOUHOU_ON_D3D8
	DWORD stateBlock;
#else
	ComPtr<IDirect3DStateBlock9> stateBlock;
#endif

	THRotatorImGui_UserData(Direct3DDeviceBase* pDevice, HWND hWnd)
		: device(pDevice)
		, hDeviceWindow(hWnd)
		, vertexBufferCapacity(5000)
		, indexBufferCapacity(10000)
		, cursorPositionScaleX(1.0f)
		, cursorPositionScaleY(1.0f)
#ifdef TOUHOU_ON_D3D8
		, stateBlock(INVALID_STATE_BLOCK_HANDLE)
#endif
	{
		QueryPerformanceFrequency(&ticksPerSecond);
		QueryPerformanceCounter(&previousTime);
	}
};

struct THRotatorImGui_Vertex
{
	float pos[3];
	DWORD col;
	float uv[2];

	enum : DWORD { FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 };
};

// Successive offsets from codepoint 0x4E00 like the way imgui_draw.cpp does.
// Generated from kanji from JMDict plus the following:
//   佶俛兪凊匱厦厲囹圀圄垠堯娥嫖嫦孚宕屨嶇徇很徙愒
//   戢挈摶擺敝旙桀椛殢汨洽涇淒渭璋睚矣砺礪秉翊翦聱
//   脯舜蔌裎裼褫襄諏跎跖蹐軼輭郢鄂醯鈇闈驤驪鷁
static const short offsetsFrom0x4E00[] =
{
	-1,0,1,3,0,0,0,0,1,0,1,0,2,1,1,0,4,2,4,3,2,4,5,0,1,0,6,0,0,2,2,1,0,0,6,0,0,0,3,0,0,17,1,10,1,1,3,1,0,1,0,1,2,0,1,0,2,0,1,0,1,2,0,1,
	0,0,1,2,0,0,0,4,6,0,4,0,2,1,0,2,0,1,1,4,0,0,0,0,0,3,0,0,3,0,0,7,0,1,1,3,4,5,7,0,2,0,0,0,0,8,1,0,17,0,3,1,1,1,1,0,5,2,0,5,0,0,0,0,
	1,1,1,1,0,0,0,0,0,10,5,0,2,1,0,4,0,2,3,2,1,2,1,1,1,6,2,1,2,0,9,1,0,0,5,0,8,2,0,0,5,3,0,0,0,5,0,1,0,1,1,0,0,1,0,0,8,0,3,1,2,1,10,0,
	2,1,1,4,1,1,1,0,0,1,2,1,1,0,0,0,1,0,0,0,1,8,2,9,3,0,0,5,3,1,0,3,1,8,6,5,1,0,0,1,4,2,4,7,3,6,0,0,17,0,4,0,0,0,1,6,3,2,4,2,1,1,3,4,
	8,1,1,5,0,6,3,1,4,0,0,1,13,1,0,2,1,3,0,1,8,7,4,2,0,0,2,0,0,1,0,0,0,0,0,0,1,0,0,0,1,3,5,1,5,2,2,1,0,0,0,3,3,0,0,0,3,3,1,2,0,2,0,1,
	0,1,1,0,2,0,0,1,6,1,1,0,0,1,1,1,3,0,0,1,0,0,0,5,6,1,2,0,0,0,0,13,0,0,2,0,4,0,1,0,2,2,0,4,1,0,0,2,0,1,2,2,0,0,1,3,2,0,3,0,5,6,0,3,
	0,3,1,2,2,0,0,0,0,0,7,0,2,2,0,0,0,6,0,0,0,3,1,5,0,0,4,4,0,1,0,0,0,7,1,3,3,0,4,4,0,7,3,0,2,5,0,0,5,2,4,4,2,1,1,1,1,8,2,2,0,3,0,0,
	2,1,1,0,10,7,3,0,1,0,2,0,1,4,1,0,4,0,0,1,0,1,3,0,1,6,6,6,0,0,0,3,0,0,1,1,0,0,0,0,0,2,3,0,0,0,2,0,1,1,3,5,2,4,0,0,0,1,0,0,2,6,2,1,
	17,1,1,4,0,4,0,1,0,3,4,0,0,6,6,0,4,0,0,0,0,0,0,5,1,0,1,1,3,1,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,2,0,0,1,6,1,0,3,0,0,0,0,0,0,0,0,0,9,1,
	0,0,0,5,4,0,0,0,7,1,0,1,0,0,0,3,3,1,0,0,2,0,2,13,8,1,6,1,1,0,0,3,0,0,2,3,1,5,1,3,3,5,7,0,2,2,0,5,0,4,4,2,0,2,2,0,0,23,3,2,0,3,0,3,
	7,9,1,0,6,1,5,24,1,1,4,5,1,3,1,8,3,2,5,1,4,23,0,1,1,2,0,2,1,0,0,12,0,0,1,0,0,0,7,0,0,0,0,0,1,1,5,13,0,1,1,10,5,1,2,3,0,4,15,3,0,7,1,0,
	10,1,0,0,2,13,5,1,0,1,1,1,8,0,9,1,4,7,8,3,1,0,0,2,4,3,1,5,5,0,2,4,4,4,6,1,2,6,1,13,3,0,0,0,4,0,0,6,1,3,2,0,2,1,2,10,1,1,0,5,0,2,2,2,
	0,6,3,2,0,1,0,1,6,7,0,4,2,15,1,2,1,2,3,0,0,0,15,2,1,2,0,12,10,8,7,8,3,1,0,0,30,2,4,2,3,0,0,7,2,0,19,0,1,1,0,1,3,1,2,0,3,0,9,3,3,3,2,5,
	4,0,0,2,0,4,5,0,8,1,4,0,1,2,2,3,2,2,4,1,9,3,4,4,15,3,4,0,1,8,0,9,6,0,2,2,3,1,2,1,1,2,3,6,2,11,0,1,1,0,0,4,1,0,0,0,0,11,0,4,4,0,0,2,
	0,1,5,2,1,1,0,0,1,0,2,5,6,5,2,0,0,4,0,0,0,2,0,1,2,5,1,2,1,0,1,3,4,0,3,4,3,0,0,0,5,2,5,2,2,9,1,2,3,12,1,2,7,2,1,4,0,1,0,4,2,8,0,0,
	2,0,15,3,1,1,13,6,3,2,0,4,3,5,5,0,5,3,0,4,2,16,7,3,3,1,18,18,7,0,23,8,0,0,2,0,6,1,0,9,0,9,1,2,2,11,19,1,0,21,2,4,1,3,1,3,7,1,0,1,12,0,2,0,
	1,0,1,1,0,1,3,0,1,2,1,4,2,1,2,1,2,2,4,1,0,0,1,0,0,1,5,1,0,0,0,0,0,0,1,2,0,0,0,0,7,1,2,0,0,0,1,0,5,2,0,0,0,0,0,4,3,1,0,0,6,1,0,0,
	1,3,0,0,0,0,1,2,4,1,0,1,1,3,0,1,0,1,1,0,0,0,0,0,2,0,1,4,3,5,1,1,3,4,3,6,0,0,0,0,0,0,0,0,0,3,0,1,1,0,1,0,0,1,1,1,8,1,0,0,1,0,2,3,
	2,1,7,18,3,16,6,0,2,6,4,32,6,0,6,4,1,0,5,4,1,9,6,7,0,3,13,34,3,24,2,2,20,2,3,34,11,1,0,11,1,0,0,5,2,4,1,0,2,1,1,0,0,0,2,2,2,0,0,0,0,1,3,1,
	0,3,0,2,5,4,4,2,0,0,1,7,5,1,1,0,2,2,0,0,4,2,3,0,1,4,7,0,9,1,0,0,14,0,0,1,1,0,0,0,0,0,0,0,1,1,0,2,2,4,5,0,0,2,1,3,5,0,3,1,7,0,0,0,
	8,0,0,4,0,0,4,0,2,6,4,0,1,0,4,0,2,7,1,0,2,0,0,2,1,2,4,0,0,0,2,0,0,1,0,0,0,0,0,2,3,5,0,0,1,3,1,1,3,1,4,0,0,11,1,1,2,1,3,1,3,3,0,3,
	2,1,0,0,2,0,1,3,1,2,2,0,0,2,0,1,0,0,0,0,0,3,1,0,3,0,0,2,1,2,6,0,0,2,0,4,0,4,3,5,0,0,3,2,0,8,0,0,0,2,0,2,10,4,4,2,2,1,1,15,2,3,2,2,
	0,2,0,3,1,0,0,0,0,3,1,8,8,7,1,2,1,2,3,0,4,2,0,0,0,2,0,0,0,0,0,1,0,4,14,4,1,0,0,4,1,1,3,0,3,0,2,2,0,1,0,7,1,1,1,3,0,7,1,9,6,1,1,2,
	1,1,3,0,7,0,1,2,0,0,0,1,5,6,0,3,0,0,2,2,4,0,3,7,12,9,3,1,2,0,1,0,0,1,6,3,0,4,0,1,0,1,1,0,2,2,1,2,0,1,0,6,3,7,3,1,0,2,1,3,9,2,1,1,
	0,1,3,3,3,3,4,3,0,0,0,5,18,2,11,3,0,0,1,1,1,1,7,1,1,0,0,1,0,0,3,3,0,2,0,2,3,3,3,0,0,1,1,3,2,3,0,0,5,0,0,1,1,5,1,2,2,2,1,2,4,5,2,4,
	2,2,2,0,2,0,4,0,6,0,0,0,0,1,0,2,0,1,12,5,3,3,2,0,7,0,0,0,0,2,0,1,0,1,0,3,0,0,1,0,0,2,0,10,0,0,2,1,1,0,0,4,1,0,1,0,4,0,0,2,4,6,0,5,
	8,2,3,5,4,2,0,0,9,2,0,1,0,4,1,4,8,1,0,0,4,3,4,2,0,7,4,0,2,2,2,3,1,2,3,0,0,0,0,1,1,0,0,0,1,5,1,6,7,0,1,2,5,0,1,3,3,0,5,1,10,5,1,3,
	18,1,4,2,10,3,1,3,0,12,3,3,14,6,14,1,5,6,1,0,0,8,4,9,0,6,3,5,0,3,1,1,0,1,1,6,1,0,4,0,2,7,4,5,1,5,0,0,0,0,1,5,2,1,0,7,2,0,1,23,10,5,0,0,
	0,2,2,1,0,1,1,1,2,0,5,7,1,1,5,1,3,4,0,2,5,3,1,1,0,1,0,9,0,3,1,4,1,0,5,1,1,0,2,1,2,0,1,3,0,0,1,0,6,1,2,0,3,3,0,4,0,2,2,4,1,1,4,1,
	2,0,0,0,0,2,0,3,8,7,3,0,4,1,0,3,0,10,0,4,1,0,4,1,4,0,5,0,5,0,7,3,2,10,6,1,0,0,0,4,0,0,3,1,3,6,5,0,0,7,4,0,5,4,3,4,2,5,4,13,1,12,2,0,
	1,0,2,8,6,1,0,0,3,0,2,0,0,0,0,2,4,0,1,1,6,0,1,3,1,1,6,0,0,1,0,0,0,0,2,2,1,3,2,8,2,4,0,0,0,1,2,2,2,1,0,0,0,0,2,4,2,0,0,1,1,1,1,4,
	1,0,7,1,1,4,4,1,0,1,1,0,2,0,0,12,3,2,0,0,0,1,7,0,5,4,0,0,1,0,3,1,2,0,3,6,2,1,0,1,0,0,0,0,3,1,2,0,2,0,0,14,2,0,6,2,7,0,7,1,3,0,2,0,
	2,0,0,0,2,1,5,1,0,1,0,4,1,0,0,1,7,3,6,1,1,0,7,1,0,0,1,9,3,0,2,0,2,1,1,0,1,0,3,0,4,1,0,0,0,0,1,0,3,0,8,3,0,0,0,1,4,2,1,0,1,4,0,2,
	3,6,3,6,0,5,4,2,2,1,0,0,2,6,0,0,0,6,4,1,5,3,1,2,3,1,2,7,8,0,0,4,2,0,1,0,0,0,0,5,0,1,0,0,2,0,1,1,0,3,0,0,1,1,7,3,2,2,0,5,0,3,6,0,
	5,2,0,1,6,2,2,1,3,3,0,0,2,2,4,0,13,0,4,4,2,5,1,1,2,2,5,3,2,0,3,1,3,1,1,1,5,0,0,9,2,0,0,2,6,0,1,0,0,1,12,0,5,1,0,28,0,3,4,3,0,1,6,4,
	0,0,4,7,0,1,4,4,2,6,0,13,1,6,0,2,0,7,0,1,15,0,8,0,9,2,3,6,2,0,1,3,10,4,1,0,2,0,8,1,2,1,4,0,10,2,0,0,1,2,0,4,3,0,3,0,1,3,3,0,1,2,0,0,
	10,11,7,2,1,1,0,0,0,0,1,2,0,0,2,0,4,5,1,0,3,1,2,0,2,3,11,0,1,0,3,20,6,0,0,1,3,11,16,0,1,0,5,1,0,0,11,1,6,2,2,0,0,0,7,1,5,1,7,2,3,1,4,3,
	3,1,0,2,2,1,5,0,8,2,2,1,4,0,1,0,0,0,0,1,2,4,0,1,8,5,1,3,0,0,1,2,1,0,4,2,23,6,4,3,2,0,5,3,0,6,0,2,2,2,1,0,2,2,0,19,0,1,6,2,2,0,1,1,
	5,2,0,12,1,0,3,1,5,0,3,1,0,18,2,2,2,3,3,5,4,5,0,5,0,7,4,3,0,0,4,1,1,1,1,0,0,9,1,0,0,0,0,7,1,3,0,0,3,0,0,1,1,0,2,1,0,0,1,8,1,3,4,6,
	2,8,1,2,11,6,0,2,11,0,0,1,9,3,5,1,3,0,1,2,7,4,2,3,0,2,2,4,1,0,5,0,4,1,0,8,0,19,1,2,0,5,0,1,0,3,2,5,1,1,0,0,10,1,0,7,0,4,0,5,6,5,11,2,
	3,2,0,2,4,8,0,1,9,6,2,1,7,8,12,5,6,1,5,6,0,1,20,2,3,0,0,2,6,3,3,2,3,3,7,2,5,1,3,2,1,0,1,0,0,6,0,4,3,13,13,4,6,18,0,2,0,7,3,0,11,0,3,3,
	6,18,0,0,0,7,0,0,0,0,12,6,9,3,1,17,7,3,11,10,4,0,1,4,4,7,1,5,5,9,2,2,1,6,0,2,6,1,1,0,1,1,4,14,6,2,2,4,4,0,3,5,8,3,4,1,10,4,4,5,1,1,1,0,
	1,8,4,0,0,4,0,7,3,1,0,2,6,20,12,1,1,3,1,2,0,3,0,0,0,0,0,0,5,0,0,3,5,3,1,0,1,0,0,1,1,0,4,1,8,1,4,3,0,1,1,4,10,13,1,9,2,0,5,11,1,1,7,1,
	1,4,1,1,5,0,6,2,0,5,3,0,3,0,12,11,0,3,0,2,5,2,0,0,0,2,5,1,7,0,4,0,9,7,11,2,1,1,5,0,0,5,1,9,2,1,1,10,18,8,0,7,4,1,5,1,2,0,15,1,4,4,2,0,
	15,4,2,2,24,2,12,0,0,0,0,3,4,1,19,3,0,0,0,1,0,0,2,6,4,3,2,7,4,2,4,18,8,3,4,12,12,4,4,7,3,1,0,0,1,2,1,2,2,0,3,12,8,0,3,3,2,1,1,1,0,3,1,0,
	1,2,4,0,0,0,3,0,1,0,16,2,1,2,4,0,1,0,2,1,2,0,3,5,2,2,0,0,6,6,0,2,0,2,0,1,0,1,5,2,5,1,5,5,0,0,1,2,0,2,0,0,1,1,2,1,5,4,1,0,2,0,1,2,
	3,0,0,4,6,1,0,0,5,1,1,2,9,1,11,6,0,0,1,1,0,5,2,3,6,6,3,0,0,2,0,5,3,1,3,4,0,1,2,1,0,1,0,2,1,3,1,1,0,0,0,0,1,0,2,1,0,6,1,2,5,20,1,3,
	3,0,0,4,2,0,2,1,1,7,4,3,0,1,0,1,1,0,0,1,2,3,3,1,3,5,2,2,2,0,0,1,0,14,2,0,0,4,0,2,10,2,0,1,1,3,6,18,0,5,1,1,0,1,2,14,3,0,11,5,12,0,0,4,
	6,0,2,2,7,0,0,1,7,5,13,0,3,1,0,1,1,1,3,0,0,3,14,9,4,0,1,0,15,0,0,8,1,1,5,10,23,10,2,0,1,0,4,7,4,5,4,0,3,1,1,1,11,3,1,5,0,9,1,1,2,3,2,1,
	0,4,0,2,5,9,2,0,3,2,2,10,3,12,2,0,6,12,3,0,0,13,1,1,1,6,0,0,6,2,2,0,2,2,0,0,0,1,2,2,4,9,7,0,0,2,0,4,2,0,0,22,3,3,1,0,7,3,0,0,0,0,7,1,
	5,0,2,2,6,1,1,0,1,0,1,3,2,10,4,2,4,2,1,0,5,2,2,1,2,0,12,0,3,4,3,0,0,1,0,1,3,6,0,0,7,9,0,0,7,4,3,1,2,0,2,1,1,1,0,3,9,0,1,0,0,0,6,0,
	8,0,3,0,6,3,4,3,0,0,2,2,1,1,5,3,2,2,0,2,1,0,3,2,1,6,2,0,2,1,4,1,1,1,0,3,1,7,1,2,1,4,0,1,3,12,11,0,1,0,1,0,0,1,0,0,0,1,1,6,7,1,4,1,
	0,5,4,11,0,3,1,1,2,1,0,1,1,2,0,3,8,2,3,2,1,1,7,0,2,1,0,1,0,0,0,2,13,2,3,0,0,2,3,5,2,0,8,6,6,2,0,0,0,2,6,0,1,1,3,2,0,7,2,0,5,0,0,0,
	2,8,0,2,8,5,0,0,2,0,6,6,1,3,4,2,1,5,1,1,4,1,0,1,0,2,3,4,0,1,0,5,2,0,0,9,0,1,1,7,3,3,2,0,0,4,0,0,0,0,1,4,3,3,2,1,0,0,1,1,0,2,1,3,
	0,0,0,2,0,1,2,3,0,1,0,0,0,0,4,0,0,8,0,1,0,0,1,0,5,0,6,0,0,0,1,5,1,0,0,5,4,2,2,0,0,2,1,5,2,0,2,0,2,3,11,5,3,5,0,0,0,2,2,8,0,0,0,0,
	0,0,0,1,0,2,1,0,1,0,0,7,2,0,3,1,0,5,1,1,0,0,1,0,6,0,2,2,2,1,6,5,2,0,3,0,0,5,0,8,2,0,1,0,0,2,4,2,3,2,1,1,0,0,1,0,2,1,2,3,0,1,6,0,
	0,2,0,2,0,2,4,0,1,1,1,2,8,1,1,4,5,4,0,0,0,1,0,0,6,0,153,3,10,6,0,0,1,3,11,7,0,0,0,2,1,1,2,1,1,8,0,1,0,0,0,1,1,1,5,2,2,2,0,5,3,0,4,0,
	6,1,0,3,3,3,4,1,5,1,0,10,0,4,2,1,4,2,5,1,0,3,0,1,0,0,0,5,3,1,2,1,0,3,9,1,10,2,4,1,8,3,11,1,1,3,0,1,0,12,0,0,0,0,0,2,5,0,0,6,0,1,1,0,
	6,2,1,1,0,1,3,0,2,3,0,2,1,1,0,1,5,8,0,1,5,1,7,2,0,0,1,0,2,2,0,7,1,1,2,3,3,0,4,2,0,0,0,0,0,15,0,7,5,5,1,1,5,4,6,5,2,1,0,1,0,0,9,5,
	0,4,1,0,1,0,2,3,0,0,4,0,1,0,4,1,4,5,4,1,0,2,2,4,0,6,2,1,4,3,0,0,1,3,1,4,3,1,4,0,0,4,0,2,1,1,0,1,2,5,0,5,1,1,2,0,2,1,0,1,1,0,0,1,
	6,0,2,0,1,0,9,0,0,0,6,1,0,0,0,0,6,6,16,0,5,4,1,1,1,0,2,0,1,0,3,0,0,0,4,8,3,1,0,3,6,3,1,5,0,4,0,0,1,1,1,3,0,0,1,1,7,11,0,0,0,2,3,0,
	1,0,3,1,0,0,3,5,1,0,4,0,3,3,1,0,3,4,8,0,2,0,6,4,2,3,1,0,1,0,0,1,0,15,0,4,2,5,26,1,1,3,0,12,4,4,2,3,3,0,2,4,0,1,0,5,11,2,0,3,4,1,1,4,
	2,1,3,0,1,0,8,1,3,0,0,0,1,8,5,0,7,0,0,17,8,2,4,3,2,3,0,10,0,4,8,3,0,4,1,2,2,1,0,0,1,1,3,1,1,0,9,0,10,3,4,2,2,1,11,4,1,3,2,0,2,4,1,2,
	0,0,1,2,0,4,2,0,17,1,5,7,2,0,11,4,1,0,0,1,0,1,4,4,1,5,0,7,7,3,1,4,0,0,0,2,4,3,1,10,3,0,0,2,9,2,3,1,3,2,0,1,5,0,2,4,10,1,1,0,0,0,0,1,
	0,9,0,6,7,1,1,1,0,4,6,0,6,0,0,2,0,12,1,0,0,3,2,3,0,2,11,0,2,3,14,17,14,1,3,0,4,1,1,0,7,3,0,2,1,7,1,14,0,0,6,1,6,6,0,4,0,0,3,0,5,10,2,1,
	0,1,1,1,0,2,2,4,1,2,0,4,4,2,0,0,0,8,0,0,0,2,1,1,0,2,1,0,0,2,3,0,0,4,1,1,8,3,7,2,2,3,2,0,9,1,0,1,4,1,1,1,5,0,2,0,0,0,1,5,2,0,1,1,
	1,6,2,5,4,17,0,1,8,1,1,3,6,0,1,2,3,1,0,3,2,1,1,3,8,0,11,2,2,3,0,1,1,2,4,1,0,3,2,0,0,1,2,0,0,8,2,0,3,9,4,9,3,1,5,0,4,0,3,1,1,1,3,0,
	0,4,2,4,4,1,5,0,0,2,5,2,1,4,3,0,0,0,4,3,1,6,5,2,0,1,7,1,0,0,0,0,8,0,1,2,0,2,0,2,0,1,1,6,9,0,4,0,2,0,0,5,2,2,1,3,1,0,10,6,4,0,10,1,
	2,5,2,0,16,7,0,0,3,1,8,2,1,2,7,1,4,0,0,1,0,3,6,0,0,1,6,5,2,4,2,0,6,2,1,0,17,7,1,0,5,2,13,11,1,0,4,1,1,1,4,3,0,2,1,1,3,1,4,2,3,1,0,1,
	3,0,0,4,6,7,0,2,0,5,2,1,1,0,2,2,1,1,0,1,0,0,0,14,2,1,1,2,0,3,1,1,2,5,1,0,1,0,1,1,3,0,2,1,4,1,2,2,2,1,2,3,0,0,1,2,3,3,0,0,3,0,0,1,
	1,0,3,1,0,2,1,3,0,1,3,1,0,0,1,9,1,3,2,1,0,0,1,3,4,1,2,0,6,5,7,1,5,4,0,8,1,1,2,6,4,0,3,1,1,2,8,2,6,1,1,1,1,0,2,3,156,2,4,1,4,0,0,0,
	0,1,1,6,4,6,8,1,11,0,0,1,5,2,3,2,0,10,2,1,0,1,0,0,4,0,0,0,0,0,1,0,0,2,0,1,0,0,2,0,0,1,0,1,0,0,2,2,2,0,2,1,6,0,0,1,1,1,0,0,1,3,2,12,
	1,0,6,0,2,2,1,1,0,2,0,1,77,1,0,3,1,2,2,0,2,13,4,24,4,10,6,3,3,3,4,0,1,0,1,4,3,0,1,1,1,1,4,1,0,3,3,1,6,12,0,4,0,12,0,1,9,5,4,12,1,2,0,0,
	0,0,0,3,4,3,5,0,2,0,13,1,1,5,4,2,0,1,2,2,3,1,5,7,8,0,0,2,0,0,12,1,7,1,0,0,0,4,8,3,2,8,12,2,0,0,5,5,0,1,5,0,0,6,0,0,8,2,0,2,1,3,4,2,
	2,0,2,1,0,0,2,2,0,9,7,1,0,1,54,0,1,0,3,2,1,4,0,0,0,0,0,2,0,0,2,0,0,2,2,1,0,8,2,2,5,11,3,0,1,1,0,6,0,0,0,2,2,0,1,1,0,6,1,0,0,1,0,0,
	1,1,0,2,0,0,0,0,0,0,8,1,2,0,5,0,2,4,0,2,1,1,0,0,1,0,0,1,1,0,0,0,2,2,3,0,1,1,3,3,0,0,2,2,1,0,1,1,0,1,0,0,0,0,0,2,0,1,1,2,1,17,2,3,
	4,8,8,8,3,18,0,5,4,7,1,5,4,22,19,2,1,22,0,0,0,0,0,3,1,1,4,6,0,1,3,0,1,8,1,0,9,4,3,1,2,1,4,4,5,1,3,1,1,2,4,0,0,1,1,1,4,4,0,0,0,2,0,0,
	0,0,0,6,3,0,5,2,0,13,9,7,5,0,2,2,0,8,21,2,1,5,4,3,0,1,3,7,3,2,3,1,1,10,6,5,1,2,1,11,1,4,1,0,0,16,9,1,21,2,6,10,4,0,2,4,0,4,1,1,9,8,0,7,
	1,5,1,0,2,1,2,0,1,0,2,7,0,15,1,6,6,16,1,1,4,6,1,13,7,1,0,2,12,4,1,10,0,8,4,8,4,0,0,4,3,2,3,43,3,0,0,16,9,0,1,1,9,12,0,0,6,0,2,1,1,13,4,1,
	4,0,1,247,8,1,0,3,1,0,0,3,1,1,0,3,9,0,0,0,0,1,4,4,6,1,0,1,4,3,0,1,0,0,0,6,0,0,1,3,4,0,2,54,4,6,1,3,3,8,3,0,3,6,0,0,2,10,2,0,2,0,0,0,
	4,1,3,2,1,0,1,1,2,7,0,1,1,0,1,0,0,4,0,1,0,0,1,0,3,2,3,0,8,2,2,1,1,0,3,0,2,0,0,0,1,1,0,0,0,2,3,0,2,1,6,0,4,1,0,4,1,3,0,1,1,4,3,1,
	0,2,2,1,1,1,2,2,2,1,8,8,1,6,3,0,3,1,1,1,0,8,3,2,1,0,1,1,0,0,5,0,1,1,3,2,5,1,7,0,0,4,1,1,0,7,3,3,2,2,1,2,1,5,0,5,8,2,4,7,4,2,0,16,
	0,3,0,7,3,1,0,0,1,0,1,3,2,0,0,0,0,3,0,1,6,2,7,0,2,6,0,2,0,0,8,4,0,0,0,0,4,0,0,1,1,0,2,8,3,0,3,0,1,0,51,1,4,1,4,13,22,0,2,2,6,3,0,0,
	2,0,0,7,0,0,4,1,3,0,1,3,1,0,4,2,1,0,1,0,0,1,3,3,1,12,2,3,2,3,1,0,3,0,0,2,1,62,0,0,0,11,2,3,0,0,4,0,12,1,0,0,0,1,7,0,0,2,2,4,1,13,0,2,
	6,2,3,14,0,2,0,5,6,7,7,6,0,5,1,12,0,6,4,0,3,2,1,0,3,0,61,4,2,5,1,3,3,3,10,1,2,3,0,8,0,6,2,0,0,1,2,2,3,10,17,1,4,2,0,1,1,0,0,4,2,0,8,0,
	4,0,0,0,0,7,0,0,1,2,3,1,0,2,4,4,3,2,3,0,4,9,0,9,1,0,0,0,3,6,0,0,7,1,0,0,0,0,2,2,3,0,7,7,0,3,0,1,0,1,1,4,1,3,0,0,0,0,1,0,6,0,0,3,
	2,1,11,1,0,0,1,0,2,1,0,1,0,2,4,2,1,0,0,1,1,3,0,0,0,0,4,0,1,0,0,2,8,0,6,2,0,3,0,1,0,0,0,1,0,6,0,1,0,2,1,1,2,0,1,108,1,1,6,1,0,0,1,12,
	2,0,0,0,4,3,2,5,3,3,2,1,2,0,14,2,0,4,1,9,0,7,2,0,0,0,0,0,2,7,2,2,4,2,1,10,1,3,1,10,14,1,3,2,1,0,2,1,0,5,0,1,4,9,3,14,6,1,2,1,3,0,0,2,
	12,15,0,2,86,2,0,2,0,1,1,5,0,2,6,0,1,1,5,0,0,5,0,1,0,0,1,0,7,2,0,0,0,2,0,4,7,0,0,0,0,1,1,4,1,0,0,0,1,4,9,4,1,6,1,8,5,4,1,10,0,10,2,9,
	0,0,2,11,3,3,12,1,0,0,2,0,2,1,5,3,0,21,7,1,4,
};

static ImWchar baseUnicodeRanges[] =
{
	0x0020, 0x00FF, // Basic Latin + Latin Supplement
	0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
	0x31F0, 0x31FF, // Katakana Phonetic Extensions
	0xFF00, 0xFFEF, // Half-width characters
	0x2191, 0x2191, // Upwards arrow
	0x2193, 0x2193, // Downwards arrow
};

/**
 * Wrapper for stbtt_GetFontNameString() easier to call.
 */
std::wstring stbtt_GetFontNameStringHelper(const stbtt_fontinfo* font, int languageID, int nameID)
{
	int nameLength = 0;

	const char* str = stbtt_GetFontNameString(font, &nameLength,
		STBTT_PLATFORM_ID_MICROSOFT, STBTT_MS_EID_UNICODE_BMP, languageID, nameID);
	if (str != nullptr)
	{
		std::vector<WCHAR> strBuffer(nameLength / sizeof(WCHAR));
		for (int charIndex = 0; charIndex < nameLength / 2; charIndex++)
		{
			// swap first and second bytes due to big endian
			auto high = (std::uint8_t)str[charIndex * 2];
			auto low = (std::uint8_t)str[charIndex * 2 + 1];

			strBuffer[charIndex] = (std::uint16_t)low | ((std::uint16_t)high << 8);
		}

		return std::wstring(strBuffer.data(), strBuffer.size());
	}

	str = stbtt_GetFontNameString(font, &nameLength,
		STBTT_PLATFORM_ID_MICROSOFT, STBTT_MS_EID_SHIFTJIS, languageID, nameID);
	if (str != nullptr)
	{
		return ConvertFromSjisToUnicode(std::string(str, nameLength));
	}

	assert(false);
	return std::wstring();
}

/**
 * Returned pointer is dynamically allocated and is owned by ImGui.
 */
bool LoadSelectedFontData(HDC hdc, void*& outPointer, DWORD& outSize)
{
	// Check if the font is ttc (TrueType Collection).

	DWORD fontTable = 0x66637474;

	outSize = GetFontData(hdc, fontTable, 0, nullptr, 0);
	if (outSize == GDI_ERROR)
	{
		// Font is not ttc.
		fontTable = 0;
		outSize = GetFontData(hdc, fontTable, 0, nullptr, 0);
	}

	if (outSize == GDI_ERROR)
	{
		return false;
	}

	// Pointer is owned by ImGui.
	outPointer = ImGui::MemAlloc(outSize);
	GetFontData(hdc, fontTable, 0, outPointer, outSize);

	return true;
}

template <typename Predicate>
int FindFontIndex(const void* fontData, Predicate predicate)
{
	int numFonts = stbtt_GetNumberOfFonts(static_cast<const unsigned char*>(fontData));
	for (int fontIndex = 0; fontIndex < numFonts; fontIndex++)
	{
		stbtt_fontinfo fontInfo;
		auto offset = stbtt_GetFontOffsetForIndex(static_cast<const unsigned char*>(fontData), fontIndex);

		stbtt_InitFont(&fontInfo, static_cast<const unsigned char*>(fontData), offset);

		if (predicate(fontInfo))
		{
			return fontIndex;
		}
	}

	return -1;
}

/**
 * Returned pointer is dynamically allocated and is owned by ImGui.
 */
bool LoadPreferredFontToImGuiMemory(HDC hdc,
	const std::vector<std::pair<std::wstring, std::wstring>>& preferredFonts,
	void*& outPointer, DWORD& outSize, int& outIndex)
{
	const std::pair<std::wstring, std::wstring>* fontToFindWithinTTF = nullptr;

	void* fontData = nullptr;
	DWORD fontDataSize = 0;

	for (const auto& preferredFont : preferredFonts)
	{
		auto font = CreateFontW(0, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0,
			preferredFont.first.c_str());
		if (font == NULL)
		{
			continue;
		}

		SelectObject(hdc, font);

		bool bLoadSuccessful = LoadSelectedFontData(hdc, fontData, fontDataSize);
		DeleteObject(font);

		if (bLoadSuccessful)
		{
			fontToFindWithinTTF = &preferredFont;
			break;
		}
	}

	if (fontToFindWithinTTF == nullptr)
	{
		return false;
	}

	// Find font index of wanted style within TTF

	int fontIndex = FindFontIndex(fontData, [fontToFindWithinTTF](const stbtt_fontinfo& fontInfo)
	{
		// Although we are retrieving Japanese font, we use STBTT_MS_LANG_ENGLISH.
		// Using STBTT_MS_LANG_JAPANESE just affects the result of stbtt_GetFontNameString().
		// In addition, using STBTT_MS_LANG_JAPANESE sometimes fails to reach an offset within TTF/TTC.

		// About name ID, https://www.microsoft.com/typography/otspec/name.htm#nameIDs

		auto fontFamilyName = stbtt_GetFontNameStringHelper(&fontInfo, STBTT_MS_LANG_ENGLISH, 1);
		auto fontSubfamilyName = stbtt_GetFontNameStringHelper(&fontInfo, STBTT_MS_LANG_ENGLISH, 2);

		return fontToFindWithinTTF->first == fontFamilyName && fontToFindWithinTTF->second == fontSubfamilyName;
	});

	if (fontIndex == -1)
	{
		ImGui::MemFree(fontData);
		fontData = nullptr;
		return false;
	}

	outPointer = fontData;
	outSize = fontDataSize;
	outIndex = fontIndex;

	return true;
}

int CALLBACK FindFirstJapaneseFontProc(const LOGFONTW* logicalFont,
	const NEWTEXTMETRICEXW* fontMetricEx,
	DWORD fontType,
	LPARAM lParam)
{
	if (fontType != TRUETYPE_FONTTYPE)
	{
		return TRUE;
	}

	// Supports Japanese encoding
	if (logicalFont->lfCharSet != SHIFTJIS_CHARSET)
	{
		return TRUE;
	}

	// Variable pitch to save space
	if ((logicalFont->lfPitchAndFamily & 0x3) != VARIABLE_PITCH)
	{
		return TRUE;
	}

	// Reject font for vertical writing
	if (logicalFont->lfFaceName[0] == '@')
	{
		return TRUE;
	}

	// Check code page bitfields
	// For complete list of bitfields: https://msdn.microsoft.com/library/windows/desktop/dd317754(v=vs.85).aspx
	static std::uint8_t codePageBitfields[] =
	{
		0, 1, // Latin,
		17,   // Japanese
	};

	for (auto bitToCheck : codePageBitfields)
	{
		DWORD mask = 1 << (bitToCheck & 32);
		if (!(fontMetricEx->ntmFontSig.fsCsb[bitToCheck / 32] & mask))
		{
			return TRUE;
		}
	}

	// Check unicode subset bitfields
	// For complete list of bitfields: https://msdn.microsoft.com/library/windows/desktop/dd374090(v=vs.85).aspx
	static std::uint8_t unicodeSubsetBitfields[] =
	{
		0, 1,       // Basic Latin + Latin Supplement
		48, 49, 50, // Punctuations, Hiragana, Katakana
		68,         // Half-width characters
		37,         // Arrows
		59,         // CJK Unified Ideographs
	};

	for (auto bitToCheck : unicodeSubsetBitfields)
	{
		DWORD mask = 1 << (bitToCheck % 32);
		if (!(fontMetricEx->ntmFontSig.fsUsb[bitToCheck / 32] & mask))
		{
			return TRUE;
		}
	}

	auto& outFontFamily = *reinterpret_cast<std::wstring*>(lParam);

	outFontFamily = logicalFont->lfFaceName;

	return FALSE;
}

bool FindFirstJapaneseFont(HDC hdc, void*& outPointer, DWORD& outSize, int& outIndex)
{
	std::wstring fontFamilyName;
	auto enumFontFamUserData = reinterpret_cast<LPARAM>(&fontFamilyName);
	EnumFontFamiliesExW(hdc, nullptr, (FONTENUMPROCW)&FindFirstJapaneseFontProc, enumFontFamUserData, 0);

	if (fontFamilyName.empty())
	{
		return false;
	}

	void* fontData = nullptr;
	DWORD fontDataSize = 0;

	auto font = CreateFontW(0, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0,
		fontFamilyName.c_str());
	if (font != NULL)
	{
		SelectObject(hdc, font);

		int foundIndex = -1;
		if (LoadSelectedFontData(hdc, fontData, fontDataSize))
		{
			foundIndex = FindFontIndex(fontData, [&fontFamilyName](const stbtt_fontinfo& fontInfo)
			{
				auto currentFontFamilyName = stbtt_GetFontNameStringHelper(&fontInfo, STBTT_MS_LANG_JAPANESE, 1);
				return fontFamilyName == currentFontFamilyName;
			});

			if (foundIndex == -1)
			{
				ImGui::MemFree(fontData);
				fontData = nullptr;
			}
		}

		DeleteObject(font);

		if (foundIndex != -1)
		{
			outPointer = fontData;
			outSize = fontDataSize;
			outIndex = foundIndex;
			return true;
		}
	}

	return false;
}

void THRotatorImGui_UpdateAndApplyFixedStates(THRotatorImGui_UserData* pUserData)
{
	auto rawDevice = pUserData->device.Get();

#ifdef TOUHOU_ON_D3D8
	if (pUserData->stateBlock != INVALID_STATE_BLOCK_HANDLE)
	{
		// Apply recorded device states for sprite rendering
		rawDevice->ApplyStateBlock(pUserData->stateBlock);
		return;
	}
#else
	if (pUserData->stateBlock)
	{
		// Apply recorded device states for sprite rendering
		pUserData->stateBlock->Apply();
		return;
	}
#endif

	rawDevice->BeginStateBlock();

#if TOUHOU_ON_D3D8
	rawDevice->SetVertexShader(THRotatorImGui_Vertex::FVF);
#else
	rawDevice->SetFVF(THRotatorImGui_Vertex::FVF);
#endif

	rawDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	rawDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	rawDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	rawDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	rawDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	rawDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	rawDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	rawDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
#ifndef TOUHOU_ON_D3D8
	rawDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
#endif

	D3DMATRIX matIdentity;
	DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&matIdentity), DirectX::XMMatrixIdentity());

	rawDevice->SetTransform(D3DTS_WORLD, &matIdentity);
	rawDevice->SetTransform(D3DTS_VIEW, &matIdentity);

	rawDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	rawDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	rawDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	rawDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	rawDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	rawDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	rawDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
#ifdef TOUHOU_ON_D3D8
	rawDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	rawDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
#endif

#ifdef TOUHOU_ON_D3D8
	rawDevice->SetPixelShader(0);
#else
	rawDevice->SetPixelShader(nullptr);
	rawDevice->SetVertexShader(nullptr);
	rawDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	rawDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
#endif

	rawDevice->EndStateBlock(&pUserData->stateBlock);
}

}

void THRotatorImGui_RenderDrawLists(ImDrawData* drawData)
{
	ImGuiIO& io = ImGui::GetIO();

	// Avoid rendering when minimized
	if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
	{
		return;
	}

	auto pUserData = static_cast<THRotatorImGui_UserData*>(io.UserData);

	if (!pUserData->vertexBuffer || pUserData->vertexBufferCapacity < drawData->TotalVtxCount)
	{
		pUserData->vertexBuffer.Reset();
		pUserData->vertexBufferCapacity = drawData->TotalVtxCount + 5000;
		if (FAILED(pUserData->device->CreateVertexBuffer(pUserData->vertexBufferCapacity * sizeof(THRotatorImGui_Vertex),
			D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, THRotatorImGui_Vertex::FVF, D3DPOOL_DEFAULT, &pUserData->vertexBuffer
			ARG_PASS_SHARED_HANDLE(nullptr))))
		{
			return;
		}
	}

	if (!pUserData->indexBuffer || pUserData->indexBufferCapacity < drawData->TotalIdxCount)
	{
		pUserData->indexBuffer.Reset();
		pUserData->indexBufferCapacity = drawData->TotalIdxCount + 10000;
		if (FAILED(pUserData->device->CreateIndexBuffer(pUserData->indexBufferCapacity * sizeof(ImDrawIdx),
			D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32,
			D3DPOOL_DEFAULT, &pUserData->indexBuffer
			ARG_PASS_SHARED_HANDLE(nullptr))))
		{
			return;
		}
	}

	// Updating vertex buffer content

	THRotatorImGui_Vertex* pLockedVerticesData;
	if (FAILED(pUserData->vertexBuffer->Lock(0, drawData->TotalVtxCount * sizeof(THRotatorImGui_Vertex), (LockedPointer*)&pLockedVerticesData,
		D3DLOCK_DISCARD)))
	{
		return;
	}

	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* commandList = drawData->CmdLists[n];
		const ImDrawVert* pSourceVerticesData = commandList->VtxBuffer.Data;
		for (int i = 0; i < commandList->VtxBuffer.Size; i++)
		{
			pLockedVerticesData->pos[0] = pSourceVerticesData->pos.x;
			pLockedVerticesData->pos[1] = pSourceVerticesData->pos.y;
			pLockedVerticesData->pos[2] = 0.0f;
			pLockedVerticesData->col = (pSourceVerticesData->col & 0xFF00FF00) | ((pSourceVerticesData->col & 0xFF0000) >> 16) | ((pSourceVerticesData->col & 0xFF) << 16);     // RGBA --> ARGB for DirectX9
			pLockedVerticesData->uv[0] = pSourceVerticesData->uv.x;
			pLockedVerticesData->uv[1] = pSourceVerticesData->uv.y;
			pLockedVerticesData++;
			pSourceVerticesData++;
		}
	}

	pUserData->vertexBuffer->Unlock();

	// Updating index buffer content

	ImDrawIdx* pLockedIndicesData;
	if (FAILED(pUserData->indexBuffer->Lock(0, drawData->TotalIdxCount * sizeof(ImDrawIdx), (LockedPointer*)&pLockedIndicesData,
		D3DLOCK_DISCARD)))
	{
		return;
	}

	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* commandList = drawData->CmdLists[n];
		memcpy(pLockedIndicesData, commandList->IdxBuffer.Data, commandList->IdxBuffer.Size * sizeof(ImDrawIdx));
		pLockedIndicesData += commandList->IdxBuffer.Size;
	}

	pUserData->indexBuffer->Unlock();

#ifdef TOUHOU_ON_D3D8
	DWORD stateBlock;
#else
	ComPtr<IDirect3DStateBlock9> stateBlock;
#endif
	if (FAILED(pUserData->device->CreateStateBlock(D3DSBT_ALL, &stateBlock)))
	{
		return;
	}

	THRotatorImGui_UpdateAndApplyFixedStates(pUserData);

	auto rawDevice = pUserData->device.Get();

	rawDevice->SetStreamSource(0, pUserData->vertexBuffer.Get(),
#ifndef TOUHOU_ON_D3D8
		0,
#endif
		sizeof(THRotatorImGui_Vertex));

#ifndef TOUHOU_ON_D3D8
	D3DMATRIX matProjection;
	const float L = 0.5f, R = io.DisplaySize.x + 0.5f, T = 0.5f, B = io.DisplaySize.y + 0.5f;
	DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&matProjection),
		DirectX::XMMatrixOrthographicOffCenterLH(L, R, B, T, 0.0f, 1.0f));

	rawDevice->SetIndices(pUserData->indexBuffer.Get());
	rawDevice->SetTransform(D3DTS_PROJECTION, &matProjection);

	// Setup viewport
	D3DVIEWPORT9 vp;
	vp.X = vp.Y = 0;
	vp.Width = (DWORD)io.DisplaySize.x;
	vp.Height = (DWORD)io.DisplaySize.y;
	vp.MinZ = 0.0f;
	vp.MaxZ = 1.0f;
	rawDevice->SetViewport(&vp);
#endif

	int vertexBufferOffset = 0;
	int indexBufferOffset = 0;
	for (int commandListIndex = 0; commandListIndex < drawData->CmdListsCount; commandListIndex++)
	{
		const ImDrawList* commandList = drawData->CmdLists[commandListIndex];
		for (int commandIndex = 0; commandIndex < commandList->CmdBuffer.Size; commandIndex++)
		{
			const ImDrawCmd& command = commandList->CmdBuffer[commandIndex];
			if (command.UserCallback)
			{
				command.UserCallback(commandList, &command);
			}
			else
			{
				rawDevice->SetTexture(0, static_cast<Direct3DTextureBase*>(command.TextureId));

#ifdef TOUHOU_ON_D3D8
				D3DVIEWPORT8 viewport;
				viewport.X = static_cast<DWORD>((std::max)(0.0f, command.ClipRect.x));
				viewport.Y = static_cast<DWORD>((std::max)(0.0f, command.ClipRect.y));
				viewport.Width = static_cast<DWORD>((std::min)(io.DisplaySize.x, command.ClipRect.z - command.ClipRect.x));
				viewport.Height = static_cast<DWORD>((std::min)(io.DisplaySize.y, command.ClipRect.w - command.ClipRect.y));
				viewport.MinZ = 0.0f;
				viewport.MaxZ = 1.0f;
				rawDevice->SetViewport(&viewport);

				const float L = 0.5f + viewport.X, R = viewport.Width + 0.5f + viewport.X;
				const float T = 0.5f + viewport.Y, B = viewport.Height + 0.5f + viewport.Y;

				D3DMATRIX matProjection;
				DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&matProjection),
					DirectX::XMMatrixOrthographicOffCenterLH(L, R, B, T, 0.0f, 1.0f));

				rawDevice->SetTransform(D3DTS_PROJECTION, &matProjection);
				rawDevice->SetIndices(pUserData->indexBuffer.Get(), vertexBufferOffset);
				rawDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, (UINT)commandList->VtxBuffer.Size, indexBufferOffset, command.ElemCount / 3);
#else
				const RECT r = { (LONG)command.ClipRect.x, (LONG)command.ClipRect.y, (LONG)command.ClipRect.z, (LONG)command.ClipRect.w };
				rawDevice->SetScissorRect(&r);
				rawDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vertexBufferOffset, 0, (UINT)commandList->VtxBuffer.Size, indexBufferOffset, command.ElemCount / 3);
#endif
			}

			indexBufferOffset += command.ElemCount;
		}

		vertexBufferOffset += commandList->VtxBuffer.Size;
	}

#ifdef TOUHOU_ON_D3D8
	rawDevice->ApplyStateBlock(stateBlock);
	rawDevice->DeleteStateBlock(stateBlock);
#else
	stateBlock->Apply();
#endif
}

void THRotatorImGui_EndFrame()
{
	ImGui::EndFrame();
}

bool THRotatorImGui_Initialize(HWND hWnd, THRotatorImGui_D3DDeviceInterface* device)
{
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	io.KeyMap[ImGuiKey_Tab] = VK_TAB;                       // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';

	io.ImeWindowHandle = hWnd;

	const std::vector<std::pair<std::wstring, std::wstring>> preferredFonts
	{
		{ L"Yu Gothic UI", L"Regular" },
		{ L"Meiryo UI",    L"Regular" },
		{ L"MS UI Gothic", L"Regular" },
	};

	// TTF/TTC file data, is going to be owned by ImGui.
	void* fontData = nullptr;
	DWORD fontDataSize = 0;

	// Index within TTF/TTC
	int fontIndex = -1;

	// Temporary device context for font operations on Windows
	auto hdc = CreateCompatibleDC(nullptr);

	if (!LoadPreferredFontToImGuiMemory(hdc, preferredFonts, fontData, fontDataSize, fontIndex))
	{
		assert(fontData == nullptr);

		// If no preferred font is found, try to find a font that supports Japanese.
		FindFirstJapaneseFont(hdc, fontData, fontDataSize, fontIndex);
	}

	DeleteDC(hdc);

	io.IniFilename = nullptr; // Don't save imgui.ini

	if (fontIndex == -1)
	{
		assert(fontData == nullptr);
		OutputLogMessage(LogSeverity::Warning, "No font found that supports Japanese.");
	}
	else
	{
		static bool bKanjiRangeUnpacked = false;
		static ImWchar japaneseRanges[_countof(baseUnicodeRanges) + _countof(offsetsFrom0x4E00) * 2 + 1];
		if (!bKanjiRangeUnpacked)
		{
			// Unpack
			int codepoint = 0x4e00;
			memcpy(japaneseRanges, baseUnicodeRanges, sizeof(baseUnicodeRanges));
			ImWchar* dst = japaneseRanges + _countof(baseUnicodeRanges);
			for (int n = 0; n < _countof(offsetsFrom0x4E00); n++, dst += 2)
			{
				dst[0] = dst[1] = (ImWchar)(codepoint += (offsetsFrom0x4E00[n] + 1));
			}

			dst[0] = 0;
			bKanjiRangeUnpacked = true;
		}

		ImFontConfig fontConfig;
		fontConfig.FontNo = fontIndex;

		io.Fonts->AddFontFromMemoryTTF(fontData, fontDataSize, 16.0f, &fontConfig, japaneseRanges);
		fontData = nullptr;
		fontDataSize = 0;
	}

	io.UserData = new THRotatorImGui_UserData(device, hWnd);

	return true;
}

void THRotatorImGui_Shutdown()
{
	THRotatorImGui_InvalidateDeviceObjects();

	ImGuiIO& io = ImGui::GetIO();

	auto pUserData = static_cast<THRotatorImGui_UserData*>(io.UserData);
	delete pUserData;

	io.UserData = nullptr;

	ImGui::DestroyContext();
}

void THRotatorImGui_NewFrame(float cursorPositionScaleX, float cursorPositionScaleY)
{
	ImGuiIO& io = ImGui::GetIO();

	auto pUserData = static_cast<THRotatorImGui_UserData*>(io.UserData);

	if (!pUserData->fontTexture)
	{
		THRotatorImGui_CreateDeviceObjects();
	}

	pUserData->cursorPositionScaleX = cursorPositionScaleX;
	pUserData->cursorPositionScaleY = cursorPositionScaleY;

	// Setup display size (every frame to accommodate for window resizing)
	RECT rect;
	GetClientRect(pUserData->hDeviceWindow, &rect);
	io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

	// Setup time step
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	io.DeltaTime = (float)(currentTime.QuadPart - pUserData->previousTime.QuadPart) / pUserData->ticksPerSecond.QuadPart;
	pUserData->previousTime = currentTime;

	// Read keyboard modifiers inputs
	io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
	io.KeySuper = false;
	// io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
	// io.MousePos : filled by WM_MOUSEMOVE events
	// io.MouseDown : filled by WM_*BUTTON* events
	// io.MouseWheel : filled by WM_MOUSEWHEEL events

	// Hide OS mouse cursor if ImGui is drawing it
	if (io.MouseDrawCursor)
		SetCursor(NULL);

	// Start the frame
	ImGui::NewFrame();
}

void THRotatorImGui_InvalidateDeviceObjects()
{
	ImGuiIO& io = ImGui::GetIO();
	auto pUserData = static_cast<THRotatorImGui_UserData*>(io.UserData);

	pUserData->vertexBuffer.Reset();
	pUserData->indexBuffer.Reset();
	pUserData->fontTexture.Reset();

#ifdef TOUHOU_ON_D3D8
	if (pUserData->stateBlock != INVALID_STATE_BLOCK_HANDLE)
	{
		pUserData->device->DeleteStateBlock(pUserData->stateBlock);
		pUserData->stateBlock = INVALID_STATE_BLOCK_HANDLE;
	}
#else
	pUserData->stateBlock.Reset();
#endif

	io.Fonts->TexID = nullptr;
}

bool THRotatorImGui_CreateDeviceObjects()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();

	auto pUserData = static_cast<THRotatorImGui_UserData*>(io.UserData);

	unsigned char* pixels;
	int width, height, bytes_per_pixel;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

	// Upload texture to graphics system
	pUserData->fontTexture.Reset();
	if (FAILED(pUserData->device->CreateTexture(
		width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
		&pUserData->fontTexture
		ARG_PASS_SHARED_HANDLE(nullptr))))
	{
		return false;
	}

	D3DLOCKED_RECT tex_locked_rect;
	if (FAILED(pUserData->fontTexture->LockRect(0, &tex_locked_rect, NULL, 0)))
	{
		return false;
	}

	for (int y = 0; y < height; y++)
	{
		memcpy((unsigned char *)tex_locked_rect.pBits + tex_locked_rect.Pitch * y, pixels + (width * bytes_per_pixel) * y, (width * bytes_per_pixel));
	}

	pUserData->fontTexture->UnlockRect(0);

	// Store our identifier
	io.Fonts->TexID = static_cast<ImTextureID>(pUserData->fontTexture.Get());

	return true;
}

LRESULT THRotatorImGui_WindowProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(hWnd);

	ImGuiIO& io = ImGui::GetIO();
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		io.MouseDown[0] = true;
		return TRUE;

	case WM_LBUTTONUP:
		io.MouseDown[0] = false;
		return TRUE;

	case WM_RBUTTONDOWN:
		io.MouseDown[1] = true;
		return TRUE;

	case WM_RBUTTONUP:
		io.MouseDown[1] = false;
		return TRUE;

	case WM_MBUTTONDOWN:
		io.MouseDown[2] = true;
		return TRUE;

	case WM_MBUTTONUP:
		io.MouseDown[2] = false;
		return TRUE;

	case WM_MOUSEWHEEL:
		io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		return TRUE;

	case WM_MOUSEMOVE:
	{
		const auto& userData = *static_cast<THRotatorImGui_UserData*>(io.UserData);
		io.MousePos.x = userData.cursorPositionScaleX * (signed short)(lParam);
		io.MousePos.y = userData.cursorPositionScaleY * (signed short)(lParam >> 16);
		return TRUE;
	}
	case WM_KEYDOWN:
		if (wParam < 256)
			io.KeysDown[wParam] = 1;
		return true;

	case WM_KEYUP:
		if (wParam < 256)
			io.KeysDown[wParam] = 0;
		return TRUE;

	case WM_CHAR:
		// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
		if (wParam > 0 && wParam < 0x10000)
		{
			io.AddInputCharacter(static_cast<ImWchar>(wParam));
		}
		return TRUE;
	}
	return FALSE;
}
