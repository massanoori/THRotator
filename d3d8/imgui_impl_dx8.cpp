// ImGui Win32 + DirectX9 binding
// In this binding, ImTextureID is used to store a 'LPDIRECT3DTEXTURE9' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <imgui.h>
#include "imgui_impl_dx8.h"

// DirectX
#include <d3d8.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <wrl.h>

using Microsoft::WRL::ComPtr;

namespace
{

// Data
HWND                     g_hWnd = 0;
INT64                    g_Time = 0;
INT64                    g_TicksPerSecond = 0;
LPDIRECT3DDEVICE8        g_pd3dDevice = NULL;
LPDIRECT3DVERTEXBUFFER8  g_pVB = NULL;
LPDIRECT3DINDEXBUFFER8   g_pIB = NULL;
LPDIRECT3DTEXTURE8       g_FontTexture = NULL;
int                      g_VertexBufferSize = 5000, g_IndexBufferSize = 10000;

struct CUSTOMVERTEX
{
	float    pos[3];
	DWORD col;
	float    uv[2];

	enum : DWORD { FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 };
};

// Structs for updating and restoring device states by RAII

struct ScopedRS
{
	D3DRENDERSTATETYPE renderStateType;
	DWORD previousValue;

	ScopedRS(D3DRENDERSTATETYPE inRenderStateType, DWORD value)
		: renderStateType(inRenderStateType)
	{
		g_pd3dDevice->GetRenderState(renderStateType, &previousValue);
		g_pd3dDevice->SetRenderState(renderStateType, value);
	}

	~ScopedRS()
	{
		g_pd3dDevice->SetRenderState(renderStateType, previousValue);
	}
};

struct ScopedTSS
{
	DWORD textureStage;
	D3DTEXTURESTAGESTATETYPE textureStageStateType;
	DWORD previousValue;

	ScopedTSS(DWORD inTextureStage, D3DTEXTURESTAGESTATETYPE inTextureStageStateType, DWORD value)
		: textureStage(inTextureStage)
		, textureStageStateType(inTextureStageStateType)
	{
		g_pd3dDevice->GetTextureStageState(textureStage, textureStageStateType, &previousValue);
		g_pd3dDevice->SetTextureStageState(textureStage, textureStageStateType, value);
	}

	~ScopedTSS()
	{
		g_pd3dDevice->SetTextureStageState(textureStage, textureStageStateType, previousValue);
	}
};

struct ScopedTex
{
	DWORD textureStage;
	ComPtr<IDirect3DBaseTexture8> previousTexture;

	ScopedTex(DWORD inTextureStage, IDirect3DBaseTexture8* texture)
		: textureStage(inTextureStage)
	{
		g_pd3dDevice->GetTexture(textureStage, &previousTexture);
		g_pd3dDevice->SetTexture(textureStage, texture);
	}

	ScopedTex(DWORD inTextureStage)
		: textureStage(inTextureStage)
	{
		g_pd3dDevice->GetTexture(textureStage, &previousTexture);
	}

	~ScopedTex()
	{
		g_pd3dDevice->SetTexture(textureStage, previousTexture.Get());
	}
};

struct ScopedTransform
{
	D3DTRANSFORMSTATETYPE transformStateType;
	D3DMATRIX previousTransformMatrix;

	ScopedTransform(D3DTRANSFORMSTATETYPE inTransformType, const D3DMATRIX& transformMatrix)
		: transformStateType(inTransformType)
	{
		g_pd3dDevice->GetTransform(transformStateType, &previousTransformMatrix);
		g_pd3dDevice->SetTransform(transformStateType, &transformMatrix);
	}

	ScopedTransform(D3DTRANSFORMSTATETYPE inTransformType)
	{
		g_pd3dDevice->GetTransform(transformStateType, &previousTransformMatrix);
	}

	~ScopedTransform()
	{
		g_pd3dDevice->SetTransform(transformStateType, &previousTransformMatrix);
	}
};

struct ScopedVP
{
	D3DVIEWPORT8 previousViewport;

	ScopedVP(const D3DVIEWPORT8& viewport)
	{
		g_pd3dDevice->GetViewport(&previousViewport);
		g_pd3dDevice->SetViewport(&viewport);
	}

	ScopedVP()
	{
		g_pd3dDevice->GetViewport(&previousViewport);
	}

	~ScopedVP()
	{
		g_pd3dDevice->SetViewport(&previousViewport);
	}
};

struct ScopedStreamSource
{
	ComPtr<IDirect3DVertexBuffer8> previousVertexBuffer;
	UINT streamNumber;
	UINT previousStride;

	ScopedStreamSource(UINT inStreamNumber, IDirect3DVertexBuffer8* vertexBuffer, UINT stride)
		: streamNumber(inStreamNumber)
	{
		g_pd3dDevice->GetStreamSource(streamNumber, &previousVertexBuffer, &previousStride);
		g_pd3dDevice->SetStreamSource(streamNumber, vertexBuffer, stride);
	}

	ScopedStreamSource(UINT inStreamNumber)
		: streamNumber(inStreamNumber)
	{
		g_pd3dDevice->GetStreamSource(streamNumber, &previousVertexBuffer, &previousStride);
	}

	~ScopedStreamSource()
	{
		g_pd3dDevice->SetStreamSource(streamNumber, previousVertexBuffer.Get(), previousStride);
	}
};

struct ScopedVS
{
	DWORD previousVertexShader;

	ScopedVS(DWORD vertexShader)
	{
		g_pd3dDevice->GetVertexShader(&previousVertexShader);
		g_pd3dDevice->SetVertexShader(vertexShader);
	}

	ScopedVS()
	{
		g_pd3dDevice->GetVertexShader(&previousVertexShader);
	}

	~ScopedVS()
	{
		g_pd3dDevice->SetVertexShader(previousVertexShader);
	}
};

struct ScopedPS
{
	DWORD previousPixelShader;

	ScopedPS(DWORD pixelShader)
	{
		g_pd3dDevice->GetVertexShader(&previousPixelShader);
		g_pd3dDevice->SetVertexShader(pixelShader);
	}

	ScopedPS()
	{
		g_pd3dDevice->GetVertexShader(&previousPixelShader);
	}

	~ScopedPS()
	{
		g_pd3dDevice->SetVertexShader(previousPixelShader);
	}
};

struct ScopedIndices
{
	ComPtr<IDirect3DIndexBuffer8> previousIndexBuffer;
	UINT previousOffset;

	ScopedIndices(IDirect3DIndexBuffer8* indices, UINT offset)
	{
		g_pd3dDevice->GetIndices(&previousIndexBuffer, &previousOffset);
		g_pd3dDevice->SetIndices(indices, offset);
	}

	ScopedIndices()
	{
		g_pd3dDevice->GetIndices(&previousIndexBuffer, &previousOffset);
	}

	~ScopedIndices()
	{
		g_pd3dDevice->SetIndices(previousIndexBuffer.Get(), previousOffset);
	}
};

} // end of anonymous namespace

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplDX8_RenderDrawLists(ImDrawData* drawData)
{
	// Avoid rendering when minimized
	ImGuiIO& io = ImGui::GetIO();
	if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
		return;

	// Create and grow buffers if needed
	if (!g_pVB || g_VertexBufferSize < drawData->TotalVtxCount)
	{
		if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }
		g_VertexBufferSize = drawData->TotalVtxCount + 5000;
		if (g_pd3dDevice->CreateVertexBuffer(g_VertexBufferSize * sizeof(CUSTOMVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, CUSTOMVERTEX::FVF, D3DPOOL_DEFAULT, &g_pVB) < 0)
			return;
	}
	if (!g_pIB || g_IndexBufferSize < drawData->TotalIdxCount)
	{
		if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
		g_IndexBufferSize = drawData->TotalIdxCount + 10000;
		if (g_pd3dDevice->CreateIndexBuffer(g_IndexBufferSize * sizeof(ImDrawIdx), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, D3DPOOL_DEFAULT, &g_pIB) < 0)
			return;
	}

	// Copy and convert all vertices into a single contiguous buffer
	CUSTOMVERTEX* destVertices;
	ImDrawIdx* destIndices;
	if (g_pVB->Lock(0, (UINT)(drawData->TotalVtxCount * sizeof(CUSTOMVERTEX)), (BYTE**)&destVertices, D3DLOCK_DISCARD) < 0)
		return;
	if (g_pIB->Lock(0, (UINT)(drawData->TotalIdxCount * sizeof(ImDrawIdx)), (BYTE**)&destIndices, D3DLOCK_DISCARD) < 0)
		return;

	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* drawCommandList = drawData->CmdLists[n];
		const ImDrawVert* sourceVertices = drawCommandList->VtxBuffer.Data;
		for (int i = 0; i < drawCommandList->VtxBuffer.Size; i++)
		{
			destVertices->pos[0] = sourceVertices->pos.x;
			destVertices->pos[1] = sourceVertices->pos.y;
			destVertices->pos[2] = 0.0f;
			destVertices->col = (sourceVertices->col & 0xFF00FF00) | ((sourceVertices->col & 0xFF0000) >> 16) | ((sourceVertices->col & 0xFF) << 16);     // RGBA --> ARGB for DirectX9
			destVertices->uv[0] = sourceVertices->uv.x;
			destVertices->uv[1] = sourceVertices->uv.y;
			destVertices++;
			sourceVertices++;
		}
		memcpy(destIndices, drawCommandList->IdxBuffer.Data, drawCommandList->IdxBuffer.Size * sizeof(ImDrawIdx));
		destIndices += drawCommandList->IdxBuffer.Size;
	}
	g_pVB->Unlock();
	g_pIB->Unlock();

	ScopedStreamSource scopedStreamSource(0, g_pVB, sizeof(CUSTOMVERTEX));
	ScopedIndices scopedIndices;

	// SetFVF() equivalent for D3D8
	ScopedVS scopedVS(CUSTOMVERTEX::FVF);

	// Setup render state: fixed-pipeline, alpha-blending, no face culling, no depth testing
	ScopedPS scopedPS(0);

	ScopedRS scopedRenderStates[] =
	{
		ScopedRS(D3DRS_CULLMODE, D3DCULL_NONE),
		ScopedRS(D3DRS_LIGHTING, false),
		ScopedRS(D3DRS_ZENABLE, false),
		ScopedRS(D3DRS_ALPHABLENDENABLE, true),
		ScopedRS(D3DRS_ALPHATESTENABLE, false),
		ScopedRS(D3DRS_BLENDOP, D3DBLENDOP_ADD),
		ScopedRS(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA),
		ScopedRS(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA),
	};

	ScopedTSS scopedTextureStageStates[] =
	{
		ScopedTSS(0, D3DTSS_COLOROP, D3DTOP_MODULATE),
		ScopedTSS(0, D3DTSS_COLORARG1, D3DTA_TEXTURE),
		ScopedTSS(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE),
		ScopedTSS(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE),
		ScopedTSS(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE),
		ScopedTSS(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE),
		ScopedTSS(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR),
		ScopedTSS(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR),
	};

	ScopedTransform scopedTransforms[] =
	{
		ScopedTransform(D3DTS_WORLD),
		ScopedTransform(D3DTS_VIEW),
		ScopedTransform(D3DTS_PROJECTION),
	};

	ScopedTex scopedTexture(0);

	{
		D3DMATRIX matIdentity = { { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f } };

		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matIdentity);
		g_pd3dDevice->SetTransform(D3DTS_VIEW, &matIdentity);
	}

	// Render command lists
	int vertexBufferOffset = 0;
	int indexBufferOffset = 0;

	ScopedVP scopedViewport;

	D3DVIEWPORT8 viewport;
	viewport.MinZ = 0.0f;
	viewport.MaxZ = 1.0f;

	for (int commandListIndex = 0; commandListIndex < drawData->CmdListsCount; commandListIndex++)
	{
		const ImDrawList* drawCommandList = drawData->CmdLists[commandListIndex];
		for (int commandIndex = 0; commandIndex < drawCommandList->CmdBuffer.Size; commandIndex++)
		{
			const ImDrawCmd* drawCommand = &drawCommandList->CmdBuffer[commandIndex];
			if (drawCommand->UserCallback)
			{
				drawCommand->UserCallback(drawCommandList, drawCommand);
			}
			else
			{
				viewport.X = static_cast<DWORD>(drawCommand->ClipRect.x);
				viewport.Y = static_cast<DWORD>(drawCommand->ClipRect.y);
				viewport.Width = static_cast<DWORD>(drawCommand->ClipRect.z - drawCommand->ClipRect.x);
				viewport.Height = static_cast<DWORD>(drawCommand->ClipRect.w - drawCommand->ClipRect.y);
				g_pd3dDevice->SetViewport(&viewport);

				// Setup orthographic projection matrix

				const float L = 0.5f + viewport.X, R = viewport.Width + 0.5f + viewport.X;
				const float T = 0.5f + viewport.Y, B = viewport.Height + 0.5f + viewport.Y;
				D3DMATRIX mat_projection =
				{
					2.0f / (R - L),    0.0f,              0.0f, 0.0f,
					0.0f,              2.0f / (T - B),    0.0f, 0.0f,
					0.0f,              0.0f,              0.5f, 0.0f,
					(L + R) / (L - R), (T + B) / (B - T), 0.5f, 1.0f,
				};

				g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &mat_projection);
				g_pd3dDevice->SetTexture(0, (LPDIRECT3DTEXTURE8)drawCommand->TextureId);
				g_pd3dDevice->SetIndices(g_pIB, vertexBufferOffset);
				g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, (UINT)drawCommandList->VtxBuffer.Size,
					indexBufferOffset, drawCommand->ElemCount / 3);
			}
			indexBufferOffset += drawCommand->ElemCount;
		}
		vertexBufferOffset += drawCommandList->VtxBuffer.Size;
	}
}

IMGUI_API LRESULT ImGui_ImplDX8_WndProcHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		io.MouseDown[0] = true;
		return true;
	case WM_LBUTTONUP:
		io.MouseDown[0] = false;
		return true;
	case WM_RBUTTONDOWN:
		io.MouseDown[1] = true;
		return true;
	case WM_RBUTTONUP:
		io.MouseDown[1] = false;
		return true;
	case WM_MBUTTONDOWN:
		io.MouseDown[2] = true;
		return true;
	case WM_MBUTTONUP:
		io.MouseDown[2] = false;
		return true;
	case WM_MOUSEWHEEL:
		io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		return true;
	case WM_MOUSEMOVE:
		io.MousePos.x = (signed short)(lParam);
		io.MousePos.y = (signed short)(lParam >> 16);
		return true;
	case WM_KEYDOWN:
		if (wParam < 256)
			io.KeysDown[wParam] = 1;
		return true;
	case WM_KEYUP:
		if (wParam < 256)
			io.KeysDown[wParam] = 0;
		return true;
	case WM_CHAR:
		// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
		if (wParam > 0 && wParam < 0x10000)
			io.AddInputCharacter((unsigned short)wParam);
		return true;
	}
	return 0;
}

bool    ImGui_ImplDX8_Init(void* hwnd, IDirect3DDevice8* device)
{
	g_hWnd = (HWND)hwnd;
	g_pd3dDevice = device;

	if (!QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSecond))
		return false;
	if (!QueryPerformanceCounter((LARGE_INTEGER *)&g_Time))
		return false;

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

	io.RenderDrawListsFn = ImGui_ImplDX8_RenderDrawLists;   // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
	io.ImeWindowHandle = g_hWnd;

	return true;
}

void ImGui_ImplDX8_Shutdown()
{
	ImGui_ImplDX8_InvalidateDeviceObjects();
	ImGui::Shutdown();
	g_pd3dDevice = NULL;
	g_hWnd = 0;
}

static bool ImGui_ImplDX8_CreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height, bytes_per_pixel;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

	// Upload texture to graphics system
	g_FontTexture = NULL;
	if (g_pd3dDevice->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_FontTexture) < 0)
		return false;
	D3DLOCKED_RECT tex_locked_rect;
	if (g_FontTexture->LockRect(0, &tex_locked_rect, NULL, 0) != D3D_OK)
		return false;
	for (int y = 0; y < height; y++)
		memcpy((unsigned char *)tex_locked_rect.pBits + tex_locked_rect.Pitch * y, pixels + (width * bytes_per_pixel) * y, (width * bytes_per_pixel));
	g_FontTexture->UnlockRect(0);

	// Store our identifier
	io.Fonts->TexID = (void *)g_FontTexture;

	return true;
}

bool ImGui_ImplDX8_CreateDeviceObjects()
{
	if (!g_pd3dDevice)
		return false;
	if (!ImGui_ImplDX8_CreateFontsTexture())
		return false;
	return true;
}

void ImGui_ImplDX8_InvalidateDeviceObjects()
{
	if (!g_pd3dDevice)
		return;
	if (g_pVB)
	{
		g_pVB->Release();
		g_pVB = NULL;
	}
	if (g_pIB)
	{
		g_pIB->Release();
		g_pIB = NULL;
	}

	// At this point note that we set ImGui::GetIO().Fonts->TexID to be == g_FontTexture, so clear both.
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(g_FontTexture == io.Fonts->TexID);
	if (g_FontTexture)
		g_FontTexture->Release();
	g_FontTexture = NULL;
	io.Fonts->TexID = NULL;
}

void ImGui_ImplDX8_NewFrame()
{
	if (!g_FontTexture)
		ImGui_ImplDX8_CreateDeviceObjects();

	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	RECT rect;
	GetClientRect(g_hWnd, &rect);
	io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

	// Setup time step
	INT64 current_time;
	QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
	io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
	g_Time = current_time;

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
