// (c) 2017 massanoori

#include <windows.h>
#include <CommCtrl.h>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include "resource.h"
#include <tchar.h>
#include <ShlObj.h>
#include <wrl.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <cstdint>

#ifdef TOUHOU_ON_D3D8
#include <d3d8.h>
#include <d3dx8.h>
#pragma comment(lib, "d3dx8.lib")

#if _MSC_VER >= 1900
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

typedef IDirect3D8 Direct3DBase;
typedef IDirect3DDevice8 Direct3DDeviceBase;
typedef IDirect3DSurface8 Direct3DSurfaceBase;
typedef IDirect3DTexture8 Direct3DTextureBase;
typedef IDirect3DBaseTexture8 Direct3DBaseTextureBase;
typedef IDirect3DVertexBuffer8 Direct3DVertexBufferBase;
typedef IDirect3DIndexBuffer8 Direct3DIndexBufferBase;
typedef IDirect3DSwapChain8 Direct3DSwapChainBase;
#else
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib,"d3dx9.lib")

typedef IDirect3D9 Direct3DBase;
typedef IDirect3DDevice9 Direct3DDeviceBase;
typedef IDirect3DSurface9 Direct3DSurfaceBase;
typedef IDirect3DTexture9 Direct3DTextureBase;
typedef IDirect3DBaseTexture9 Direct3DBaseTextureBase;
typedef IDirect3DVertexBuffer9 Direct3DVertexBufferBase;
typedef IDirect3DIndexBuffer9 Direct3DIndexBufferBase;
typedef IDirect3DSwapChain9 Direct3DSwapChainBase;
#endif

#pragma comment(lib,"comctl32.lib")

static LPCTSTR THROTATOR_VERSION_STRING = "1.2.0";

using Microsoft::WRL::ComPtr;

enum RotationAngle : std::uint32_t
{
	Rotation_0 = 0,
	Rotation_90 = 1,
	Rotation_180 = 2,
	Rotation_270 = 3,

	Rotation_Num,
};

static float RotationAngleToRadian(RotationAngle rotation)
{
	assert(0 <= rotation && rotation < Rotation_Num);
	return static_cast<float>(rotation) * D3DX_PI * 0.5f;
}

struct RectTransferData
{
	RECT rcSrc, rcDest;
	RotationAngle rotation;
	TCHAR name[64];
};

const UINT BASE_SCREEN_WIDTH = 640u;
const UINT BASE_SCREEN_HEIGHT = 480u;

HANDLE g_hModule;

#ifdef TOUHOU_ON_D3D8

IDirect3D8* (WINAPI * p_Direct3DCreate8)(UINT);

FARPROC p_ValidatePixelShader;
FARPROC p_ValidateVertexShader;
FARPROC p_DebugSetMute;

#else

typedef IDirect3D9* (WINAPI * DIRECT3DCREATE9PROC)(UINT);

FARPROC p_Direct3DShaderValidatorCreate9;
FARPROC p_PSGPError;
FARPROC p_PSGPSampleTexture;
FARPROC p_D3DPERF_BeginEvent;
FARPROC p_D3DPERF_EndEvent;
FARPROC p_D3DPERF_GetStatus;
FARPROC p_D3DPERF_QueryRepeatFrame;
FARPROC p_D3DPERF_SetMarker;
FARPROC p_D3DPERF_SetOptions;
FARPROC p_D3DPERF_SetRegion;
FARPROC p_DebugSetLevel;
FARPROC p_DebugSetMute;
DIRECT3DCREATE9PROC p_Direct3DCreate9;
FARPROC p_Direct3DCreate9Ex;

#endif

class THRotatorDirect3DDevice;

class THRotatorDirect3D : public Direct3DBase
{
	friend class THRotatorDirect3DDevice;
	Direct3DBase* m_pd3d;
	ULONG m_referenceCount;
	
public:
	THRotatorDirect3D();
	virtual ~THRotatorDirect3D();

	bool init(UINT v);

	ULONG WINAPI AddRef(VOID) override;
	HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj) override;
	ULONG WINAPI Release() override;
	HRESULT WINAPI CreateDevice(UINT Adapter,
		D3DDEVTYPE DeviceType,
		HWND hFocusWindow,
		DWORD BehaviorFlags,
		D3DPRESENT_PARAMETERS *pPresentationParameters,
		Direct3DDeviceBase** ppReturnedDeviceInterface) override;

	HRESULT WINAPI CheckDepthStencilMatch(UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DFORMAT AdapterFormat,
		D3DFORMAT RenderTargetFormat,
		D3DFORMAT DepthStencilFormat) override
	{
		return m_pd3d->CheckDepthStencilMatch(Adapter,
			DeviceType,
			AdapterFormat,
			RenderTargetFormat,
			DepthStencilFormat);
	}

	HRESULT WINAPI CheckDeviceFormat(UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DFORMAT AdapterFormat,
		DWORD Usage,
		D3DRESOURCETYPE RType,
		D3DFORMAT CheckFormat) override
	{
		return m_pd3d->CheckDeviceFormat(Adapter,
			DeviceType,
			AdapterFormat,
			Usage,
			RType,
			CheckFormat);
	}

#ifndef TOUHOU_ON_D3D8
	HRESULT WINAPI CheckDeviceFormatConversion(UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DFORMAT SourceFormat,
		D3DFORMAT TargetFormat) override
	{
		return m_pd3d->CheckDeviceFormatConversion(Adapter,
			DeviceType,
			SourceFormat,
			TargetFormat);
	}
#endif

	HRESULT WINAPI CheckDeviceMultiSampleType(UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DFORMAT SurfaceFormat,
		BOOL Windowed,
#ifdef TOUHOU_ON_D3D8
		D3DMULTISAMPLE_TYPE MultiSampleType) override
#else
		D3DMULTISAMPLE_TYPE MultiSampleType,
		DWORD* pQualityLevels) override
#endif
	{
		return m_pd3d->CheckDeviceMultiSampleType(Adapter,
			DeviceType,
			SurfaceFormat,
			Windowed,
#ifdef TOUHOU_ON_D3D8
			MultiSampleType);
#else
			MultiSampleType,
			pQualityLevels);
#endif
	}
	
	HRESULT WINAPI CheckDeviceType(UINT Adapter,
		D3DDEVTYPE DeviceType,
		D3DFORMAT DisplayFormat,
		D3DFORMAT BackBufferFormat,
		BOOL Windowed) override
	{
		return m_pd3d->CheckDeviceType(Adapter,
			DeviceType,
			DisplayFormat,
			BackBufferFormat,
			Windowed);
	}


	HRESULT WINAPI EnumAdapterModes(UINT Adapter,
#ifndef TOUHOU_ON_D3D8
		D3DFORMAT Format,
#endif
		UINT Mode,
		D3DDISPLAYMODE* pMode) override
	{
		return m_pd3d->EnumAdapterModes(Adapter,
#ifndef TOUHOU_ON_D3D8
			Format,
#endif
			Mode,
			pMode);
	}

	UINT WINAPI GetAdapterCount(VOID) override
	{
		return m_pd3d->GetAdapterCount();
	}

	HRESULT WINAPI GetAdapterDisplayMode(UINT Adapter,
		D3DDISPLAYMODE *pMode) override
	{
		return m_pd3d->GetAdapterDisplayMode(Adapter, pMode);
	}

	HRESULT WINAPI GetAdapterIdentifier(UINT Adapter,
		DWORD Flags,
#ifdef TOUHOU_ON_D3D8
		D3DADAPTER_IDENTIFIER8 *pIdentifier) override
#else
		D3DADAPTER_IDENTIFIER9 *pIdentifier) override
#endif
	{
		return m_pd3d->GetAdapterIdentifier(Adapter,
			Flags,
			pIdentifier);
	}
	
#ifdef TOUHOU_ON_D3D8
	UINT WINAPI GetAdapterModeCount(UINT Adapter) override
#else
	UINT WINAPI GetAdapterModeCount(UINT Adapter,
		D3DFORMAT Format) override
#endif
	{
#ifdef TOUHOU_ON_D3D8
		return m_pd3d->GetAdapterModeCount(Adapter);
#else
		return m_pd3d->GetAdapterModeCount(Adapter, Format);
#endif
	}

	HMONITOR WINAPI GetAdapterMonitor(UINT Adapter) override
	{
		return m_pd3d->GetAdapterMonitor(Adapter);
	}

	HRESULT WINAPI GetDeviceCaps(UINT Adapter,
		D3DDEVTYPE DeviceType,
#ifdef TOUHOU_ON_D3D8
		D3DCAPS8 *pCaps) override
#else
		D3DCAPS9 *pCaps) override
#endif
	{
		return m_pd3d->GetDeviceCaps(Adapter, DeviceType, pCaps);
	}

	HRESULT WINAPI RegisterSoftwareDevice(void *pInitializeFunction) override
	{
		return m_pd3d->RegisterSoftwareDevice(pInitializeFunction);
	}
};

class THRotatorEditorContext;

class THRotatorDirect3DDevice : public Direct3DDeviceBase
{
private:
	friend class THRotatorDirect3D;

	bool m_bInitialized;
	ULONG m_referenceCount;

	/****************************************
	 * Direct3D resources
	 ****************************************/

	ComPtr<Direct3DDeviceBase> m_pd3dDev;
	ComPtr<ID3DXSprite> m_pSprite;
	ComPtr<Direct3DSurfaceBase> m_pBackBuffer, m_pTexSurface;

	// thXX.exeにレンダリングを行ってもらうレンダーターゲット。
	// ただ画面回転するだけであればm_pTexSurfaceをバックバッファ代わりにすればよいが、
	// Homeキーによるスクリーンキャプチャでクラッシュするので、別で用意
	ComPtr<Direct3DSurfaceBase> m_pRenderTarget;
#ifdef TOUHOU_ON_D3D8
	ComPtr<Direct3DSurfaceBase> m_pDepthStencil;
#endif
	ComPtr<Direct3DTextureBase> m_pTex;
	ComPtr<THRotatorDirect3D> m_pMyD3D;

#ifdef _DEBUG
	ComPtr<ID3DXFont> m_pFont;
#endif


	/****************************************
	 * Direct3D parameters
	 ****************************************/

	D3DPRESENT_PARAMETERS m_d3dpp;
	D3DTEXTUREFILTERTYPE m_filterType;
	UINT m_requestedWidth, m_requestedHeight;
#ifdef TOUHOU_ON_D3D8
	D3DSURFACE_DESC m_surfDesc;
#endif


	/****************************************
	 * Editor context data
	 ****************************************/

	std::shared_ptr<THRotatorEditorContext> m_pEditorContext;
	int m_resetRevision; // このデバイスが最後にリセットを行ったのは何番目のリセットか


#ifdef _DEBUG
	/****************************************
	* Frames per second
	****************************************/

	LARGE_INTEGER m_freq, m_cur, m_prev;
	float m_FPSNew, m_FPS;
	int m_fpsCount;
#endif

private:

	THRotatorDirect3DDevice(const THRotatorDirect3DDevice&) {}
	THRotatorDirect3DDevice(THRotatorDirect3DDevice&&) {}
	THRotatorDirect3DDevice& operator=(const THRotatorDirect3DDevice&) {}
	THRotatorDirect3DDevice& operator=(THRotatorDirect3D&&) {}

public:
	THRotatorDirect3DDevice();
	virtual ~THRotatorDirect3DDevice();

	HRESULT init(UINT Adapter, THRotatorDirect3D* pMyD3D, D3DDEVTYPE DeviceType, HWND hFocusWindow,
		DWORD BehaviorFlags, const D3DPRESENT_PARAMETERS& d3dpp,
		UINT requestedWidth, UINT requestedHeight);

	HRESULT InitResources();

	void ReleaseResources();

	ULONG WINAPI AddRef(VOID) override;
	HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj) override;
	ULONG WINAPI Release() override;
	HRESULT WINAPI Present(CONST RECT *pSourceRect,
		CONST RECT *pDestRect,
		HWND hDestWindowOverride,
		CONST RGNDATA *pDirtyRegion) override;
	HRESULT WINAPI EndScene(VOID) override;
	HRESULT WINAPI Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) override;

	HRESULT WINAPI BeginScene(VOID) override
	{
		return m_pd3dDev->BeginScene();
	}

	HRESULT WINAPI BeginStateBlock(VOID) override
	{
		return m_pd3dDev->BeginStateBlock();
	}

	HRESULT WINAPI Clear(DWORD Count,
		const D3DRECT *pRects,
		DWORD Flags,
		D3DCOLOR Color,
		float Z,
		DWORD Stencil) override
	{
		return m_pd3dDev->Clear(Count,
			pRects,
			Flags,
			Color,
			Z,
			Stencil);
	}

	HRESULT WINAPI CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters,
		Direct3DSwapChainBase** ppSwapChain) override
	{
		return m_pd3dDev->CreateAdditionalSwapChain(pPresentationParameters, ppSwapChain);
	}

	HRESULT WINAPI CreateCubeTexture(UINT EdgeLength,
		UINT Levels,
		DWORD Usage,
		D3DFORMAT Format,
		D3DPOOL Pool,
#ifdef TOUHOU_ON_D3D8
		IDirect3DCubeTexture8 **ppCubeTexture) override
#else
		IDirect3DCubeTexture9 **ppCubeTexture,
		HANDLE* pHandle) override
#endif
	{
		return m_pd3dDev->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool,
#ifdef TOUHOU_ON_D3D8
			ppCubeTexture);
#else
			ppCubeTexture, pHandle);
#endif
	}

	HRESULT WINAPI CreateDepthStencilSurface(UINT Width,
		UINT Height,
		D3DFORMAT Format,
		D3DMULTISAMPLE_TYPE MultiSample,
#ifdef TOUHOU_ON_D3D8
		Direct3DSurfaceBase** ppSurface) override
#else
		DWORD MultisampleQuality,
		BOOL Discard,
		Direct3DSurfaceBase** ppSurface,
		HANDLE* pHandle) override
#endif
	{
		return m_pd3dDev->CreateDepthStencilSurface(Width,
			Height, Format, MultiSample,
#ifdef TOUHOU_ON_D3D8
			ppSurface);
#else
			MultisampleQuality,
			Discard, ppSurface, pHandle);
#endif
	}

	HRESULT WINAPI CreateIndexBuffer(UINT Length,
		DWORD Usage,
		D3DFORMAT Format,
		D3DPOOL Pool,
#ifdef TOUHOU_ON_D3D8
		Direct3DIndexBufferBase** ppIndexBuffer) override
#else
		Direct3DIndexBufferBase** ppIndexBuffer,
		HANDLE* pHandle) override
#endif
	{
		return m_pd3dDev->CreateIndexBuffer(Length, Usage, Format, Pool,
#ifdef TOUHOU_ON_D3D8
			ppIndexBuffer);
#else
			ppIndexBuffer, pHandle);
#endif
	}

	HRESULT WINAPI CreatePixelShader(CONST DWORD *pFunction,
#ifdef TOUHOU_ON_D3D8
		DWORD* pHandle) override
#else
		IDirect3DPixelShader9** ppShader) override
#endif
	{
		return m_pd3dDev->CreatePixelShader(pFunction,
#ifdef TOUHOU_ON_D3D8
			pHandle);
#else
			ppShader);
#endif
	}

	HRESULT WINAPI CreateRenderTarget(UINT Width,
		UINT Height,
		D3DFORMAT Format,
		D3DMULTISAMPLE_TYPE MultiSample,
#ifndef TOUHOU_ON_D3D8
		DWORD MultisampleQuality,
#endif
		BOOL Lockable,
#ifdef TOUHOU_ON_D3D8
		Direct3DSurfaceBase** ppSurface) override
#else
		Direct3DSurfaceBase** ppSurface,
		HANDLE* pHandle) override
#endif
	{
		return m_pd3dDev->CreateRenderTarget(Width, Height, Format, MultiSample,
#ifdef TOUHOU_ON_D3D8
			Lockable, ppSurface);
#else
			MultisampleQuality,
			Lockable, ppSurface, pHandle);
#endif
	}

	HRESULT WINAPI CreateStateBlock(D3DSTATEBLOCKTYPE Type,
#ifdef TOUHOU_ON_D3D8
		DWORD* pToken) override
#else
		IDirect3DStateBlock9** ppSB) override
#endif
	{
		return m_pd3dDev->CreateStateBlock(Type,
#ifdef TOUHOU_ON_D3D8
			pToken);
#else
			ppSB);
#endif
	}

	HRESULT WINAPI CreateTexture(UINT Width,
		UINT Height,
		UINT Levels,
		DWORD Usage,
		D3DFORMAT Format,
		D3DPOOL Pool,
#ifdef TOUHOU_ON_D3D8
		Direct3DTextureBase** ppTexture) override
#else
		Direct3DTextureBase** ppTexture,
		HANDLE* pHandle) override
#endif
	{
		return m_pd3dDev->CreateTexture(Width, Height, Levels,
			Usage, Format, Pool,
#ifdef TOUHOU_ON_D3D8
			ppTexture);
#else
			ppTexture, pHandle);
#endif
	}

	HRESULT WINAPI CreateVertexBuffer(UINT Length,
		DWORD Usage,
		DWORD FVF,
		D3DPOOL Pool,
#ifdef TOUHOU_ON_D3D8
		Direct3DVertexBufferBase** ppVertexBuffer) override
#else
		Direct3DVertexBufferBase** ppVertexBuffer,
		HANDLE* pHandle) override
#endif
	{
		return m_pd3dDev->CreateVertexBuffer(Length,
			Usage, FVF, Pool,
#ifdef TOUHOU_ON_D3D8
			ppVertexBuffer);
#else
			ppVertexBuffer, pHandle);
#endif
	}

	HRESULT WINAPI CreateVertexShader(
#ifdef TOUHOU_ON_D3D8
		const DWORD* pDeclaration, const DWORD *pFunction,
		DWORD* pHandle, DWORD Usage) override
#else
		const DWORD *pFunction,
		IDirect3DVertexShader9** ppShader) override
#endif
	{
#ifdef TOUHOU_ON_D3D8
		return m_pd3dDev->CreateVertexShader(pDeclaration, pFunction, pHandle, Usage);
#else
		return m_pd3dDev->CreateVertexShader(pFunction, ppShader);
#endif
	}

	HRESULT WINAPI CreateVolumeTexture(UINT Width,
		UINT Height,
		UINT Depth,
		UINT Levels,
		DWORD Usage,
		D3DFORMAT Format,
		D3DPOOL Pool,
#ifdef TOUHOU_ON_D3D8
		IDirect3DVolumeTexture8** ppVolumeTexture) override
#else
		IDirect3DVolumeTexture9** ppVolumeTexture,
		HANDLE* pHandle) override
#endif
	{
		return m_pd3dDev->CreateVolumeTexture(Width, Height, Depth,
			Levels, Usage, Format, Pool,
#ifdef TOUHOU_ON_D3D8
			ppVolumeTexture);
#else
			ppVolumeTexture, pHandle);
#endif
	}

	HRESULT WINAPI DeletePatch(UINT Handle) override
	{
		return m_pd3dDev->DeletePatch(Handle);
	}

	HRESULT WINAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE Type,
#ifndef TOUHOU_ON_D3D8
		INT BaseVertexIndex,
#endif
		UINT MinIndex,
		UINT NumVertices,
		UINT StartIndex,
		UINT PrimitiveCount) override
	{
		return m_pd3dDev->DrawIndexedPrimitive(Type,
#ifndef TOUHOU_ON_D3D8
			BaseVertexIndex,
#endif
			MinIndex,
			NumVertices,
			StartIndex,
			PrimitiveCount);
	}

	HRESULT WINAPI DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,
		UINT MinVertexIndex,
		UINT NumVertexIndices,
		UINT PrimitiveCount,
		const void *pIndexData,
		D3DFORMAT IndexDataFormat,
		CONST void* pVertexStreamZeroData,
		UINT VertexStreamZeroStride) override
	{
		return m_pd3dDev->DrawIndexedPrimitiveUP(PrimitiveType,
			MinVertexIndex, NumVertexIndices, PrimitiveCount,
			pIndexData, IndexDataFormat, pVertexStreamZeroData,
			VertexStreamZeroStride);
	}

	HRESULT WINAPI DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,
		UINT StartVertex,
		UINT PrimitiveCount) override
	{
		return m_pd3dDev->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
	}

	HRESULT WINAPI DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,
		UINT PrimitiveCount,
		const void *pVertexStreamZeroData,
		UINT VertexStreamZeroStride) override
	{
		return m_pd3dDev->DrawPrimitiveUP(PrimitiveType,
			PrimitiveCount, pVertexStreamZeroData,
			VertexStreamZeroStride);
	}

	HRESULT WINAPI DrawRectPatch(UINT Handle,
		const float* pNumSegs,
		const D3DRECTPATCH_INFO* pRectPatchInfo) override
	{
		return m_pd3dDev->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
	}

	HRESULT WINAPI DrawTriPatch(UINT Handle,
		const float* pNumSegs,
		const D3DTRIPATCH_INFO* pTriPatchInfo) override
	{
		return m_pd3dDev->DrawTriPatch(Handle, pNumSegs,
			pTriPatchInfo);
	}

	HRESULT WINAPI EndStateBlock(
#ifdef TOUHOU_ON_D3D8
		DWORD* pToken) override
#else
		IDirect3DStateBlock9 **ppSB) override
#endif
	{
#ifdef TOUHOU_ON_D3D8
		return m_pd3dDev->EndStateBlock(pToken);
#else
		return m_pd3dDev->EndStateBlock(ppSB);
#endif
	}

	UINT WINAPI GetAvailableTextureMem(VOID) override
	{
		return m_pd3dDev->GetAvailableTextureMem();
	}

	HRESULT WINAPI GetBackBuffer(
#ifndef TOUHOU_ON_D3D8
		UINT iSwapChain,
#endif
		UINT BackBuffer,
		D3DBACKBUFFER_TYPE Type,
		Direct3DSurfaceBase** ppBackBuffer) override
	{
		*ppBackBuffer = m_pRenderTarget.Get();
		(*ppBackBuffer)->AddRef();
		return S_OK;
	}

	HRESULT WINAPI GetClipPlane(DWORD Index,
		float *pPlane) override
	{
		return m_pd3dDev->GetClipPlane(Index, pPlane);
	}

	HRESULT WINAPI GetClipStatus(
#ifdef TOUHOU_ON_D3D8
		D3DCLIPSTATUS8* pClipStatus) override
#else
		D3DCLIPSTATUS9* pClipStatus) override
#endif
	{
		return m_pd3dDev->GetClipStatus( pClipStatus );
	}

	HRESULT WINAPI GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters) override
	{
		return m_pd3dDev->GetCreationParameters(pParameters);
	}

	HRESULT WINAPI GetCurrentTexturePalette(UINT *pPaletteNumber) override
	{
		return m_pd3dDev->GetCurrentTexturePalette(pPaletteNumber);
	}

	HRESULT WINAPI GetDepthStencilSurface(Direct3DSurfaceBase ** ppZStencilSurface) override
	{
		return m_pd3dDev->GetDepthStencilSurface(ppZStencilSurface);
	}

	HRESULT WINAPI GetDeviceCaps(
#ifdef TOUHOU_ON_D3D8
		D3DCAPS8* pCaps) override
#else
		D3DCAPS9* pCaps) override
#endif
	{
		return m_pd3dDev->GetDeviceCaps( pCaps );
	}

	HRESULT WINAPI GetDirect3D(Direct3DBase** ppD3D) override
	{
		*ppD3D = m_pMyD3D.Get();
		(*ppD3D)->AddRef();
		return S_OK;
	}

	HRESULT WINAPI GetDisplayMode(
#ifndef TOUHOU_ON_D3D8
		UINT iSwapChain,
#endif
		D3DDISPLAYMODE *pMode) override
	{
		return m_pd3dDev->GetDisplayMode(
#ifndef TOUHOU_ON_D3D8
			iSwapChain,
#endif
			pMode);
	}

	void WINAPI GetGammaRamp(
#ifndef TOUHOU_ON_D3D8
		UINT iSwapChain,
#endif
		D3DGAMMARAMP *pRamp) override
	{
		return m_pd3dDev->GetGammaRamp(
#ifndef TOUHOU_ON_D3D8
			iSwapChain,
#endif
			pRamp);
	}

	HRESULT WINAPI GetIndices(
#ifdef TOUHOU_ON_D3D8
		Direct3DIndexBufferBase **ppIndexData, UINT* pBaseVertexIndex) override
#else
		Direct3DIndexBufferBase **ppIndexData) override
#endif
	{

		return m_pd3dDev->GetIndices(
#ifdef TOUHOU_ON_D3D8
			ppIndexData, pBaseVertexIndex);
#else
			ppIndexData);
#endif
	}

	HRESULT WINAPI GetLight(DWORD Index,
#ifdef TOUHOU_ON_D3D8
		D3DLIGHT8* pLight) override
#else
		D3DLIGHT9* pLight) override
#endif
	{
		return m_pd3dDev->GetLight(Index, pLight);
	}

	HRESULT WINAPI GetLightEnable(DWORD Index,
		BOOL *pEnable) override
	{
		return m_pd3dDev->GetLightEnable(Index, pEnable);
	}

	HRESULT WINAPI GetMaterial(
#ifdef TOUHOU_ON_D3D8
		D3DMATERIAL8* pMaterial) override
#else
		D3DMATERIAL9* pMaterial) override
#endif
	{
		return m_pd3dDev->GetMaterial(pMaterial);
	}

	HRESULT WINAPI GetPaletteEntries(UINT PaletteNumber,
		PALETTEENTRY *pEntries) override
	{
		return m_pd3dDev->GetPaletteEntries(PaletteNumber, pEntries);
	}

	HRESULT WINAPI GetPixelShader(
#ifdef TOUHOU_ON_D3D8
		DWORD* pHandle) override
#else
		IDirect3DPixelShader9** ppShader) override
#endif
	{
#ifdef TOUHOU_ON_D3D8
		return m_pd3dDev->GetPixelShader(pHandle);
#else
		return m_pd3dDev->GetPixelShader(ppShader);
#endif
	}

	HRESULT WINAPI GetRasterStatus(
#ifndef TOUHOU_ON_D3D8
		UINT iSwapChain,
#endif
		D3DRASTER_STATUS *pRasterStatus) override
	{
		return m_pd3dDev->GetRasterStatus(
#ifndef TOUHOU_ON_D3D8
			iSwapChain,
#endif
			pRasterStatus);
	}

	HRESULT WINAPI GetRenderState(D3DRENDERSTATETYPE State,
		DWORD *pValue) override
	{
		return m_pd3dDev->GetRenderState(State, pValue);
	}

	HRESULT WINAPI GetRenderTarget(
#ifndef TOUHOU_ON_D3D8
		DWORD RenderTargetIndex,
#endif
		Direct3DSurfaceBase** ppRenderTarget) override
	{
		return m_pd3dDev->GetRenderTarget(
#ifndef TOUHOU_ON_D3D8
			RenderTargetIndex,
#endif
			ppRenderTarget);
	}

	HRESULT WINAPI GetStreamSource(UINT StreamNumber,
		Direct3DVertexBufferBase** ppStreamData,
#ifndef TOUHOU_ON_D3D8
		UINT* pOffsetInBytes,
#endif
		UINT* pStride) override
	{
		return m_pd3dDev->GetStreamSource(StreamNumber, ppStreamData,
#ifndef TOUHOU_ON_D3D8
			pOffsetInBytes,
#endif
			pStride);
	}
	HRESULT WINAPI GetTexture(DWORD Stage,
		Direct3DBaseTextureBase** ppTexture) override
	{
		return m_pd3dDev->GetTexture(Stage, ppTexture);
	}

	HRESULT WINAPI GetTextureStageState(DWORD Stage,
		D3DTEXTURESTAGESTATETYPE Type,
		DWORD *pValue) override
	{
		return m_pd3dDev->GetTextureStageState(Stage, Type, pValue);
	}

	HRESULT WINAPI GetTransform(D3DTRANSFORMSTATETYPE State,
		D3DMATRIX *pMatrix) override
	{
		return m_pd3dDev->GetTransform(State, pMatrix);
	}

	HRESULT WINAPI GetVertexShader(
#ifdef TOUHOU_ON_D3D8
		DWORD* pHandle) override
#else
		IDirect3DVertexShader9** ppShader) override
#endif
	{
#ifdef TOUHOU_ON_D3D8
		return m_pd3dDev->GetVertexShader(pHandle);
#else
		return m_pd3dDev->GetVertexShader(ppShader);
#endif
	}

	HRESULT WINAPI GetViewport(
#ifdef TOUHOU_ON_D3D8
		D3DVIEWPORT8* pViewport) override
#else
		D3DVIEWPORT9* pViewport) override
#endif
	{
		return m_pd3dDev->GetViewport(pViewport);
	}

	HRESULT WINAPI LightEnable(DWORD LightIndex,
		BOOL bEnable) override
	{
		return m_pd3dDev->LightEnable(LightIndex, bEnable);
	}

	HRESULT WINAPI MultiplyTransform(D3DTRANSFORMSTATETYPE State,
		CONST D3DMATRIX *pMatrix) override
	{
		return m_pd3dDev->MultiplyTransform(State, pMatrix);
	}

	HRESULT WINAPI ProcessVertices(UINT SrcStartIndex,
		UINT DestIndex,
		UINT VertexCount,
		Direct3DVertexBufferBase *pDestBuffer,
#ifndef TOUHOU_ON_D3D8
		IDirect3DVertexDeclaration9* pVertexDecl,
#endif
		DWORD Flags) override
	{
		return m_pd3dDev->ProcessVertices(SrcStartIndex,
			DestIndex, VertexCount, pDestBuffer,
#ifndef TOUHOU_ON_D3D8
			pVertexDecl,
#endif
			Flags);
	}

	HRESULT WINAPI SetClipPlane(DWORD Index,
		const float *pPlane) override
	{
		return m_pd3dDev->SetClipPlane(Index, pPlane);
	}

	HRESULT WINAPI SetClipStatus(
#ifdef TOUHOU_ON_D3D8
		const D3DCLIPSTATUS8 *pClipStatus) override
#else
		const D3DCLIPSTATUS9 *pClipStatus) override
#endif
	{
		return m_pd3dDev->SetClipStatus(pClipStatus);
	}

	HRESULT WINAPI SetCurrentTexturePalette(UINT PaletteNumber) override
	{
		return m_pd3dDev->SetCurrentTexturePalette(PaletteNumber);
	}

	void WINAPI SetCursorPosition(
		INT X, INT Y,
		DWORD Flags) override
	{
		m_pd3dDev->SetCursorPosition(X, Y, Flags);
	}

	HRESULT WINAPI SetCursorProperties(UINT XHotSpot,
		UINT YHotSpot,
		Direct3DSurfaceBase* pCursorBitmap) override
	{
		return m_pd3dDev->SetCursorProperties(XHotSpot,
			YHotSpot, pCursorBitmap);
	}

	void WINAPI SetGammaRamp(
#ifndef TOUHOU_ON_D3D8
		UINT iSwapChain,
#endif
		DWORD Flags,
		CONST D3DGAMMARAMP *pRamp) override
	{
		m_pd3dDev->SetGammaRamp(
#ifndef TOUHOU_ON_D3D8
			iSwapChain,
#endif
			Flags, pRamp);
	}

	HRESULT WINAPI SetIndices(
#ifdef TOUHOU_ON_D3D8
		Direct3DIndexBufferBase* pIndexData,
		UINT BaseVertexIndex) override
#else
		Direct3DIndexBufferBase* pIndexData) override
#endif
	{
#ifdef TOUHOU_ON_D3D8
		return m_pd3dDev->SetIndices(pIndexData, BaseVertexIndex);
#else
		return m_pd3dDev->SetIndices(pIndexData);
#endif
	}

	HRESULT WINAPI SetLight(DWORD Index,
#ifdef TOUHOU_ON_D3D8
		CONST D3DLIGHT8* pLight) override
#else
		CONST D3DLIGHT9* pLight) override
#endif
	{
		return m_pd3dDev->SetLight(Index, pLight);
	}

	HRESULT WINAPI SetMaterial(
#ifdef TOUHOU_ON_D3D8
		CONST D3DMATERIAL8* pMaterial) override
#else
		CONST D3DMATERIAL9* pMaterial) override
#endif
	{
		return m_pd3dDev->SetMaterial(pMaterial);
	}

	HRESULT WINAPI SetPaletteEntries(          UINT PaletteNumber,
		const PALETTEENTRY *pEntries ) override
	{
		return m_pd3dDev->SetPaletteEntries( PaletteNumber, pEntries );
	}

	HRESULT WINAPI SetPixelShader(
#ifdef TOUHOU_ON_D3D8
		DWORD Handle) override
#else
		IDirect3DPixelShader9* pShader)
#endif
	{
		return m_pd3dDev->SetPixelShader(
#ifdef TOUHOU_ON_D3D8
			Handle);
#else
			pShader);
#endif
	}

	HRESULT WINAPI SetRenderState(D3DRENDERSTATETYPE State,
		DWORD Value) override
	{
		return m_pd3dDev->SetRenderState(State, Value);
	}

	HRESULT WINAPI SetRenderTarget(
#ifdef TOUHOU_ON_D3D8
		Direct3DSurfaceBase* pRenderTarget,
		Direct3DSurfaceBase* pNewZStencil) override
#else
		DWORD RenderTargetIndex,
		Direct3DSurfaceBase* pRenderTarget) override
#endif
	{
#ifdef TOUHOU_ON_D3D8
		return m_pd3dDev->SetRenderTarget(pRenderTarget, pNewZStencil);
#else
		return m_pd3dDev->SetRenderTarget(RenderTargetIndex, pRenderTarget);
#endif
	}

	HRESULT WINAPI SetStreamSource(UINT StreamNumber,
		Direct3DVertexBufferBase *pStreamData,
#ifndef TOUHOU_ON_D3D8
		UINT OffsetInBytes,
#endif
		UINT Stride) override
	{
		return m_pd3dDev->SetStreamSource(StreamNumber, pStreamData,
#ifndef TOUHOU_ON_D3D8
			OffsetInBytes,
#endif
			Stride);
	}

	HRESULT WINAPI SetTexture(DWORD Stage,
		Direct3DBaseTextureBase* pTexture) override
	{
		return m_pd3dDev->SetTexture(Stage, pTexture);
	}

	HRESULT WINAPI SetTextureStageState(DWORD Stage,
		D3DTEXTURESTAGESTATETYPE Type,
		DWORD Value) override
	{
		return m_pd3dDev->SetTextureStageState(Stage, Type, Value);
	}

	HRESULT WINAPI SetTransform(          D3DTRANSFORMSTATETYPE State,
		CONST D3DMATRIX *pMatrix ) override
	{
		return m_pd3dDev->SetTransform( State, pMatrix );
	}

	HRESULT WINAPI SetVertexShader(
#ifdef TOUHOU_ON_D3D8
		DWORD Handle) override
#else
		IDirect3DVertexShader9* pShader) override
#endif
	{
#ifdef TOUHOU_ON_D3D8
		return m_pd3dDev->SetVertexShader(Handle);
#else
		return m_pd3dDev->SetVertexShader(pShader);
#endif
	}

	HRESULT WINAPI SetViewport(
#ifdef TOUHOU_ON_D3D8
		CONST D3DVIEWPORT8* pViewport) override;
#else
		CONST D3DVIEWPORT9* pViewport) override;
#endif

	BOOL WINAPI ShowCursor(BOOL bShow) override
	{
		return m_pd3dDev->ShowCursor(bShow);
	}

	HRESULT WINAPI TestCooperativeLevel(VOID) override
	{
		return m_pd3dDev->TestCooperativeLevel();
	}

	HRESULT WINAPI UpdateTexture(Direct3DBaseTextureBase* pSourceTexture,
		Direct3DBaseTextureBase *pDestinationTexture) override
	{
		return m_pd3dDev->UpdateTexture(pSourceTexture,
			pDestinationTexture);
	}

	HRESULT WINAPI ValidateDevice(DWORD *pNumPasses) override
	{
		return m_pd3dDev->ValidateDevice(pNumPasses);
	}

#ifdef TOUHOU_ON_D3D8

	// Interface implementations that appears only in IDirect3D8

	HRESULT WINAPI CopyRects(
		IDirect3DSurface8* pSourceSurface,
		CONST RECT* pSourceRectsArray,
		UINT cRects,
		IDirect3DSurface8* pDestinationSurface,
		CONST POINT* pDestPointsArray) override
	{
		return m_pd3dDev->CopyRects(pSourceSurface, pSourceRectsArray,
			cRects, pDestinationSurface, pDestPointsArray);
	}

	HRESULT WINAPI CreateImageSurface(
		UINT Width,
		UINT Height,
		D3DFORMAT Format,
		IDirect3DSurface8** ppSurface) override
	{
		return m_pd3dDev->CreateImageSurface(Width, Height, Format, ppSurface);
	}

	HRESULT WINAPI GetFrontBuffer(Direct3DSurfaceBase *pDestSurface) override
	{
		return m_pd3dDev->GetFrontBuffer(pDestSurface);
	}

	HRESULT WINAPI GetPixelShaderConstant(THIS_ DWORD Register, void* pConstantData, DWORD ConstantCount) override
	{
		return m_pd3dDev->GetPixelShaderConstant(Register, pConstantData, ConstantCount);
	}

	HRESULT WINAPI GetPixelShaderFunction(DWORD Register, void* pConstantData, DWORD* pSizeOfData) override
	{
		return m_pd3dDev->GetPixelShaderFunction(Register, pConstantData, pSizeOfData);
	}

	HRESULT WINAPI GetVertexShaderConstant(DWORD Register,
		void* pConstantData, DWORD ConstantCount) override
	{
		return m_pd3dDev->GetVertexShaderConstant(Register,
			pConstantData, ConstantCount);
	}

	HRESULT WINAPI GetVertexShaderFunction(DWORD Register, void* pConstantData, DWORD* pSizeOfData) override
	{
		return m_pd3dDev->GetVertexShaderFunction(Register, pConstantData, pSizeOfData);
	}

	HRESULT WINAPI GetVertexShaderDeclaration(DWORD Handle, void* pData, DWORD* pSizeOfData) override
	{
		return m_pd3dDev->GetVertexShaderDeclaration(Handle, pData, pSizeOfData);
	}

	HRESULT WINAPI SetPixelShaderConstant(DWORD Register, CONST void* pConstantData, DWORD ConstantCount)
	{
		return m_pd3dDev->SetPixelShaderConstant(Register,
			pConstantData, ConstantCount);
	}

	HRESULT WINAPI SetVertexShaderConstant(DWORD Register, CONST void* pConstantData, DWORD ConstantCount) override
	{
		return m_pd3dDev->SetVertexShaderConstant(Register,
			pConstantData, ConstantCount);
	}

	HRESULT WINAPI ResourceManagerDiscardBytes(DWORD dw) override
	{
		return m_pd3dDev->ResourceManagerDiscardBytes(dw);
	}

	HRESULT WINAPI ApplyStateBlock(DWORD Token)
	{
		return m_pd3dDev->ApplyStateBlock(Token);
	}

	HRESULT WINAPI CaptureStateBlock(DWORD Token)
	{
		return m_pd3dDev->CaptureStateBlock(Token);
	}

	HRESULT WINAPI DeleteStateBlock(DWORD Token)
	{
		return m_pd3dDev->DeleteStateBlock(Token);
	}

	HRESULT WINAPI GetInfo(DWORD DevInfoID, VOID* pDevInfoStruct,
		DWORD DevInfoStructSize)
	{
		return m_pd3dDev->GetInfo(DevInfoID, pDevInfoStruct, DevInfoStructSize);
	}

	HRESULT WINAPI DeleteVertexShader(DWORD Handle)
	{
		return m_pd3dDev->DeleteVertexShader(Handle);
	}

	HRESULT WINAPI DeletePixelShader(DWORD Handle)
	{
		return m_pd3dDev->DeletePixelShader(Handle);
	}
#else // #ifdef TOUHOU_ON_D3D8

	// Interface implementations that appears only in IDirect3D9

	HRESULT WINAPI ColorFill(IDirect3DSurface9 *pSurface,
		CONST RECT *pRect,
		D3DCOLOR color) override
	{
		return m_pd3dDev->ColorFill(pSurface, pRect, color);
	}

	HRESULT WINAPI CreateOffscreenPlainSurface(UINT Width,
		UINT Height,
		D3DFORMAT Format,
		D3DPOOL Pool,
		Direct3DSurfaceBase** ppSurface,
		HANDLE* pHandle) override
	{
		return m_pd3dDev->CreateOffscreenPlainSurface(Width,
			Height, Format, Pool, ppSurface, pHandle);
	}

	HRESULT WINAPI CreateQuery(D3DQUERYTYPE Type,
		IDirect3DQuery9** ppQuery) override
	{
		return m_pd3dDev->CreateQuery(Type, ppQuery);
	}

	HRESULT WINAPI CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements,
		IDirect3DVertexDeclaration9** ppDecl) override
	{
		return m_pd3dDev->CreateVertexDeclaration(pVertexElements, ppDecl);
	}

	HRESULT WINAPI EvictManagedResources(VOID) override
	{
		return m_pd3dDev->EvictManagedResources();
	}

	HRESULT WINAPI GetFrontBufferData(
		UINT iSwapChain,
		Direct3DSurfaceBase *pDestSurface) override
	{
		return m_pd3dDev->GetFrontBufferData(
			iSwapChain,
			pDestSurface);
	}

	HRESULT WINAPI GetFVF(DWORD *pFVF) override
	{
		return m_pd3dDev->GetFVF(pFVF);
	}

	FLOAT WINAPI GetNPatchMode(VOID) override
	{
		return m_pd3dDev->GetNPatchMode();
	}

	UINT WINAPI GetNumberOfSwapChains(VOID) override
	{
		return m_pd3dDev->GetNumberOfSwapChains();
	}

	HRESULT WINAPI GetPixelShaderConstantB(UINT StartRegister,
		BOOL *pConstantData,
		UINT BoolCount) override
	{
		return m_pd3dDev->GetPixelShaderConstantB(StartRegister,
			pConstantData, BoolCount);
	}

	HRESULT WINAPI GetPixelShaderConstantF(UINT StartRegister,
		float *pConstantData,
		UINT Vector4fCount) override
	{
		return m_pd3dDev->GetPixelShaderConstantF(StartRegister,
			pConstantData, Vector4fCount);
	}

	HRESULT WINAPI GetPixelShaderConstantI(UINT StartRegister,
		int *pConstantData,
		UINT Vector4iCount) override
	{
		return m_pd3dDev->GetPixelShaderConstantI(StartRegister,
			pConstantData, Vector4iCount);
	}

	HRESULT WINAPI GetRenderTargetData(Direct3DSurfaceBase* pRenderTarget,
		Direct3DSurfaceBase* pDestSurface) override
	{
		return m_pd3dDev->GetRenderTargetData(pRenderTarget,
			pDestSurface);
	}

	HRESULT WINAPI GetSamplerState(DWORD Sampler,
		D3DSAMPLERSTATETYPE Type,
		DWORD* pValue) override
	{
		return m_pd3dDev->GetSamplerState(Sampler, Type, pValue);
	}

	HRESULT WINAPI GetScissorRect(RECT* pRect) override
	{
		return m_pd3dDev->GetScissorRect(pRect);
	}

	BOOL WINAPI GetSoftwareVertexProcessing(VOID) override
	{
		return m_pd3dDev->GetSoftwareVertexProcessing();
	}

	HRESULT WINAPI GetStreamSourceFreq(UINT StreamNumber,
		UINT* pDivider)
	{
		return m_pd3dDev->GetStreamSourceFreq(StreamNumber,
			pDivider);
	}

	HRESULT WINAPI GetSwapChain(UINT iSwapChain,
		Direct3DSwapChainBase **ppSwapChain)
	{
		return m_pd3dDev->GetSwapChain(iSwapChain, ppSwapChain);
	}

	HRESULT WINAPI GetVertexShaderConstantB(UINT StartRegister,
		BOOL *pConstantData,
		UINT BoolCount) override
	{
		return m_pd3dDev->GetVertexShaderConstantB(StartRegister,
			pConstantData, BoolCount);
	}

	HRESULT WINAPI GetVertexShaderConstantF(UINT StartRegister,
		float *pConstantData,
		UINT Vector4fCount) override
	{
		return m_pd3dDev->GetVertexShaderConstantF(StartRegister,
			pConstantData, Vector4fCount);
	}

	HRESULT WINAPI GetVertexShaderConstantI(UINT StartRegister,
		int *pConstantData,
		UINT Vector4iCount) override
	{
		return m_pd3dDev->GetVertexShaderConstantI(StartRegister,
			pConstantData, Vector4iCount);
	}

	HRESULT WINAPI GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) override
	{
		return m_pd3dDev->GetVertexDeclaration(ppDecl);
	}

	HRESULT WINAPI SetDepthStencilSurface(Direct3DSurfaceBase* pZStencilSurface) override
	{
		return m_pd3dDev->SetDepthStencilSurface(pZStencilSurface);
	}

	HRESULT WINAPI SetDialogBoxMode(BOOL bEnableDialogs) override
	{
		return m_pd3dDev->SetDialogBoxMode(bEnableDialogs);
	}

	HRESULT WINAPI SetFVF(DWORD FVF) override
	{
		return m_pd3dDev->SetFVF(FVF);
	}

	HRESULT WINAPI SetNPatchMode(float nSegments) override
	{
		return m_pd3dDev->SetNPatchMode(nSegments);
	}

	HRESULT WINAPI SetPixelShaderConstantB(UINT StartRegister,
		CONST BOOL *pConstantData,
		UINT BoolCount) override
	{
		return m_pd3dDev->SetPixelShaderConstantB(StartRegister,
			pConstantData, BoolCount);
	}

	HRESULT WINAPI SetPixelShaderConstantF(UINT StartRegister,
		CONST float *pConstantData,
		UINT Vector4fCount) override
	{
		return m_pd3dDev->SetPixelShaderConstantF(StartRegister,
			pConstantData, Vector4fCount);
	}

	HRESULT WINAPI SetPixelShaderConstantI(UINT StartRegister,
		CONST int *pConstantData,
		UINT Vector4iCount) override
	{
		return m_pd3dDev->SetPixelShaderConstantI(StartRegister,
			pConstantData, Vector4iCount);
	}

	HRESULT WINAPI SetSamplerState(DWORD Sampler,
		D3DSAMPLERSTATETYPE Type,
		DWORD Value) override
	{
		return m_pd3dDev->SetSamplerState(Sampler,
			Type, Value);
	}

	HRESULT WINAPI SetScissorRect(CONST RECT* pRect) override
	{
		return m_pd3dDev->SetScissorRect(pRect);
	}

	HRESULT WINAPI SetSoftwareVertexProcessing(BOOL bSoftware) override
	{
		return m_pd3dDev->SetSoftwareVertexProcessing(bSoftware);
	}

	HRESULT WINAPI SetStreamSourceFreq(UINT StreamNumber,
		UINT Divider) override
	{
		return m_pd3dDev->SetStreamSourceFreq(StreamNumber,
			Divider);
	}

	HRESULT WINAPI SetVertexDeclaration(IDirect3DVertexDeclaration9 *pDecl) override
	{
		return m_pd3dDev->SetVertexDeclaration(pDecl);
	}

	HRESULT WINAPI SetVertexShaderConstantB(UINT StartRegister,
		CONST BOOL *pConstantData,
		UINT BoolCount) override
	{
		return m_pd3dDev->SetVertexShaderConstantB(StartRegister,
			pConstantData, BoolCount);
	}

	HRESULT WINAPI SetVertexShaderConstantF(UINT StartRegister,
		CONST float *pConstantData,
		UINT Vector4fCount) override
	{
		return m_pd3dDev->SetVertexShaderConstantF(StartRegister,
			pConstantData, Vector4fCount);
	}

	HRESULT WINAPI SetVertexShaderConstantI(UINT StartRegister,
		CONST int *pConstantData,
		UINT Vector4iCount) override
	{
		return m_pd3dDev->SetVertexShaderConstantI(StartRegister,
			pConstantData, Vector4iCount);
	}

	HRESULT WINAPI StretchRect(IDirect3DSurface9 *pSourceSurface,
		CONST RECT *pSourceRect,
		IDirect3DSurface9 *pDestSurface,
		CONST RECT *pDestRect,
		D3DTEXTUREFILTERTYPE Filter) override
	{
		return m_pd3dDev->StretchRect(pSourceSurface, pSourceRect,
			pDestSurface, pDestRect, Filter);
	}

	HRESULT WINAPI UpdateSurface(Direct3DSurfaceBase* pSourceSurface,
		CONST RECT* pSourceRect,
		Direct3DSurfaceBase* pDestinationSurface,
		CONST POINT* pDestinationPoint) override
	{
		return m_pd3dDev->UpdateSurface(pSourceSurface, pSourceRect,
			pDestinationSurface, pDestinationPoint);
	}
#endif // #ifdef TOUHOU_ON_D3D8 #else
};

class THRotatorEditorContext
{
public:
	static std::shared_ptr<THRotatorEditorContext> CreateOrGetEditorContext(HWND hTouhouWindow)
	{
		auto foundItr = ms_touhouWinToContext.find(hTouhouWindow);
		if (foundItr != ms_touhouWinToContext.end())
		{
			return foundItr->second.lock();
		}

		std::shared_ptr<THRotatorEditorContext> ret(new THRotatorEditorContext(hTouhouWindow));
		if (!ret || !ret->m_bInitialized)
		{
			return nullptr;
		}

		ms_touhouWinToContext[hTouhouWindow] = ret;

		return ret;
	}

	int GetResetRevision() const
	{
		return m_deviceResetRevision;
	}

	void GetBackBufferResolution(Direct3DBase* pd3d, D3DFORMAT Format, UINT Adapter, BOOL bWindowed, UINT requestedWidth, UINT requestedHeight, UINT& outBackBufferWidth, UINT& outBackBufferHeight) const;

	// ビューポート設定回数を増やす
	void AddViewportSetCount()
	{
		m_judgeCount++;
	}

	// ビューポート設定回数をUIに渡し、設定回数をリセットして計測を再開
	void SubmitViewportSetCountToEditor()
	{
		m_judgeCountPrev = m_judgeCount;
		m_judgeCount = 0;
	}

	// ビューポート設定回数が閾値を超えたか
	bool IsViewportSetCountOverThreshold() const
	{
		return m_judgeThreshold <= m_judgeCount;
	}

	RotationAngle GetRotationAngle() const
	{
		return m_RotationAngle;
	}

	DWORD GetPlayRegionLeft() const
	{
		return m_playRegionLeft;
	}

	DWORD GetPlayRegionWidth() const
	{
		return m_playRegionWidth;
	}

	DWORD GetPlayRegionTop() const
	{
		return m_playRegionTop;
	}

	DWORD GetPlayRegionHeight() const
	{
		return m_playRegionHeight;
	}

	LONG GetYOffset() const
	{
		return m_yOffset;
	}

	const std::vector<RectTransferData> GetRectTransfers() const
	{
		return m_currentRectTransfers;
	}

	bool ConsumeScreenCaptureRequest()
	{
		if (!m_bScreenCaptureQueued)
		{
			return false;
		}

		m_bScreenCaptureQueued = false;
		return true;
	}

	const boost::filesystem::path& GetWorkingDirectory() const
	{
		return m_workingDir;
	}

	~THRotatorEditorContext()
	{
		ms_touhouWinToContext.erase(m_hTouhouWin);
	}

private:
	THRotatorEditorContext(HWND hTouhouWin)
		: m_hTouhouWin(hTouhouWin)
		, m_deviceResetRevision(0)
#ifdef TOUHOU_ON_D3D8
		, m_bNeedModalEditor(true)
#else
		, m_bNeedModalEditor(false)
#endif
		, m_modalEditorWindowPosX(0)
		, m_modalEditorWindowPosY(0)
		, m_bInitialized(false)
		, m_bScreenCaptureQueued(false)
	{
		static MessageHook messageHook;

		char fname[MAX_PATH];
		GetModuleFileName(NULL, fname, MAX_PATH);
		*strrchr(fname, '.') = '\0';
		boost::filesystem::path pth(fname);

		m_appName = std::string("THRotator_") + pth.filename().generic_string();
		char path[MAX_PATH];
		size_t retSize;
		errno_t en = getenv_s(&retSize, path, "APPDATA");

		double touhouIndex = 0.0;
		if (pth.filename().generic_string().compare("東方紅魔郷") == 0)
		{
			touhouIndex = 6.0;
		}
		else
		{
			char dummy[128];
			sscanf_s(pth.filename().generic_string().c_str(), "th%lf%s", &touhouIndex, dummy, (unsigned)_countof(dummy));
		}

		while (touhouIndex > 90.0)
		{
			touhouIndex /= 10.0;
		}

		if (touhouIndex > 12.3 && en == 0 && retSize > 0)
		{
			m_workingDir = boost::filesystem::path(path) / "ShanghaiAlice" / pth.filename();
			boost::filesystem::create_directory(m_workingDir);
		}
		else
		{
			m_workingDir = boost::filesystem::current_path();
		}
		m_iniPath = (m_workingDir / "throt.ini").string();

		LoadSettings();

		// スクリーンキャプチャ機能がないのは紅魔郷
		m_bTouhouWithoutScreenCapture = touhouIndex == 6.0;

		int bVerticallyLongWindow = m_bVerticallyLongWindow;
		m_bVerticallyLongWindow = 0;
		SetVerticallyLongWindow(bVerticallyLongWindow);

		m_currentRectTransfers = m_editedRectTransfers;

		//	メニューを改造
		HMENU hMenu = GetSystemMenu(m_hTouhouWin, FALSE);
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING, ms_switchVisibilityID, _T("THRotatorを開く"));

		m_hSysMenu = hMenu;

		if (m_bNeedModalEditor)
		{
			m_hEditorWin = nullptr;
		}
		else
		{
			HWND hWnd = m_hEditorWin = CreateDialogParam(reinterpret_cast<HINSTANCE>(g_hModule), MAKEINTRESOURCE(IDD_MAINDLG), NULL, MainDialogProc, (LPARAM)this);
			if (hWnd == NULL)
			{
				MessageBox(NULL, _T("ダイアログの作成に失敗"), NULL, MB_ICONSTOP);
			}
		}

		if (!m_bNeedModalEditor)
		{
			SetEditorWindowVisibility(m_bVisible);
		}

		m_bInitialized = true;
	}

	// エディタウィンドウの可視性を設定
	// エディタウィンドウをモーダルで開かなければならない場合は、
	// 既存のウィンドウの表示非表示を切り替えることはできないので、呼んではいけない
	void SetEditorWindowVisibility(BOOL bVisible);

	void InitListView(HWND hLV);

	BOOL ApplyChangeFromEditorWindow(HWND hWnd);
	BOOL ApplyChangeFromEditorWindow()
	{
		return ApplyChangeFromEditorWindow(m_hEditorWin);
	}

	void SaveSettings();

	void LoadSettings();

	void SetVerticallyLongWindow(BOOL bVerticallyLongWindow);

	static LRESULT CALLBACK MessageHookProc(int nCode, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK MainDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK EditRectDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	struct MessageHook
	{
		HHOOK m_hHook;

		MessageHook()
		{
			m_hHook = SetWindowsHookEx(WH_GETMESSAGE, THRotatorEditorContext::MessageHookProc, nullptr, GetCurrentThreadId());
			ms_hHook = m_hHook;
		}

		~MessageHook()
		{
			UnhookWindowsHookEx(m_hHook);
		}
	};

	boost::filesystem::path m_workingDir;

	/****************************************
	* THRotator parameters
	****************************************/

	DWORD m_playRegionLeft, m_playRegionTop, m_playRegionWidth, m_playRegionHeight;
	int m_yOffset;
	int m_judgeThreshold;
	int m_judgeCount, m_judgeCountPrev;
	BOOL m_bVisible;
	std::string m_iniPath;
	std::string m_appName;
	BOOL m_bVerticallyLongWindow;
	RotationAngle m_RotationAngle;
	std::vector<RectTransferData> m_editedRectTransfers, m_currentRectTransfers;
	D3DTEXTUREFILTERTYPE m_filterType;
	bool m_bTouhouWithoutScreenCapture;


	/****************************************
	* Class status
	****************************************/

	bool m_bInitialized;
	bool m_bScreenCaptureQueued; // スクリーンキャプチャのリクエストがあるか
	int m_deviceResetRevision; // 何番目のリセットを実施要求しているか


	/****************************************
	* Editor window
	****************************************/

	HMENU m_hSysMenu;
	HWND m_hEditorWin, m_hTouhouWin;
	bool m_bNeedModalEditor;
	int m_modalEditorWindowPosX, m_modalEditorWindowPosY;

	static HHOOK ms_hHook;
	static std::map<HWND, std::weak_ptr<THRotatorEditorContext>> ms_touhouWinToContext;
	static int ms_switchVisibilityID;
};

HHOOK THRotatorEditorContext::ms_hHook;
std::map<HWND, std::weak_ptr<THRotatorEditorContext>> THRotatorEditorContext::ms_touhouWinToContext;
int THRotatorEditorContext::ms_switchVisibilityID = 12345;

THRotatorDirect3D::THRotatorDirect3D()
	: m_pd3d(nullptr)
	, m_referenceCount(1)
{
}

THRotatorDirect3D::~THRotatorDirect3D()
{
	if (m_pd3d)
	{
		m_pd3d->Release();
	}
}

bool THRotatorDirect3D::init(UINT v)
{
#ifdef TOUHOU_ON_D3D8
	m_pd3d = p_Direct3DCreate8(v);
#else
	m_pd3d = p_Direct3DCreate9(v);
#endif
	return m_pd3d != NULL;
}

ULONG WINAPI THRotatorDirect3D::AddRef(VOID)
{
	return ++m_referenceCount;
}

HRESULT WINAPI THRotatorDirect3D::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	if (ppvObj == nullptr)
	{
		return E_POINTER;
	}
#ifdef TOUHOU_ON_D3D8
	else if (riid == IID_IDirect3D8
#else
	else if (riid == __uuidof(Direct3DBase)
#endif
		|| riid == __uuidof(IUnknown))
	{
		AddRef();
		*ppvObj = this;
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

ULONG WINAPI THRotatorDirect3D::Release()
{
	ULONG ret = --m_referenceCount;
	if (ret == 0)
	{
		delete this;
	}
	return ret;
}

HRESULT WINAPI THRotatorDirect3D::CreateDevice(UINT Adapter,
	D3DDEVTYPE DeviceType,
	HWND hFocusWindow,
	DWORD BehaviorFlags,
	D3DPRESENT_PARAMETERS* pPresentationParameters,
	Direct3DDeviceBase** ppReturnedDeviceInterface)
{
	D3DPRESENT_PARAMETERS d3dpp = *pPresentationParameters;

	auto pRetDev = new THRotatorDirect3DDevice();
	HRESULT ret = pRetDev->init(Adapter, this, DeviceType, hFocusWindow, BehaviorFlags,
		d3dpp, pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight);

	if (FAILED(ret))
	{
		pRetDev->Release();
		return ret;
	}
	*ppReturnedDeviceInterface = pRetDev;

	return ret;
}

THRotatorDirect3DDevice::THRotatorDirect3DDevice()
	: m_bInitialized(false)
	, m_referenceCount(1)
	, m_requestedWidth(0)
	, m_requestedHeight(0)
	, m_resetRevision(0)
#ifdef _DEBUG
	, m_FPSNew(60.f)
	, m_fpsCount(0)
#endif
{
#ifdef _DEBUG
	QueryPerformanceFrequency(&m_freq);
	ZeroMemory(&m_prev, sizeof(m_prev));
#endif
}

THRotatorDirect3DDevice::~THRotatorDirect3DDevice()
{
}

void THRotatorEditorContext::SetEditorWindowVisibility(BOOL bVisible)
{
	assert(!m_bNeedModalEditor);

	m_bVisible = bVisible;
	MENUITEMINFO mi;
	ZeroMemory(&mi, sizeof(mi));

	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STRING;
	mi.dwTypeData = bVisible ? _T("TH Rotatorのウィンドウを非表示") : _T("Th Rotatorのウィンドウを表示");
	SetMenuItemInfo(m_hSysMenu, ms_switchVisibilityID, FALSE, &mi);

	ShowWindow(m_hEditorWin, bVisible ? SW_SHOW : SW_HIDE);
}

void THRotatorEditorContext::InitListView(HWND hLV)
{
	ListView_DeleteAllItems(hLV);
	int i = 0;
	for (std::vector<RectTransferData>::iterator itr = m_editedRectTransfers.begin();
		itr != m_editedRectTransfers.end(); ++itr, ++i)
	{
		LVITEM lvi;
		ZeroMemory(&lvi, sizeof(lvi));

		lvi.mask = LVIF_TEXT;
		lvi.iItem = i;
		lvi.pszText = itr->name;
		ListView_InsertItem(hLV, &lvi);

		TCHAR str[64];
		lvi.pszText = str;

		lvi.iSubItem = 1;
		wsprintf(str, _T("%d,%d,%d,%d"), itr->rcSrc.left, itr->rcSrc.top, itr->rcSrc.right, itr->rcSrc.bottom);
		ListView_SetItem(hLV, &lvi);

		lvi.iSubItem = 2;
		wsprintf(str, _T("%d,%d,%d,%d"), itr->rcDest.left, itr->rcDest.top, itr->rcDest.right, itr->rcDest.bottom);
		ListView_SetItem(hLV, &lvi);

		lvi.iSubItem = 3;
		wsprintf(str, _T("%d°"), itr->rotation * 90);
		ListView_SetItem(hLV, &lvi);
	}
}

BOOL CALLBACK THRotatorEditorContext::MainDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		auto pContext = reinterpret_cast<THRotatorEditorContext*>(lParam);
		SetWindowLongPtr(hWnd, DWLP_USER, static_cast<LONG_PTR>(lParam));
		assert(lParam == GetWindowLongPtr(hWnd, DWLP_USER));

		SetDlgItemInt(hWnd, IDC_JUDGETHRESHOLD, pContext->m_judgeThreshold, TRUE);
		SetDlgItemInt(hWnd, IDC_PRLEFT, pContext->m_playRegionLeft, FALSE);
		SetDlgItemInt(hWnd, IDC_PRTOP, pContext->m_playRegionTop, FALSE);
		SetDlgItemInt(hWnd, IDC_PRWIDTH, pContext->m_playRegionWidth, FALSE);
		SetDlgItemInt(hWnd, IDC_PRHEIGHT, pContext->m_playRegionHeight, FALSE);
		SetDlgItemInt(hWnd, IDC_YOFFSET, pContext->m_yOffset, TRUE);

		{
			std::basic_string<TCHAR> versionUiString("Version: ");
			versionUiString += THROTATOR_VERSION_STRING;
			SetDlgItemText(hWnd, IDC_VERSION, versionUiString.c_str());
		}

		SendDlgItemMessage(hWnd, IDC_VISIBLE, BM_SETCHECK, pContext->m_bVisible ? BST_CHECKED : BST_UNCHECKED, 0);
		EnableWindow(GetDlgItem(hWnd, IDC_VISIBLE), !pContext->m_bNeedModalEditor);

		switch (pContext->m_filterType)
		{
		case D3DTEXF_NONE:
			SendDlgItemMessage(hWnd, IDC_FILTERNONE, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case D3DTEXF_LINEAR:
			SendDlgItemMessage(hWnd, IDC_FILTERBILINEAR, BM_SETCHECK, BST_CHECKED, 0);
			break;
		}

		if (pContext->m_bVerticallyLongWindow == 1)
			SendDlgItemMessage(hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_CHECKED, 0);

		switch (pContext->m_RotationAngle)
		{
		case Rotation_0:
			SendDlgItemMessage(hWnd, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case Rotation_90:
			SendDlgItemMessage(hWnd, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case Rotation_180:
			SendDlgItemMessage(hWnd, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case Rotation_270:
			SendDlgItemMessage(hWnd, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0);
			break;
		}

		{
			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;
			lvc.pszText = _T("矩形名");
			lvc.cx = 64;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 0, &lvc);
			lvc.pszText = _T("元の矩形");
			lvc.cx = 96;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 1, &lvc);
			lvc.pszText = _T("先の矩形");
			lvc.cx = 96;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 2, &lvc);
			lvc.pszText = _T("回転角");
			lvc.cx = 48;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_ORLIST), 3, &lvc);
		}

		pContext->InitListView(GetDlgItem(hWnd, IDC_ORLIST));

		return TRUE;
	}

	case WM_CLOSE:
		return FALSE;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_GETVPCOUNT:
			SetDlgItemInt(hWnd, IDC_VPCOUNT,
				reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER))->m_judgeCountPrev, FALSE);
			return TRUE;

		case IDOK:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			BOOL bResult = pContext->ApplyChangeFromEditorWindow(hWnd);
			if (!bResult)
			{
				return FALSE;
			}

			if (pContext->m_bNeedModalEditor)
			{
				RECT rc;
				GetWindowRect(hWnd, &rc);
				pContext->m_modalEditorWindowPosX = rc.top;
				pContext->m_modalEditorWindowPosY = rc.bottom;
				EndDialog(hWnd, IDOK);
			}
			return TRUE;
		}

		case IDC_RESET:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			SetDlgItemInt(hWnd, IDC_JUDGETHRESHOLD, pContext->m_judgeThreshold, FALSE);
			SetDlgItemInt(hWnd, IDC_PRLEFT, pContext->m_playRegionLeft, FALSE);
			SetDlgItemInt(hWnd, IDC_PRTOP, pContext->m_playRegionTop, FALSE);
			SetDlgItemInt(hWnd, IDC_PRWIDTH, pContext->m_playRegionWidth, FALSE);
			SetDlgItemInt(hWnd, IDC_PRHEIGHT, pContext->m_playRegionHeight, FALSE);
			SetDlgItemInt(hWnd, IDC_YOFFSET, pContext->m_yOffset, TRUE);
			pContext->m_editedRectTransfers = pContext->m_currentRectTransfers;
			pContext->InitListView(GetDlgItem(hWnd, IDC_ORLIST));
			SendDlgItemMessage(hWnd, IDC_VISIBLE, BM_SETCHECK, pContext->m_bVisible ? BST_CHECKED : BST_UNCHECKED, 0);
			if (pContext->m_bVerticallyLongWindow == 1)
				SendDlgItemMessage(hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_CHECKED, 0);
			else
				SendDlgItemMessage(hWnd, IDC_VERTICALWINDOW, BM_SETCHECK, BST_UNCHECKED, 0);

			switch (pContext->m_RotationAngle)
			{
			case 0:
				SendDlgItemMessage(hWnd, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 1:
				SendDlgItemMessage(hWnd, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 2:
				SendDlgItemMessage(hWnd, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 3:
				SendDlgItemMessage(hWnd, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
				break;
			}
			switch (pContext->m_filterType)
			{
			case D3DTEXF_NONE:
				SendDlgItemMessage(hWnd, IDC_FILTERNONE, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_FILTERBILINEAR, BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case D3DTEXF_LINEAR:
				SendDlgItemMessage(hWnd, IDC_FILTERBILINEAR, BM_SETCHECK, BST_CHECKED, 0);
				SendDlgItemMessage(hWnd, IDC_FILTERNONE, BM_SETCHECK, BST_UNCHECKED, 0);
				break;
			}
			return TRUE;
		}

		case IDCANCEL:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			if (pContext->m_bNeedModalEditor)
			{
				RECT rc;
				GetWindowRect(hWnd, &rc);
				pContext->m_modalEditorWindowPosX = rc.top;
				pContext->m_modalEditorWindowPosY = rc.bottom;
				EndDialog(hWnd, IDCANCEL);
			}
			else
			{
				pContext->SetEditorWindowVisibility(FALSE);
			}
			return TRUE;
		}

		case IDC_ADDRECT:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			RectTransferData erd;
			ZeroMemory(&erd, sizeof(erd));
		reedit1:
			if (IDOK == DialogBoxParam((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_EDITRECTDLG), hWnd, EditRectDialogProc, reinterpret_cast<LPARAM>(&erd)))
			{
				for (std::vector<RectTransferData>::const_iterator itr = pContext->m_editedRectTransfers.cbegin(); itr != pContext->m_editedRectTransfers.cend();
					++itr)
				{
					if (lstrcmp(itr->name, erd.name) == 0)
					{
						MessageBox(hWnd, _T("この矩形名はすでに存在しています。別の矩形名前を入力してください。"), NULL, MB_ICONSTOP);
						goto reedit1;
					}
				}
				TCHAR str[64];
				pContext->m_editedRectTransfers.push_back(erd);
				LVITEM lvi;
				ZeroMemory(&lvi, sizeof(lvi));
				lvi.mask = LVIF_TEXT;
				lvi.iItem = ListView_GetItemCount(GetDlgItem(hWnd, IDC_ORLIST));
				lvi.pszText = erd.name;
				ListView_InsertItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.pszText = str;

				lvi.iSubItem = 1;
				wsprintf(str, _T("%d,%d,%d,%d"), erd.rcSrc.left, erd.rcSrc.top, erd.rcSrc.right, erd.rcSrc.bottom);
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.iSubItem = 2;
				wsprintf(str, _T("%d,%d,%d,%d"), erd.rcDest.left, erd.rcDest.top, erd.rcDest.right, erd.rcDest.bottom);
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.iSubItem = 3;
				switch (erd.rotation)
				{
				case 0:
					lvi.pszText = _T("0°");
					break;

				case 1:
					lvi.pszText = _T("90°");
					break;

				case 2:
					lvi.pszText = _T("180°");
					break;

				case 3:
					lvi.pszText = _T("270°");
					break;
				}
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);
			}
		}
		return TRUE;

		case IDC_EDITRECT:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			int i = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST));
			if (i == -1)
			{
				MessageBox(hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			RectTransferData& erd = pContext->m_editedRectTransfers[i];
		reedit2:
			if (IDOK == DialogBoxParam((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_EDITRECTDLG), hWnd, EditRectDialogProc, reinterpret_cast<LPARAM>(&erd)))
			{
				for (std::vector<RectTransferData>::const_iterator itr = pContext->m_editedRectTransfers.cbegin(); itr != pContext->m_editedRectTransfers.cend();
					++itr)
				{
					if (itr == pContext->m_editedRectTransfers.begin() + i)
						continue;
					if (lstrcmp(itr->name, erd.name) == 0)
					{
						MessageBox(hWnd, _T("この矩形名はすでに存在しています。別の矩形名前を入力してください。"), NULL, MB_ICONSTOP);
						goto reedit2;
					}
				}
				TCHAR str[64];
				LVITEM lvi;
				ZeroMemory(&lvi, sizeof(lvi));
				lvi.mask = LVIF_TEXT;
				lvi.iItem = i;
				lvi.pszText = erd.name;
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.pszText = str;

				lvi.iSubItem = 1;
				wsprintf(str, _T("%d,%d,%d,%d"), erd.rcSrc.left, erd.rcSrc.top, erd.rcSrc.right, erd.rcSrc.bottom);
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.iSubItem = 2;
				wsprintf(str, _T("%d,%d,%d,%d"), erd.rcDest.left, erd.rcDest.top, erd.rcDest.right, erd.rcDest.bottom);
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);

				lvi.iSubItem = 3;
				switch (erd.rotation)
				{
				case 0:
					lvi.pszText = _T("0°");
					break;

				case 1:
					lvi.pszText = _T("90°");
					break;

				case 2:
					lvi.pszText = _T("180°");
					break;

				case 3:
					lvi.pszText = _T("270°");
					break;
				}
				ListView_SetItem(GetDlgItem(hWnd, IDC_ORLIST), &lvi);
			}
			return TRUE;
		}

		case IDC_REMRECT:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			int i = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST));
			if (i == -1)
			{
				MessageBox(hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			pContext->m_editedRectTransfers.erase(pContext->m_editedRectTransfers.cbegin() + i);
			ListView_DeleteItem(GetDlgItem(hWnd, IDC_ORLIST), i);
			if (ListView_GetItemCount(GetDlgItem(hWnd, IDC_ORLIST)) == 0)
			{
				EnableWindow(GetDlgItem(hWnd, IDC_EDITRECT), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_REMRECT), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_UP), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_DOWN), FALSE);
			}
			return TRUE;
		}

		case IDC_UP:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			int i = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST));
			if (i == -1)
			{
				MessageBox(hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			if (i == 0)
			{
				MessageBox(hWnd, _T("選択されている矩形が一番上のものです。"), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			RectTransferData erd = pContext->m_editedRectTransfers[i];
			pContext->m_editedRectTransfers[i] = pContext->m_editedRectTransfers[i - 1];
			pContext->m_editedRectTransfers[i - 1] = erd;
			pContext->InitListView(GetDlgItem(hWnd, IDC_ORLIST));
			ListView_SetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST), i - 1);
			EnableWindow(GetDlgItem(hWnd, IDC_DOWN), TRUE);
			if (i == 1)
				EnableWindow(GetDlgItem(hWnd, IDC_UP), FALSE);
			return TRUE;
		}

		case IDC_DOWN:
		{
			auto pContext = reinterpret_cast<THRotatorEditorContext*>(GetWindowLongPtr(hWnd, DWLP_USER));

			int i = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST));
			int count = ListView_GetItemCount(GetDlgItem(hWnd, IDC_ORLIST));
			if (i == -1)
			{
				MessageBox(hWnd, _T("矩形が選択されていません。"), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			if (i == count - 1)
			{
				MessageBox(hWnd, _T("選択されている矩形が一番下のものです。"), NULL, MB_ICONEXCLAMATION);
				return FALSE;
			}
			RectTransferData erd = pContext->m_editedRectTransfers[i];
			pContext->m_editedRectTransfers[i] = pContext->m_editedRectTransfers[i + 1];
			pContext->m_editedRectTransfers[i + 1] = erd;
			pContext->InitListView(GetDlgItem(hWnd, IDC_ORLIST));
			ListView_SetSelectionMark(GetDlgItem(hWnd, IDC_ORLIST), i + 1);
			EnableWindow(GetDlgItem(hWnd, IDC_UP), TRUE);
			if (i == count - 2)
				EnableWindow(GetDlgItem(hWnd, IDC_DOWN), FALSE);
			return TRUE;
		}
		}
		return FALSE;

	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->code == LVN_ITEMCHANGED)
		{
			LPNMLISTVIEW lpnmLV = (LPNMLISTVIEW)lParam;
			int count = ListView_GetItemCount(GetDlgItem(hWnd, IDC_ORLIST));
			BOOL bEnabled = count > 0;
			EnableWindow(GetDlgItem(hWnd, IDC_EDITRECT), bEnabled);
			EnableWindow(GetDlgItem(hWnd, IDC_REMRECT), bEnabled);
			EnableWindow(GetDlgItem(hWnd, IDC_UP), bEnabled && lpnmLV->iItem > 0);
			EnableWindow(GetDlgItem(hWnd, IDC_DOWN), bEnabled && lpnmLV->iItem < count - 1);
		}
		return FALSE;

	default:
		break;
	}

	return FALSE;
}

BOOL CALLBACK THRotatorEditorContext::EditRectDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		SetWindowLongPtr(hWnd, DWLP_USER, static_cast<LONG_PTR>(lParam));
		RectTransferData* pErd = reinterpret_cast<RectTransferData*>(lParam);

		SetDlgItemInt(hWnd, IDC_SRCLEFT, pErd->rcSrc.left, TRUE);
		SetDlgItemInt(hWnd, IDC_SRCWIDTH, pErd->rcSrc.right, TRUE);
		SetDlgItemInt(hWnd, IDC_SRCTOP, pErd->rcSrc.top, TRUE);
		SetDlgItemInt(hWnd, IDC_SRCHEIGHT, pErd->rcSrc.bottom, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTLEFT, pErd->rcDest.left, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTWIDTH, pErd->rcDest.right, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTTOP, pErd->rcDest.top, TRUE);
		SetDlgItemInt(hWnd, IDC_DESTHEIGHT, pErd->rcDest.bottom, TRUE);
		switch (pErd->rotation)
		{
		case 0:
			SendDlgItemMessage(hWnd, IDC_ROT0, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case 1:
			SendDlgItemMessage(hWnd, IDC_ROT90, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case 2:
			SendDlgItemMessage(hWnd, IDC_ROT180, BM_SETCHECK, BST_CHECKED, 0);
			break;

		case 3:
			SendDlgItemMessage(hWnd, IDC_ROT270, BM_SETCHECK, BST_CHECKED, 0);
			break;
		}

		SetDlgItemText(hWnd, IDC_RECTNAME, pErd->name);

		return TRUE;
	}

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
		{
#define GETANDSET_E(id,var,bSig,errmsg) {\
					BOOL bRet;UINT ret;\
					ret = GetDlgItemInt( hWnd, id, &bRet, bSig );\
					if( !bRet )\
					{\
						MessageBox( hWnd, errmsg, NULL, MB_ICONSTOP );\
						return FALSE;\
					}\
					if( bSig )\
						var=(signed)ret;\
					else\
						var=ret;\
				}

			RECT rcSrc, rcDest;
			GETANDSET_E(IDC_SRCLEFT, rcSrc.left, TRUE, _T("矩形転送元の左座標の入力が不正です。"));
			GETANDSET_E(IDC_SRCTOP, rcSrc.top, TRUE, _T("矩形転送元の上座標の入力が不正です。"));
			GETANDSET_E(IDC_SRCWIDTH, rcSrc.right, TRUE, _T("矩形転送元の幅の入力が不正です。"));
			GETANDSET_E(IDC_SRCHEIGHT, rcSrc.bottom, TRUE, _T("矩形転送元の高さの入力が不正です。"));
			GETANDSET_E(IDC_DESTLEFT, rcDest.left, TRUE, _T("矩形転送先の左座標の入力が不正です。"));
			GETANDSET_E(IDC_DESTTOP, rcDest.top, TRUE, _T("矩形転送先の上座標の入力が不正です。"));
			GETANDSET_E(IDC_DESTWIDTH, rcDest.right, TRUE, _T("矩形転送先の幅の入力が不正です。"));
			GETANDSET_E(IDC_DESTHEIGHT, rcDest.bottom, TRUE, _T("矩形転送先の高さの入力が不正です。"));
#undef GETANDSET_E

			RectTransferData* pErd = reinterpret_cast<RectTransferData*>(GetWindowLongPtr(hWnd, DWLP_USER));

			if (rcSrc.right == 0 || rcSrc.bottom == 0 || rcDest.right == 0 || rcDest.bottom == 0)
			{
				MessageBox(hWnd, _T("入力された幅と高さに0が含まれています。この場合、矩形は描画されません。\r\n描画するにはあとで変更してください。"), NULL, MB_ICONEXCLAMATION);
			}

			if (SendDlgItemMessage(hWnd, IDC_ROT0, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				pErd->rotation = Rotation_0;
			}
			else if (SendDlgItemMessage(hWnd, IDC_ROT90, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				pErd->rotation = Rotation_90;
			}
			else if (SendDlgItemMessage(hWnd, IDC_ROT180, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				pErd->rotation = Rotation_180;
			}
			else if (SendDlgItemMessage(hWnd, IDC_ROT270, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				pErd->rotation = Rotation_270;
			}
			pErd->rcSrc = rcSrc;
			pErd->rcDest = rcDest;
			GetDlgItemText(hWnd, IDC_RECTNAME,
				pErd->name, sizeof(((RectTransferData*)NULL)->name) / sizeof(TCHAR));

			EndDialog(hWnd, IDOK);
			return TRUE;
		}

		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

BOOL THRotatorEditorContext::ApplyChangeFromEditorWindow(HWND hEditorWin)
{
	assert(hEditorWin);

#define GETANDSET(id,var,bSig,errmsg) {\
			BOOL bRet;UINT ret;\
			ret = GetDlgItemInt( hEditorWin, id, &bRet, bSig );\
			if( !bRet )\
			{\
				MessageBox( hEditorWin, errmsg, NULL, MB_ICONSTOP );\
				return FALSE;\
			}\
			if( bSig )\
				var=(signed)ret;\
			else\
				var=ret;\
			SetDlgItemInt( hEditorWin, id, ret, bSig );\
		}

	DWORD pl, pt, pw, ph;
	int jthres, yo;
	GETANDSET(IDC_JUDGETHRESHOLD, jthres, TRUE, _T("判定の回数閾値の入力が不正です。"));
	GETANDSET(IDC_PRLEFT, pl, FALSE, _T("プレイ画面領域の左座標の入力が不正です。"));
	GETANDSET(IDC_PRTOP, pt, FALSE, _T("プレイ画面領域の上座標の入力が不正です。"));
	GETANDSET(IDC_PRWIDTH, pw, FALSE, _T("プレイ画面領域の幅の入力が不正です。"));
	if (pw == 0)
	{
		MessageBox(hEditorWin, _T("プレイ画面領域の幅は0以外の値でなければいけません。"), NULL, MB_ICONSTOP);
		return FALSE;
	}
	GETANDSET(IDC_PRHEIGHT, ph, FALSE, _T("プレイ画面領域の高さの入力が不正です。"));
	if (ph == 0)
	{
		MessageBox(hEditorWin, _T("プレイ画面領域の高さは0以外の値でなければいけません。"), NULL, MB_ICONSTOP);
		return FALSE;
	}
	GETANDSET(IDC_YOFFSET, yo, TRUE, _T("プレイ画面領域のオフセットの入力が不正です。"));

	m_judgeThreshold = jthres;
	m_playRegionLeft = pl;
	m_playRegionTop = pt;
	m_playRegionWidth = pw;
	m_playRegionHeight = ph;
	m_yOffset = yo;
#undef GETANDSET

	BOOL bVerticallyLong = BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_VERTICALWINDOW, BM_GETCHECK, 0, 0);
	SetVerticallyLongWindow(bVerticallyLong);

	BOOL bFilterTypeNone = BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_FILTERNONE, BM_GETCHECK, 0, 0);
	m_filterType = bFilterTypeNone ? D3DTEXF_NONE : D3DTEXF_LINEAR;

	if (BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_ROT0_2, BM_GETCHECK, 0, 0))
	{
		m_RotationAngle = Rotation_0;
	}
	else if (BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_ROT90_2, BM_GETCHECK, 0, 0))
	{
		m_RotationAngle = Rotation_90;
	}
	else if (BST_CHECKED == SendDlgItemMessage(hEditorWin, IDC_ROT180_2, BM_GETCHECK, 0, 0))
	{
		m_RotationAngle = Rotation_180;
	}
	else
	{
		m_RotationAngle = Rotation_270;
	}

	m_bVisible = SendDlgItemMessage(hEditorWin, IDC_VISIBLE, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;

	m_currentRectTransfers = m_editedRectTransfers;

	SaveSettings();

	return TRUE;
}

void THRotatorEditorContext::SaveSettings()
{
	namespace proptree = boost::property_tree;
	proptree::ptree tree;

#define WRITE_INI_PARAM(name, value) tree.add(m_appName + "." + name, value)
	WRITE_INI_PARAM("JC", m_judgeThreshold);
	WRITE_INI_PARAM("PL", m_playRegionLeft);
	WRITE_INI_PARAM("PT", m_playRegionTop);
	WRITE_INI_PARAM("PW", m_playRegionWidth);
	WRITE_INI_PARAM("PH", m_playRegionHeight);
	WRITE_INI_PARAM("YOffset", m_yOffset);
	WRITE_INI_PARAM("Visible", m_bVisible);
	WRITE_INI_PARAM("PivRot", m_bVerticallyLongWindow);
	WRITE_INI_PARAM("Filter", m_filterType);
	WRITE_INI_PARAM("Rot", m_RotationAngle);

	int i = 0;
	TCHAR name[64];
	for (std::vector<RectTransferData>::iterator itr = m_currentRectTransfers.begin();
		itr != m_currentRectTransfers.cend(); ++itr, ++i)
	{
		wsprintf(name, _T("Name%d"), i); WRITE_INI_PARAM(name, itr->name);
		wsprintf(name, _T("OSL%d"), i); WRITE_INI_PARAM(name, itr->rcSrc.left);
		wsprintf(name, _T("OST%d"), i); WRITE_INI_PARAM(name, itr->rcSrc.top);
		wsprintf(name, _T("OSW%d"), i); WRITE_INI_PARAM(name, itr->rcSrc.right);
		wsprintf(name, _T("OSH%d"), i); WRITE_INI_PARAM(name, itr->rcSrc.bottom);
		wsprintf(name, _T("ODL%d"), i); WRITE_INI_PARAM(name, itr->rcDest.left);
		wsprintf(name, _T("ODT%d"), i); WRITE_INI_PARAM(name, itr->rcDest.top);
		wsprintf(name, _T("ODW%d"), i); WRITE_INI_PARAM(name, itr->rcDest.right);
		wsprintf(name, _T("ODH%d"), i); WRITE_INI_PARAM(name, itr->rcDest.bottom);
		wsprintf(name, _T("OR%d"), i); WRITE_INI_PARAM(name, itr->rotation);
		wsprintf(name, _T("ORHas%d"), i); WRITE_INI_PARAM(name, TRUE);
	}
	wsprintf(name, _T("ORHas%d"), i); WRITE_INI_PARAM(name, FALSE);
#undef WRITE_INI_PARAM

	proptree::write_ini(m_iniPath, tree);
}

void THRotatorEditorContext::LoadSettings()
{
	namespace proptree = boost::property_tree;

	proptree::ptree tree;
	try
	{
		proptree::read_ini(m_iniPath, tree);
	}
	catch (const proptree::ptree_error&)
	{
		// ファイルオープン失敗だが、boost::optional::get_value_or()でデフォルト値を設定できるので、そのまま進行
	}

#define READ_INI_PARAM(type, name, default_value) tree.get_optional<type>(m_appName + "." + name).get_value_or(default_value)
	m_judgeThreshold = READ_INI_PARAM(int, "JC", 999);
	m_playRegionLeft = READ_INI_PARAM(int, "PL", 32);
	m_playRegionTop = READ_INI_PARAM(int, "PT", 16);
	m_playRegionWidth = READ_INI_PARAM(int, "PW", 384);
	m_playRegionHeight = READ_INI_PARAM(int, "PH", 448);
	m_yOffset = READ_INI_PARAM(int, "YOffset", 0);
	m_bVisible = READ_INI_PARAM(BOOL, "Visible", FALSE);
	m_bVerticallyLongWindow = READ_INI_PARAM(BOOL, "PivRot", FALSE);
	m_RotationAngle = static_cast<RotationAngle>(READ_INI_PARAM(std::uint32_t, "Rot", Rotation_0));
	m_filterType = static_cast<D3DTEXTUREFILTERTYPE>(READ_INI_PARAM(int, "Filter", D3DTEXF_LINEAR));

	BOOL bHasNext = READ_INI_PARAM(BOOL, "ORHas0", FALSE);
	int cnt = 0;
	while (bHasNext)
	{
		RectTransferData erd;
		TCHAR name[64];

		wsprintf(name, _T("Name%d"), cnt);
		std::basic_string<TCHAR> rectName = READ_INI_PARAM(std::basic_string<TCHAR>, name, "");
		strcpy_s(erd.name, rectName.c_str());
		erd.name[rectName.length()] = '\0';

		wsprintf(name, _T("OSL%d"), cnt);
		erd.rcSrc.left = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("OST%d"), cnt);
		erd.rcSrc.top = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("OSW%d"), cnt);
		erd.rcSrc.right = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("OSH%d"), cnt);
		erd.rcSrc.bottom = READ_INI_PARAM(int, name, 0);

		wsprintf(name, _T("ODL%d"), cnt);
		erd.rcDest.left = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("ODT%d"), cnt);
		erd.rcDest.top = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("ODW%d"), cnt);
		erd.rcDest.right = READ_INI_PARAM(int, name, 0);
		wsprintf(name, _T("ODH%d"), cnt);
		erd.rcDest.bottom = READ_INI_PARAM(int, name, 0);

		wsprintf(name, _T("OR%d"), cnt);
		erd.rotation = static_cast<RotationAngle>(READ_INI_PARAM(int, name, 0));

		m_editedRectTransfers.push_back(erd);
		cnt++;

		wsprintf(name, _T("ORHas%d"), cnt);
		bHasNext = READ_INI_PARAM(BOOL, name, FALSE);
	}
#undef READ_INI_PARAM
}

LRESULT CALLBACK THRotatorEditorContext::MessageHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPMSG pMsg = reinterpret_cast<LPMSG>(lParam);
	if (nCode >= 0 && PM_REMOVE == wParam)
	{
		for (auto itr = ms_touhouWinToContext.begin();
			itr != ms_touhouWinToContext.end(); ++itr)
		{
			// Don't translate non-input events.
			if ((pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST))
			{
				if (!itr->second.expired() && IsDialogMessage(itr->second.lock()->m_hEditorWin, pMsg))
				{
					// The value returned from this hookproc is ignored, 
					// and it cannot be used to tell Windows the message has been handled.
					// To avoid further processing, convert the message to WM_NULL 
					// before returning.
					pMsg->message = WM_NULL;
					pMsg->lParam = 0;
					pMsg->wParam = 0;
				}
			}
		}
	}
	if (nCode == HC_ACTION)
	{
		auto foundItr = ms_touhouWinToContext.find(pMsg->hwnd);
		if (foundItr != ms_touhouWinToContext.end() && !foundItr->second.expired())
		{
			auto context = foundItr->second.lock();
			switch (pMsg->message)
			{
			case WM_SYSCOMMAND:
				if (pMsg->wParam == ms_switchVisibilityID)
				{
					if (context->m_bNeedModalEditor)
					{
						DialogBoxParam((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_MAINDLG), pMsg->hwnd, MainDialogProc, (LPARAM)context.get());
					}
					else
					{
						context->SetEditorWindowVisibility(!context->m_bVisible);
					}
				}
				break;

			case WM_KEYDOWN:
				switch (pMsg->wParam)
				{
				case VK_HOME:
					if (context->m_bTouhouWithoutScreenCapture && (HIWORD(pMsg->lParam) & KF_REPEAT) == 0)
					{
						context->m_bScreenCaptureQueued = true;
					}
					break;
				}
				break;

			case WM_SYSKEYDOWN:
				switch (pMsg->wParam)
				{
				case VK_LEFT:
					if ((HIWORD(pMsg->lParam) & KF_ALTDOWN) && !(HIWORD(pMsg->lParam) & KF_REPEAT))
					{
						auto nextRotationAngle = static_cast<RotationAngle>((context->m_RotationAngle + 1) % 4);

						if (context->m_bNeedModalEditor)
						{
							context->m_RotationAngle = nextRotationAngle;
							context->SaveSettings();
							break;
						}

						switch ((context->m_RotationAngle + 1) % 4)
						{
						case Rotation_0:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( pDev->m_d3dpp.Windowed ) pDev->SetVerticallyLongWindow( 0 );
							break;

						case 1:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 1 );
							break;

						case 2:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 0 );
							break;

						case 3:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 1 );
							break;
						}

						context->ApplyChangeFromEditorWindow();
					}
					break;

				case VK_RIGHT:
					if (HIWORD(pMsg->lParam) & KF_ALTDOWN && !(HIWORD(pMsg->lParam) & KF_REPEAT))
					{
						auto nextRotationAngle = static_cast<RotationAngle>((context->m_RotationAngle + Rotation_Num - 1) % 4);

						if (context->m_bNeedModalEditor)
						{
							context->m_RotationAngle = nextRotationAngle;
							context->SaveSettings();
							break;
						}

						switch (nextRotationAngle)
						{
						case 0:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 0 );
							break;

						case 1:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 1 );
							break;

						case 2:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_CHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_UNCHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 0 );
							break;

						case 3:
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT0_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT90_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT180_2, BM_SETCHECK, BST_UNCHECKED, 0);
							SendDlgItemMessage(context->m_hEditorWin, IDC_ROT270_2, BM_SETCHECK, BST_CHECKED, 0);
							//if( context->m_d3dpp.Windowed ) context->SetVerticallyLongWindow( 1 );
							break;
						}

						context->ApplyChangeFromEditorWindow();
					}
					break;
				}
				break;
			}
		}
	}
	return CallNextHookEx(ms_hHook, nCode, wParam, lParam);
}

void THRotatorEditorContext::SetVerticallyLongWindow(BOOL bVerticallyLongWindow)
{
	if (m_bVerticallyLongWindow % 2 != bVerticallyLongWindow % 2/* && m_d3dpp.Windowed*/)
	{
		RECT rcClient, rcWindow;
		GetClientRect(m_hTouhouWin, &rcClient);
		GetWindowRect(m_hTouhouWin, &rcWindow);

		MoveWindow(m_hTouhouWin, rcWindow.left, rcWindow.top,
			(rcWindow.right - rcWindow.left) - (rcClient.right - rcClient.left) + (rcClient.bottom - rcClient.top),
			(rcWindow.bottom - rcWindow.top) - (rcClient.bottom - rcClient.top) + (rcClient.right - rcClient.left), TRUE);

		m_deviceResetRevision++;
	}
	SendDlgItemMessage(m_hEditorWin, IDC_VERTICALWINDOW, BM_SETCHECK, bVerticallyLongWindow ? BST_CHECKED : BST_UNCHECKED, 0);
	m_bVerticallyLongWindow = bVerticallyLongWindow;
}

HRESULT THRotatorDirect3DDevice::init(UINT Adapter, THRotatorDirect3D* pMyD3D, D3DDEVTYPE DeviceType, HWND hFocusWindow,
	DWORD BehaviorFlags, const D3DPRESENT_PARAMETERS& d3dpp,
	UINT requestedWidth, UINT requestedHeight)
{
	m_d3dpp = d3dpp;

	m_requestedWidth = requestedWidth;
	m_requestedHeight = requestedHeight;

	m_pEditorContext = THRotatorEditorContext::CreateOrGetEditorContext(hFocusWindow);
	if (!m_pEditorContext)
	{
		return E_FAIL;
	}

	m_resetRevision = m_pEditorContext->GetResetRevision();

	m_pEditorContext->GetBackBufferResolution(pMyD3D->m_pd3d, d3dpp.BackBufferFormat, Adapter, d3dpp.Windowed,
		m_requestedWidth, m_requestedHeight, m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight);

	HRESULT ret = pMyD3D->m_pd3d->CreateDevice(Adapter,
		DeviceType, hFocusWindow, BehaviorFlags,
		&m_d3dpp, &m_pd3dDev);
	if (FAILED(ret))
	{
		return ret;
	}

	ret = InitResources();
	if (FAILED(ret))
	{
		return ret;
	}

	m_pMyD3D = pMyD3D;

	m_bInitialized = true;
	return S_OK;
}

HRESULT THRotatorDirect3DDevice::InitResources()
{
	HRESULT hr = m_pd3dDev->CreateTexture(m_requestedWidth, m_requestedHeight, 1,
		D3DUSAGE_RENDERTARGET, m_d3dpp.BackBufferFormat, D3DPOOL_DEFAULT,
#ifdef TOUHOU_ON_D3D8
		&m_pTex);
#else
		&m_pTex, NULL);
#endif
	if (FAILED(hr))
	{
		return hr;
	}

	m_pTex->GetSurfaceLevel(0, &m_pTexSurface);

	if (m_pSprite && FAILED(hr = m_pSprite->OnResetDevice()))
	{
		return hr;
	}
	else if (FAILED(hr = D3DXCreateSprite(m_pd3dDev.Get(), &m_pSprite)))
	{
		return hr;
	}

#if defined(_DEBUG) && !defined(TOUHOU_ON_D3D8)
	if (m_pFont && FAILED(hr = m_pFont->OnResetDevice()))
	{
		return hr;
	}
	else if (FAILED(hr = D3DXCreateFont(m_pd3dDev.Get(),
		0, 0, 0, 0, 0, 0, 0, 0, 0, "Arial", &m_pFont)))
	{
		return hr;
	}
#endif

	hr = m_pd3dDev->CreateRenderTarget(m_requestedWidth, m_requestedHeight,
		m_d3dpp.BackBufferFormat, m_d3dpp.MultiSampleType,
#ifdef TOUHOU_ON_D3D8
		TRUE, &m_pRenderTarget);
#else
		m_d3dpp.MultiSampleQuality, TRUE, &m_pRenderTarget, nullptr);
#endif

	if (FAILED(hr))
	{
		return hr;
	}

#ifdef TOUHOU_ON_D3D8
	ComPtr<Direct3DSurfaceBase> pSurf;

	//	深さバッファの作成
	if (FAILED(hr = m_pd3dDev->GetDepthStencilSurface(&pSurf)))
	{
		return hr;
	}

	if (FAILED(hr = pSurf->GetDesc(&m_surfDesc)))
	{
		return hr;
	}

	if (FAILED(hr = m_pd3dDev->CreateDepthStencilSurface(m_requestedWidth, m_requestedHeight,
		m_surfDesc.Format, m_surfDesc.MultiSampleType, &m_pDepthStencil)))
	{
		return hr;
	}

	if (FAILED(hr = m_pd3dDev->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer)))
	{
		return hr;
	}

	if (FAILED(hr = m_pd3dDev->SetRenderTarget(m_pRenderTarget.Get(), m_pDepthStencil.Get())))
	{
		return hr;
	}
#else
	if (FAILED(hr = m_pd3dDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer)))
	{
		return hr;
	}

	if (FAILED(hr = m_pd3dDev->SetRenderTarget(0, m_pRenderTarget.Get())))
	{
		return hr;
	}
#endif

	return S_OK;
}

void THRotatorDirect3DDevice::ReleaseResources()
{
#if TOUHOU_ON_D3D8
	m_pSprite.Reset();
	m_pDepthStencil.Reset();
#else
	if (m_pSprite)
	{
		m_pSprite->OnLostDevice();
	}
#ifdef _DEBUG
	if (m_pFont)
	{
		m_pFont->OnLostDevice();
	}
#endif
#endif

	m_pRenderTarget.Reset();
	m_pTexSurface.Reset();
	m_pTex.Reset();
	m_pBackBuffer.Reset();
}

void THRotatorEditorContext::GetBackBufferResolution(Direct3DBase* pd3d, D3DFORMAT Format, UINT Adapter, BOOL bWindowed, UINT requestedWidth, UINT requestedHeight, UINT& outBackBufferWidth, UINT& outBackBufferHeight) const
{
	if (bWindowed)
	{
		RECT rc;
		GetClientRect(m_hTouhouWin, &rc);
		outBackBufferWidth = rc.right - rc.left;
		outBackBufferHeight = rc.bottom - rc.top;

		// バックバッファの幅がリクエストの幅より小さい場合、バッファに何も描画されなくなるため、
		// バックバッファの幅がリクエストの幅以上になるように補正
		if (outBackBufferWidth < outBackBufferHeight)
		{
			UINT newWidth = outBackBufferHeight;

			// アスペクト比を掛けて小数点以下が発生しないようにする
			while ((newWidth * requestedWidth) % requestedHeight != 0)
			{
				newWidth++;
			}

			outBackBufferHeight = newWidth * requestedWidth / requestedHeight;
			outBackBufferWidth = newWidth;
		}
	}
	else
	{
		outBackBufferWidth = GetSystemMetrics(SM_CXSCREEN);
		outBackBufferHeight = GetSystemMetrics(SM_CYSCREEN);

		D3DDISPLAYMODE d3ddm;
		UINT count = pd3d->GetAdapterModeCount(
#ifdef TOUHOU_ON_D3D8
			Adapter);
#else
			Adapter, Format);
#endif
		std::vector<D3DDISPLAYMODE> availableModes;

		//	ディスプレイの最大解像度を探す＆60Hzの解像度を列挙
		for (UINT i = 0; i < count; ++i)
		{
			pd3d->EnumAdapterModes(Adapter,
#ifndef TOUHOU_ON_D3D8
				Format,
#endif
				i, &d3ddm);
			if (d3ddm.RefreshRate == 60)
			{
				availableModes.push_back(d3ddm);
				if (outBackBufferWidth*outBackBufferHeight < d3ddm.Width*d3ddm.Height)
				{
					outBackBufferWidth = d3ddm.Width;
					outBackBufferHeight = d3ddm.Height;
				}
			}
		}

		//	最大解像度と同じアスペクト比で、できるだけ低い解像度を選ぶ
		for (std::vector<D3DDISPLAYMODE>::const_iterator itr = availableModes.cbegin();
			itr != availableModes.cend(); ++itr)
		{
			if (outBackBufferWidth * itr->Height != outBackBufferHeight * itr->Width)
			{
				continue;
			}

			if (outBackBufferWidth <= itr->Width || outBackBufferHeight <= itr->Height)
			{
				continue;
			}

			if (itr->Width >= requestedWidth && itr->Height >= requestedHeight)
			{
				outBackBufferWidth = itr->Width;
				outBackBufferHeight = itr->Height;
			}
		}
	}
}

ULONG WINAPI THRotatorDirect3DDevice::AddRef(VOID)
{
	return ++m_referenceCount;
}

HRESULT WINAPI THRotatorDirect3DDevice::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	if (ppvObj == nullptr)
	{
		return E_POINTER;
	}
#if TOUHOU_ON_D3D8
	else if (riid == IID_IDirect3DDevice8
#else
	else if (riid == __uuidof(IDirect3DDevice9)
#endif
		|| riid == __uuidof(IUnknown))
	{
		AddRef();
		*ppvObj = this;
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

ULONG WINAPI THRotatorDirect3DDevice::Release()
{
	ULONG ret = --m_referenceCount;
	if (ret == 0)
	{
		delete this;
	}
	return ret;
}

HRESULT WINAPI THRotatorDirect3DDevice::SetViewport(
#ifdef TOUHOU_ON_D3D8
	CONST D3DVIEWPORT8* pViewport)
#else
	CONST D3DVIEWPORT9* pViewport)
#endif
{
	m_pEditorContext->AddViewportSetCount();
	return m_pd3dDev->SetViewport(pViewport);
}

HRESULT WINAPI THRotatorDirect3DDevice::EndScene(VOID)
{
#ifdef TOUHOU_ON_D3D8
	m_pd3dDev->SetRenderTarget(m_pBackBuffer.Get(), nullptr);
#else
	m_pd3dDev->SetRenderTarget(0, m_pBackBuffer.Get());
#endif
	D3DXLoadSurfaceFromSurface(m_pTexSurface.Get(), nullptr, nullptr,
		m_pRenderTarget.Get(), nullptr, nullptr, D3DX_FILTER_NONE, 0);

	m_pd3dDev->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.f, 0);
	//if( SUCCEEDED( m_pd3dDev->BeginScene() ) )
	{
#ifdef TOUHOU_ON_D3D8
		if (SUCCEEDED(m_pSprite->Begin()))
#else
		if (SUCCEEDED(m_pSprite->Begin(D3DXSPRITE_ALPHABLEND)))
#endif
		{
#ifdef TOUHOU_ON_D3D8
			D3DTEXTUREFILTERTYPE prevFilter, prevFilter2;
			m_pd3dDev->GetTextureStageState(0, D3DTSS_MAGFILTER, (DWORD*)&prevFilter);
			m_pd3dDev->GetTextureStageState(0, D3DTSS_MINFILTER, (DWORD*)&prevFilter2);
			m_pd3dDev->SetTextureStageState(0, D3DTSS_MAGFILTER, m_filterType);
			m_pd3dDev->SetTextureStageState(0, D3DTSS_MINFILTER, m_filterType);

#else
			m_pd3dDev->SetSamplerState(0, D3DSAMP_MAGFILTER, m_filterType);
			m_pd3dDev->SetSamplerState(0, D3DSAMP_MINFILTER, m_filterType);
#endif

			m_pd3dDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

			auto rotationAngle = m_pEditorContext->GetRotationAngle();

			bool aspectLessThan133;
			if (rotationAngle % 2 == 0)
				aspectLessThan133 = m_d3dpp.BackBufferWidth * 3 < m_d3dpp.BackBufferHeight * 4;
			else
				aspectLessThan133 = m_d3dpp.BackBufferHeight * 3 < m_d3dpp.BackBufferWidth * 4;

			LONG playRegionLeft = static_cast<LONG>(m_pEditorContext->GetPlayRegionLeft());
			LONG playRegionTop = static_cast<LONG>(m_pEditorContext->GetPlayRegionTop());
			LONG playRegionWidth = static_cast<LONG>(m_pEditorContext->GetPlayRegionWidth());
			LONG playRegionHeight = static_cast<LONG>(m_pEditorContext->GetPlayRegionHeight());

			bool bNeedsRearrangeHUD = aspectLessThan133 && m_pEditorContext->IsViewportSetCountOverThreshold();
			LONG baseDestRectWidth = bNeedsRearrangeHUD ? playRegionWidth : static_cast<LONG>(BASE_SCREEN_WIDTH);
			LONG baseDestRectHeight = bNeedsRearrangeHUD ? playRegionHeight : static_cast<LONG>(BASE_SCREEN_HEIGHT);



			/***** 変換行列の計算 *****

			1. テクスチャを東方が要求したバックバッファ解像度から640x480にスケーリング
			2. 矩形の中心を原点(0, 0)に移動
			3. ユーザが指定した転送元矩形サイズの逆数でスケールをかける
			4. ユーザが指定した矩形ごとの回転角度で矩形を回転
			5. ユーザが指定した転送先矩形サイズでスケールをかける
			6. ユーザが指定した転送先矩形の位置へ移動
			7. プレイ領域のサイズがTHRotatorで内部的に確保したバックバッファ解像度へ拡大
			8. 現在の画面回転角度で回転
			9. 画面全体の中心を原点からバックバッファの中心へ移動

			2から6は矩形ごとのパラメータが必要。
			それより前の1と、後の7-9は事前計算できるので、それぞれpreRectTransform、postRectTransformとして保持

			*****/

			D3DXMATRIX preRectTransform;
			{
				D3DXMATRIX baseSrcScale;
				D3DXMatrixScaling(&baseSrcScale, BASE_SCREEN_WIDTH / static_cast<float>(m_requestedWidth), BASE_SCREEN_HEIGHT / static_cast<float>(m_requestedHeight), 1.0f);

				preRectTransform = baseSrcScale;
			}

			D3DXMATRIX postRectTransform;
			{
				float baseDestScaleScalar = rotationAngle % 2 == 0 ?
					(std::min)(m_d3dpp.BackBufferWidth / static_cast<float>(baseDestRectWidth), m_d3dpp.BackBufferHeight / static_cast<float>(baseDestRectHeight)) :
					(std::min)(m_d3dpp.BackBufferWidth / static_cast<float>(baseDestRectHeight), m_d3dpp.BackBufferHeight / static_cast<float>(baseDestRectWidth));
				D3DXMATRIX baseDestScale;
				D3DXMatrixScaling(&baseDestScale, baseDestScaleScalar, baseDestScaleScalar, 1.0f);

				D3DXMATRIX rotation;
				D3DXMatrixRotationZ(&rotation, RotationAngleToRadian(rotationAngle));

				D3DXMATRIX baseDestTranslation;
				D3DXMatrixTranslation(&baseDestTranslation, 0.5f * m_d3dpp.BackBufferWidth, 0.5f * m_d3dpp.BackBufferHeight, 0.0f);

				D3DXMATRIX temp;
				D3DXMatrixMultiply(&temp, &baseDestScale, &rotation);
				D3DXMatrixMultiply(&postRectTransform, &temp, &baseDestTranslation);
			}

			auto rectDrawer = [this, &preRectTransform, &postRectTransform](
				const POINT& srcPos,
				const SIZE& srcSize,
				const POINT& destPos,
				const SIZE& destSize,
				const SIZE& playRegionSize,
				RotationAngle rotation)
			{
				POINT correctedSrcPos = srcPos;
				SIZE correctedSrcSize = srcSize;

				SIZE srcRectTopLeftOffset = {};
#ifdef TOUHOU_ON_D3D8
				// 負の座標値がある場合、D3DX8では描画が行われないため、クランプ
				if (correctedSrcPos.x < 0)
				{
					srcRectTopLeftOffset.cx = -correctedSrcPos.x;
					correctedSrcPos.x = 0;
					correctedSrcSize.cx -= srcRectTopLeftOffset.cx;
				}

				if (correctedSrcPos.y < 0)
				{
					srcRectTopLeftOffset.cy = -correctedSrcPos.y;
					correctedSrcPos.y = 0;
					correctedSrcSize.cy -= srcRectTopLeftOffset.cy;
				}
#endif

				D3DXMATRIX translateSrcCenterToOrigin;
				D3DXMatrixTranslation(&translateSrcCenterToOrigin,
					-0.5f * srcSize.cx + srcRectTopLeftOffset.cx,
					-0.5f * srcSize.cy + srcRectTopLeftOffset.cy, 0.0f);

				D3DXMATRIX rectScaleSrcInv;
				D3DXMatrixScaling(&rectScaleSrcInv, 1.0f / srcSize.cx, 1.0f / srcSize.cy, 1.0f);

				D3DXMATRIX rectRotation;
				D3DXMatrixRotationZ(&rectRotation, RotationAngleToRadian(rotation));

				D3DXMATRIX rectScaleDest;
				D3DXMatrixScaling(&rectScaleDest, static_cast<float>(destSize.cx), static_cast<float>(destSize.cy), 1.0f);

				D3DXMATRIX rectTranslation;
				D3DXMatrixTranslation(&rectTranslation,
					static_cast<float>(destPos.x) + 0.5f * (destSize.cx - playRegionSize.cx),
					static_cast<float>(destPos.y) + 0.5f * (destSize.cy - playRegionSize.cy), 0.0f);

				D3DXMATRIX finalTransform, temp;
				D3DXMatrixMultiply(&temp, &preRectTransform, &translateSrcCenterToOrigin);
				D3DXMatrixMultiply(&finalTransform, &temp, &rectScaleSrcInv);
				D3DXMatrixMultiply(&temp, &finalTransform, &rectRotation);
				D3DXMatrixMultiply(&finalTransform, &temp, &rectScaleDest);
				D3DXMatrixMultiply(&temp, &finalTransform, &rectTranslation);
				D3DXMatrixMultiply(&finalTransform, &temp, &postRectTransform);

				RECT scaledSourceRect;
				float scaleFactorForSourceX = static_cast<float>(m_requestedWidth) / BASE_SCREEN_WIDTH;
				float scaleFactorForSourceY = static_cast<float>(m_requestedHeight) / BASE_SCREEN_HEIGHT;
				scaledSourceRect.left = static_cast<LONG>(correctedSrcPos.x * scaleFactorForSourceX);
				scaledSourceRect.right = static_cast<LONG>((correctedSrcPos.x + correctedSrcSize.cx) * scaleFactorForSourceX);
				scaledSourceRect.top = static_cast<LONG>(correctedSrcPos.y * scaleFactorForSourceY);
				scaledSourceRect.bottom = static_cast<LONG>((correctedSrcPos.y + correctedSrcSize.cy) * scaleFactorForSourceY);

#ifdef TOUHOU_ON_D3D8
				m_pSprite->DrawTransform(m_pTex.Get(), &scaledSourceRect, &finalTransform, 0xffffffff);
#else
				m_pSprite->SetTransform(&finalTransform);
				m_pSprite->Draw(m_pTex.Get(), &scaledSourceRect, &D3DXVECTOR3(0, 0, 0), NULL, 0xffffffff);
#endif
			};

			if (!bNeedsRearrangeHUD)
			{
				POINT pointZero = {};
				SIZE rectSize = { BASE_SCREEN_WIDTH, BASE_SCREEN_HEIGHT };
				rectDrawer(pointZero, rectSize, pointZero, rectSize, rectSize, Rotation_0);
			}
			else
			{
				// エネミーマーカーと周囲の枠が表示されるよう、ゲーム画面の矩形を拡張

				POINT prPosition = { playRegionLeft, playRegionTop };
				SIZE prSize = { playRegionWidth, playRegionHeight };
				if (playRegionWidth * 4 < playRegionHeight * 3)
				{
					prPosition.x = playRegionLeft + (playRegionWidth - playRegionHeight * 3 / 4) / 2;
					prSize.cx = playRegionHeight * 3 / 4;
				}
				else if (playRegionWidth * 4 > playRegionHeight * 3)
				{
					prPosition.y = playRegionTop + (playRegionHeight - playRegionWidth * 4 / 3) / 2;
					prSize.cy = playRegionWidth * 4 / 3;
				}

				prPosition.y -= m_pEditorContext->GetYOffset();

				POINT pointZero = {};
				rectDrawer(prPosition, prSize, pointZero, prSize, prSize, Rotation_0);

				for (const auto& rectData : m_pEditorContext->GetRectTransfers())
				{
					if (rectData.rcSrc.right == 0 || rectData.rcSrc.bottom == 0)
					{
						continue;
					}

					if (rectData.rcDest.right == 0 || rectData.rcDest.bottom == 0)
					{
						continue;
					}

					POINT srcPos = { rectData.rcSrc.left, rectData.rcSrc.top };
					SIZE srcSize = { rectData.rcSrc.right, rectData.rcSrc.bottom };
					POINT destPos = { rectData.rcDest.left, rectData.rcDest.top };
					SIZE destSize = { rectData.rcDest.right, rectData.rcDest.bottom };

					rectDrawer(srcPos, srcSize, destPos, destSize, prSize, rectData.rotation);
				}
			}

#ifdef TOUHOU_ON_D3D8
			m_pd3dDev->SetTextureStageState(0, D3DTSS_MAGFILTER, prevFilter);
			m_pd3dDev->SetTextureStageState(0, D3DTSS_MINFILTER, prevFilter2);
#endif

			m_pSprite->End();
		}

#if defined(_DEBUG) && !defined(TOUHOU_ON_D3D8)
		RECT rcText;
		rcText.right = m_d3dpp.BackBufferWidth;
		rcText.left = rcText.right - 128;
		rcText.bottom = m_d3dpp.BackBufferHeight;
		rcText.top = rcText.bottom - 24;
		TCHAR text[64];
		_stprintf_s(text, _T("%5.2f fps"), m_FPS);
		m_pFont->DrawText(NULL, text, -1, &rcText, 0, D3DCOLOR_XRGB(0xff, (unsigned)(0xff * max(0, m_FPS - 30.f) / 30.f), (unsigned)(0xff * max(0, m_FPS - 30.f) / 30.f)));
#endif
	}
#ifdef TOUHOU_ON_D3D8
	m_pd3dDev->SetRenderTarget(m_pRenderTarget.Get(), m_pDepthStencil.Get());
#else
	m_pd3dDev->SetRenderTarget(0, m_pRenderTarget.Get());
#endif
	return m_pd3dDev->EndScene();
}

HRESULT WINAPI THRotatorDirect3DDevice::Present(CONST RECT *pSourceRect,
	CONST RECT *pDestRect,
	HWND hDestWindowOverride,
	CONST RGNDATA *pDirtyRegion)
{
	m_pEditorContext->SubmitViewportSetCountToEditor();

	HRESULT ret = m_pd3dDev->Present(pSourceRect, pDestRect,
		hDestWindowOverride, pDirtyRegion);
	if (FAILED(ret))
	{
		return ret;
	}

	// スクリーンキャプチャリクエストを消費する
	if (m_pEditorContext->ConsumeScreenCaptureRequest())
	{
		namespace fs = boost::filesystem;
		char fname[MAX_PATH];

		fs::create_directory(m_pEditorContext->GetWorkingDirectory() / "snapshot");
		for (int i = 0; i >= 0; ++i)
		{
			wsprintfA(fname, (m_pEditorContext->GetWorkingDirectory() / "snapshot/thRot%03d.bmp").string().c_str(), i);
			if (!fs::exists(fname))
			{
				::D3DXSaveSurfaceToFileA(fname, D3DXIFF_BMP, m_pRenderTarget.Get(), nullptr, nullptr);
				break;
			}
		}
	}

	if (m_resetRevision != m_pEditorContext->GetResetRevision())
	{
		return D3DERR_DEVICENOTRESET;
	}

#ifdef _DEBUG
	QueryPerformanceCounter(&m_cur);
	float FPS = float(m_freq.QuadPart) / float(m_cur.QuadPart - m_prev.QuadPart);
	if (FPS < m_FPSNew)
		m_FPSNew = FPS;

	if (m_fpsCount % 30 == 0)
	{
		m_FPS = m_FPSNew;
		m_FPSNew = 60.f;
	}
	m_fpsCount++;

	m_prev = m_cur;

#endif
	return ret;
}

HRESULT WINAPI THRotatorDirect3DDevice::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	assert(m_bInitialized);

#ifdef _DEBUG
	::OutputDebugStringA("Resetting D3D device\n");
#endif
	ReleaseResources();

	m_d3dpp = *pPresentationParameters;

	if (m_d3dpp.BackBufferWidth != 0)
	{
		m_requestedWidth = m_d3dpp.BackBufferWidth;
	}

	if (m_d3dpp.BackBufferHeight != 0)
	{
		m_requestedHeight = m_d3dpp.BackBufferHeight;
	}

	D3DDEVICE_CREATION_PARAMETERS creationParameters;
	m_pd3dDev->GetCreationParameters(&creationParameters);
	m_pEditorContext->GetBackBufferResolution(m_pMyD3D->m_pd3d, m_d3dpp.BackBufferFormat, creationParameters.AdapterOrdinal,
		m_d3dpp.Windowed, m_requestedWidth, m_requestedHeight, m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight);

	HRESULT ret = m_pd3dDev->Reset(&m_d3dpp);

	if (FAILED(ret))
	{
		return ret;
	}

	m_resetRevision = m_pEditorContext->GetResetRevision();
	return InitResources();
}

extern "C" {
#ifdef TOUHOU_ON_D3D8

	__declspec(naked) void WINAPI d_ValidatePixelShader() { _asm { jmp p_ValidatePixelShader } }
	__declspec(naked) void WINAPI d_ValidateVertexShader() { _asm { jmp p_ValidateVertexShader } }
	__declspec(naked) void WINAPI d_DebugSetMute() { _asm { jmp p_DebugSetMute } }
#else
	__declspec(naked) void WINAPI d_Direct3DShaderValidatorCreate9() { _asm { jmp p_Direct3DShaderValidatorCreate9 } }
	__declspec(naked) void WINAPI d_PSGPError() { _asm { jmp p_PSGPError } }
	__declspec(naked) void WINAPI d_PSGPSampleTexture() { _asm { jmp p_PSGPSampleTexture } }
	__declspec(naked) void WINAPI d_D3DPERF_BeginEvent() { _asm { jmp p_D3DPERF_BeginEvent } }
	__declspec(naked) void WINAPI d_D3DPERF_EndEvent() { _asm { jmp p_D3DPERF_EndEvent } }
	__declspec(naked) void WINAPI d_D3DPERF_GetStatus() { _asm { jmp p_D3DPERF_GetStatus } }
	__declspec(naked) void WINAPI d_D3DPERF_QueryRepeatFrame() { _asm { jmp p_D3DPERF_QueryRepeatFrame } }
	__declspec(naked) void WINAPI d_D3DPERF_SetMarker() { _asm { jmp p_D3DPERF_SetMarker } }
	__declspec(naked) void WINAPI d_D3DPERF_SetOptions() { _asm { jmp p_D3DPERF_SetOptions } }
	__declspec(naked) void WINAPI d_D3DPERF_SetRegion() { _asm { jmp p_D3DPERF_SetRegion } }
	__declspec(naked) void WINAPI d_DebugSetLevel() { _asm { jmp p_DebugSetLevel } }
	__declspec(naked) void WINAPI d_DebugSetMute() { _asm { jmp p_DebugSetMute } }

	__declspec(naked) void WINAPI d_Direct3DCreate9Ex() { _asm { jmp p_Direct3DCreate9Ex } }
#endif
	__declspec(dllexport) Direct3DBase* WINAPI
#ifdef TOUHOU_ON_D3D8
		d_Direct3DCreate8
#else
		d_Direct3DCreate9
#endif
		(UINT v)
	{
		auto pd3d = new THRotatorDirect3D();

		if (!pd3d->init(v))
		{
			pd3d->Release();
			return nullptr;
		}

		return pd3d;
	}
}

HINSTANCE h_original;

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			INITCOMMONCONTROLSEX InitCtrls;
			InitCtrls.dwSize = sizeof(InitCtrls);
			// アプリケーションで使用するすべてのコモン コントロール クラスを含めるには、
			// これを設定します。
			InitCtrls.dwICC = ICC_WIN95_CLASSES;
			InitCommonControlsEx(&InitCtrls);
		}

		g_hModule = hModule;
		{
			TCHAR sysDir[MAX_PATH];
			GetSystemDirectory(sysDir, MAX_PATH);
			TCHAR ch = sysDir[lstrlen(sysDir) - 1];
			if (ch != '\\' || ch != '/')
			{
				sysDir[lstrlen(sysDir) + 1] = '\0';
				sysDir[lstrlen(sysDir)] = '\\';
			}
#ifdef TOUHOU_ON_D3D8
			lstrcatA(sysDir, "d3d8.dll");
#else
			lstrcatA(sysDir, "d3d9.dll");
#endif
			h_original = LoadLibrary(sysDir);
		}

		if (h_original == NULL)
		{
			return FALSE;
		}

#ifdef TOUHOU_ON_D3D8
		p_ValidatePixelShader = GetProcAddress(h_original, "ValidatePixelShader");
		p_ValidateVertexShader = GetProcAddress(h_original, "ValidateVertexShader");
		p_DebugSetMute = GetProcAddress(h_original, "DebugSetMute");
		p_Direct3DCreate8 = (IDirect3D8*(WINAPI*)(UINT))GetProcAddress(h_original, "Direct3DCreate8");
#else
		p_Direct3DShaderValidatorCreate9 = GetProcAddress(h_original, "Direct3DShaderValidatorCreate9");
		p_PSGPError = GetProcAddress(h_original, "PSGPError");
		p_PSGPSampleTexture = GetProcAddress(h_original, "PSGPSampleTexture");
		p_D3DPERF_BeginEvent = GetProcAddress(h_original, "D3DPERF_BeginEvent");
		p_D3DPERF_EndEvent = GetProcAddress(h_original, "D3DPERF_EndEvent");
		p_D3DPERF_GetStatus = GetProcAddress(h_original, "D3DPERF_GetStatus");
		p_D3DPERF_QueryRepeatFrame = GetProcAddress(h_original, "D3DPERF_QueryRepeatFrame");
		p_D3DPERF_SetMarker = GetProcAddress(h_original, "D3DPERF_SetMarker");
		p_D3DPERF_SetOptions = GetProcAddress(h_original, "D3DPERF_SetOptions");
		p_D3DPERF_SetRegion = GetProcAddress(h_original, "D3DPERF_SetRegion");
		p_DebugSetLevel = GetProcAddress(h_original, "DebugSetLevel");
		p_DebugSetMute = GetProcAddress(h_original, "DebugSetMute");
		p_Direct3DCreate9 = reinterpret_cast<DIRECT3DCREATE9PROC>(GetProcAddress(h_original, "Direct3DCreate9"));
		p_Direct3DCreate9Ex = GetProcAddress(h_original, "Direct3DCreate9Ex");
#endif
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		FreeLibrary(h_original);
		break;

	default:
		break;
	}
	return TRUE;
}
