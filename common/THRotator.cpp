// (c) 2017 massanoori

#include "stdafx.h"

#include <wrl.h>

#include "THRotatorEditor.h"

#ifdef TOUHOU_ON_D3D8
#pragma comment(lib, "d3dx8.lib")

#if _MSC_VER >= 1900
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

#else
#pragma comment(lib,"d3dx9.lib")
#endif

using Microsoft::WRL::ComPtr;

static float RotationAngleToRadian(RotationAngle rotation)
{
	assert(0 <= rotation && rotation < Rotation_Num);
	return static_cast<float>(rotation) * D3DX_PI * 0.5f;
}

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
	void GetBackBufferResolution(D3DFORMAT Format, UINT Adapter, BOOL bWindowed, UINT requestedWidth, UINT requestedHeight, UINT& outBackBufferWidth, UINT& outBackBufferHeight) const
	{
		if (bWindowed)
		{
			RECT rc;
			GetClientRect(m_pEditorContext->GetTouhouWindow(), &rc);
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
			UINT count = m_pMyD3D->m_pd3d->GetAdapterModeCount(
#ifdef TOUHOU_ON_D3D8
				Adapter);
#else
				Adapter, Format);
#endif
			std::vector<D3DDISPLAYMODE> availableModes;

			//	ディスプレイの最大解像度を探す＆60Hzの解像度を列挙
			for (UINT i = 0; i < count; ++i)
			{
				m_pMyD3D->m_pd3d->EnumAdapterModes(Adapter,
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
	m_pMyD3D = pMyD3D;

	GetBackBufferResolution(d3dpp.BackBufferFormat, Adapter, d3dpp.Windowed,
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
	GetBackBufferResolution(m_d3dpp.BackBufferFormat, creationParameters.AdapterOrdinal,
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
