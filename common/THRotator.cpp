// (c) 2017 massanoori

#include "stdafx.h"

#include <DirectXMath.h>

#include "THRotatorEditor.h"
#include "THRotatorSystem.h"
#include "StringResource.h"
#include "THRotatorLog.h"
#include "resource.h"

#include "THRotatorImGui.h"
#include "Direct3DUtils.h"
#include "ImageIO.h"

#ifdef TOUHOU_ON_D3D8

// For IID_IDirect3D8, IID_IDirect3DDevice8.
#pragma comment(lib, "dxguid.lib")

#define DECLARE_POSSIBLY_SCOPED_STATE(suffix) \
using PossiblyScoped##suffix = Scoped##suffix;

DECLARE_POSSIBLY_SCOPED_STATE(RS)
DECLARE_POSSIBLY_SCOPED_STATE(TSS)
DECLARE_POSSIBLY_SCOPED_STATE(Tex)
DECLARE_POSSIBLY_SCOPED_STATE(Transform)
DECLARE_POSSIBLY_SCOPED_STATE(StreamSource)
DECLARE_POSSIBLY_SCOPED_STATE(Indices)
DECLARE_POSSIBLY_SCOPED_STATE(FVF)

#undef DECLARE_POSSIBLY_SCOPED_STATE

#else

struct PossiblyScopedRS
{
	PossiblyScopedRS(Direct3DDeviceBase* inDevice, D3DRENDERSTATETYPE inRenderStateType, DWORD value)
	{
		inDevice->SetRenderState(inRenderStateType, value);
	}
};

struct PossiblyScopedTSS
{
	PossiblyScopedTSS(Direct3DDeviceBase* inDevice, DWORD inTextureStage, D3DTEXTURESTAGESTATETYPE inTextureStageStateType, DWORD value)
	{
		inDevice->SetTextureStageState(inTextureStage, inTextureStageStateType, value);
	}
};

struct PossiblyScopedTex
{
	PossiblyScopedTex(Direct3DDeviceBase* inDevice, DWORD inTextureStage, Direct3DBaseTextureBase* texture)
	{
		inDevice->SetTexture(inTextureStage, texture);
	}
};

struct PossiblyScopedTransform
{
	PossiblyScopedTransform(Direct3DDeviceBase* inDevice, D3DTRANSFORMSTATETYPE inTransformType, const D3DMATRIX& transformMatrix)
	{
		inDevice->SetTransform(inTransformType, &transformMatrix);
	}

	PossiblyScopedTransform(Direct3DDeviceBase* inDevice, D3DTRANSFORMSTATETYPE inTransformType)
	{
	}
};

struct PossiblyScopedStreamSource
{
	PossiblyScopedStreamSource(Direct3DDeviceBase* inDevice, UINT inStreamNumber, Direct3DVertexBufferBase* vertexBuffer, UINT stride)
	{
		inDevice->SetStreamSource(inStreamNumber, vertexBuffer, 0, stride);
	}
};

struct PossiblyScopedFVF
{
	PossiblyScopedFVF(Direct3DDeviceBase* inDevice, DWORD FVF)
	{
		inDevice->SetFVF(FVF);
	}
};

struct PossiblyScopedIndices
{
	PossiblyScopedIndices(Direct3DDeviceBase* inDevice, Direct3DIndexBufferBase* indices, UINT offset)
	{
		UNREFERENCED_PARAMETER(offset);

		inDevice->SetIndices(indices);
	}
};

#endif

using Microsoft::WRL::ComPtr;

static float RotationAngleToRadian(RotationAngle rotation)
{
	assert(0 <= rotation && rotation < Rotation_Num);
	return static_cast<float>(rotation) * DirectX::XM_PIDIV2;
}

namespace
{

const UINT BASE_SCREEN_WIDTH = 640u;
const UINT BASE_SCREEN_HEIGHT = 480u;

struct THRotatorSpriteVertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 uv;

	enum
	{
		FVF = D3DFVF_XYZ | D3DFVF_TEX1,
	};

	static const THRotatorSpriteVertex vertexBufferContent[4];
	static const D3DPRIMITIVETYPE vertexBufferPrimitiveType;
	static const UINT vertexBufferPrimitiveCount;
};

const THRotatorSpriteVertex THRotatorSpriteVertex::vertexBufferContent[4] =
{
	{ DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
	{ DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },
	{ DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },
	{ DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },
};

const D3DPRIMITIVETYPE THRotatorSpriteVertex::vertexBufferPrimitiveType = D3DPT_TRIANGLESTRIP;
const UINT THRotatorSpriteVertex::vertexBufferPrimitiveCount = _countof(THRotatorSpriteVertex::vertexBufferContent) - 2u;

D3DMATRIX ToD3DMATRIX(const DirectX::XMMATRIX& matrix)
{
	D3DMATRIX outMatrix;
	static_assert(sizeof(outMatrix) == sizeof(matrix), "XMMATRIX and D3DMATRIX are assumed to have same size.");

	memcpy(&outMatrix, &matrix, sizeof(matrix));

	return outMatrix;
}

}

class THRotatorDirect3DDevice;

class THRotatorDirect3D : public Direct3DExBase
{
	friend class THRotatorDirect3DDevice;
	Direct3DBase* m_pd3d;
	ComPtr<Direct3DExBase> m_pd3dEx;
	ULONG m_referenceCount;

public:
	THRotatorDirect3D();
	virtual ~THRotatorDirect3D();

	bool Init(UINT v);
#ifndef TOUHOU_ON_D3D8
	HRESULT InitEx(UINT v);
#endif

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

#ifndef TOUHOU_ON_D3D8
	UINT WINAPI GetAdapterModeCountEx(UINT Adapter, const D3DDISPLAYMODEFILTER *pFilter) override
	{
		return m_pd3dEx->GetAdapterModeCountEx(Adapter, pFilter);
	}

	HRESULT WINAPI EnumAdapterModesEx(
		UINT Adapter,
		const D3DDISPLAYMODEFILTER *pFilter,
		UINT Mode,
		D3DDISPLAYMODEEX* pMode) override
	{
		return m_pd3dEx->EnumAdapterModesEx(Adapter, pFilter, Mode, pMode);
	}

	HRESULT WINAPI GetAdapterDisplayModeEx(
		UINT               Adapter,
		D3DDISPLAYMODEEX   *pMode,
		D3DDISPLAYROTATION *pRotation) override
	{
		return m_pd3dEx->GetAdapterDisplayModeEx(Adapter, pMode, pRotation);
	}

	HRESULT WINAPI GetAdapterLUID(
		UINT Adapter,
		LUID *pLUID) override
	{
		return m_pd3dEx->GetAdapterLUID(Adapter, pLUID);
	}

	HRESULT WINAPI CreateDeviceEx(UINT Adapter,
		D3DDEVTYPE DeviceType,
		HWND hFocusWindow,
		DWORD BehaviorFlags,
		D3DPRESENT_PARAMETERS *pPresentationParameters,
		D3DDISPLAYMODEEX *pFullscreenDisplayMode,
		IDirect3DDevice9Ex **ppReturnedDeviceInterface) override;
#endif
};

class THRotatorEditorContext;

class THRotatorDirect3DDevice : public Direct3DDeviceExBase
{
private:
	friend class THRotatorDirect3D;

	bool m_bInitialized;
	ULONG m_referenceCount;

	/****************************************
	 * Direct3D resources
	 ****************************************/

	ComPtr<Direct3DDeviceBase> m_pd3dDev;
	ComPtr<Direct3DDeviceExBase> m_pd3dDevEx;

	ComPtr<Direct3DVertexBufferBase> m_pSpriteVertexBuffer;

	// Original underlying back buffer.
	ComPtr<Direct3DSurfaceBase> m_pBackBuffer;

	// Dummy render target and accompanying depth stencil surface.
	ComPtr<Direct3DSurfaceBase> m_pRenderTarget;
	ComPtr<Direct3DSurfaceBase> m_pDepthStencil;

	// Intermediate texture and texture as a surface.
	// m_pTexSurface and m_pRenderTarget should be separated
	// for Touhou's screen capture to work without crash.
	ComPtr<Direct3DTextureBase> m_pTex;
	ComPtr<Direct3DSurfaceBase> m_pTexSurface;
	
	ComPtr<THRotatorDirect3D> m_pMyD3D;


	/****************************************
	 * Direct3D parameters
	 ****************************************/

	D3DPRESENT_PARAMETERS m_d3dpp;
	UINT m_requestedWidth, m_requestedHeight;
	D3DDEVTYPE m_deviceType;


	/****************************************
	 * Editor context data
	 ****************************************/

	std::shared_ptr<THRotatorEditorContext> m_pEditorContext;
	int m_resetRevision; // revision of last device reset


private:

	THRotatorDirect3DDevice(const THRotatorDirect3DDevice&) = delete;
	THRotatorDirect3DDevice(THRotatorDirect3DDevice&&) = delete;
	THRotatorDirect3DDevice& operator=(const THRotatorDirect3DDevice&) = delete;
	THRotatorDirect3DDevice& operator=(THRotatorDirect3D&&) = delete;

	HRESULT InternalInit(UINT Adapter, THRotatorDirect3D* pMyD3D, D3DDEVTYPE DeviceType, HWND hFocusWindow,
		DWORD BehaviorFlags,
#ifdef TOUHOU_ON_D3D8
		D3DPRESENT_PARAMETERS* pPresentationParameters);
#else
		D3DPRESENT_PARAMETERS* pPresentationParameters, bool bIsEx, D3DDISPLAYMODEEX* pModeEx);
#endif

public:
	THRotatorDirect3DDevice();
	virtual ~THRotatorDirect3DDevice();

	HRESULT Init(UINT Adapter, THRotatorDirect3D* pMyD3D, D3DDEVTYPE DeviceType, HWND hFocusWindow,
		DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters);

#ifndef TOUHOU_ON_D3D8
	HRESULT InitEx(UINT Adapter, THRotatorDirect3D* pMyD3D, D3DDEVTYPE DeviceType, HWND hFocusWindow,
		DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pModeEx);
#endif

	HRESULT InitResources();
	void ReleaseResources();
	void UpdateResolution(UINT Adapter);

	ULONG WINAPI AddRef(VOID) override;
	HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj) override;
	ULONG WINAPI Release() override;
	HRESULT WINAPI Present(CONST RECT *pSourceRect,
		CONST RECT *pDestRect,
		HWND hDestWindowOverride,
		CONST RGNDATA *pDirtyRegion) override;

	HRESULT InternalPresent(CONST RECT *pSourceRect,
		CONST RECT *pDestRect,
		HWND hDestWindowOverride,
#ifdef TOUHOU_ON_D3D8
		CONST RGNDATA *pDirtyRegion);
#else
		CONST RGNDATA *pDirtyRegion,
		bool bIsEx,
		DWORD dwFlags);
#endif

	HRESULT WINAPI EndScene(VOID) override;
	void EndSceneInternal();

	HRESULT WINAPI Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) override;

	HRESULT InternalReset(
#ifdef TOUHOU_ON_D3D8
		D3DPRESENT_PARAMETERS* pPresentationParameters);
#else
		D3DPRESENT_PARAMETERS* pPresentationParameters, bool bIsEx, D3DDISPLAYMODEEX* pFullscreenDisplayMode);
#endif

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
		UINT /* iSwapChain */,
#endif
		UINT /* BackBuffer */,
		D3DBACKBUFFER_TYPE /* Type */,
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
		HRESULT originalResult = m_pd3dDev->TestCooperativeLevel();
		if (FAILED(originalResult))
		{
			return originalResult;
		}

		if (m_resetRevision != m_pEditorContext->GetResetRevision())
		{
			return D3DERR_DEVICENOTRESET;
		}

		return originalResult;
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

	HRESULT WINAPI CheckDeviceState(HWND hWindow) override
	{
		return m_pd3dDevEx->CheckDeviceState(hWindow);
	}

	HRESULT WINAPI CheckResourceResidency(
		IDirect3DResource9** pResourceArray,
		UINT32 NumResources) override
	{
		return m_pd3dDevEx->CheckResourceResidency(pResourceArray, NumResources);
	}

	HRESULT WINAPI ComposeRects(
		IDirect3DSurface9 *pSource,
		IDirect3DSurface9 *pDestination,
		IDirect3DVertexBuffer9 *pSrcRectDescriptors,
		UINT NumRects,
		IDirect3DVertexBuffer9 *pDstRectDescriptors,
		D3DCOMPOSERECTSOP Operation,
		INT XOffset,
		INT YOffset) override
	{
		return m_pd3dDevEx->ComposeRects(pSource, pDestination, pSrcRectDescriptors, NumRects, pDstRectDescriptors, Operation, XOffset, YOffset);
	}

	HRESULT WINAPI CreateDepthStencilSurfaceEx(
		UINT Width,
		UINT Height,
		D3DFORMAT Format,
		D3DMULTISAMPLE_TYPE MultiSample,
		DWORD MultisampleQuality,
		BOOL Discard,
		IDirect3DSurface9 **ppSurface,
		HANDLE *pSharedHandle,
		DWORD Usage) override
	{
		return m_pd3dDevEx->CreateDepthStencilSurfaceEx(Width, Height, Format, MultiSample, MultisampleQuality,
			Discard, ppSurface, pSharedHandle, Usage);
	}

	HRESULT WINAPI CreateOffscreenPlainSurfaceEx(
		UINT Width,
		UINT Height,
		D3DFORMAT Format,
		D3DPOOL Pool,
		IDirect3DSurface9 **ppSurface,
		HANDLE *pSharedHandle,
		DWORD Usage) override
	{
		return m_pd3dDevEx->CreateOffscreenPlainSurfaceEx(Width, Height, Format, Pool, ppSurface, pSharedHandle, Usage);
	}

	HRESULT WINAPI CreateRenderTargetEx(
		UINT Width,
		UINT Height,
		D3DFORMAT Format,
		D3DMULTISAMPLE_TYPE MultiSample,
		DWORD MultisampleQuality,
		BOOL Lockable,
		IDirect3DSurface9 **ppSurface,
		HANDLE *pSharedHandle,
		DWORD Usage) override
	{
		return m_pd3dDevEx->CreateRenderTargetEx(Width, Height, Format, MultiSample, MultisampleQuality,
			Lockable, ppSurface, pSharedHandle, Usage);
	}

	HRESULT WINAPI GetDisplayModeEx(
		UINT  iSwapChain,
		D3DDISPLAYMODEEX *pMode,
		D3DDISPLAYROTATION *pRotation) override
	{
		return m_pd3dDevEx->GetDisplayModeEx(iSwapChain, pMode, pRotation);
	}

	HRESULT WINAPI GetGPUThreadPriority(
		INT *pPriority) override
	{
		return m_pd3dDevEx->GetGPUThreadPriority(pPriority);
	}

	HRESULT WINAPI GetMaximumFrameLatency(
		UINT *pMaxLatency) override
	{
		return m_pd3dDevEx->GetMaximumFrameLatency(pMaxLatency);
	}

	HRESULT WINAPI PresentEx(
		CONST RECT *pSourceRect,
		CONST RECT *pDestRect,
		HWND hDestWindowOverride,
		CONST RGNDATA *pDirtyRegion,
		DWORD dwFlags) override;

	HRESULT WINAPI ResetEx(
		D3DPRESENT_PARAMETERS* pPresentationParameters,
		D3DDISPLAYMODEEX* pFullscreenDisplayMode) override;

	HRESULT WINAPI SetConvolutionMonoKernel(
		UINT Width,
		UINT Height,
		float *RowWeights,
		float *ColumnWeights) override
	{
		return m_pd3dDevEx->SetConvolutionMonoKernel(Width, Height, RowWeights, ColumnWeights);
	}

	HRESULT WINAPI SetGPUThreadPriority(
		INT Priority) override
	{
		return m_pd3dDevEx->SetGPUThreadPriority(Priority);
	}

	HRESULT WINAPI SetMaximumFrameLatency(
		UINT MaxLatency) override
	{
		return m_pd3dDevEx->SetMaximumFrameLatency(MaxLatency);
	}

	HRESULT WINAPI WaitForVBlank(
		UINT SwapChainIndex) override
	{
		return m_pd3dDevEx->WaitForVBlank(SwapChainIndex);
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

bool THRotatorDirect3D::Init(UINT v)
{
#ifndef TOUHOU_ON_D3D8
	if (m_pd3d)
	{
		m_pd3d->Release();
	}

	m_pd3dEx.Reset();
#endif

	m_pd3d = CallOriginalDirect3DCreate(v);

	return m_pd3d != NULL;
}

#ifndef TOUHOU_ON_D3D8
HRESULT THRotatorDirect3D::InitEx(UINT v)
{
	HRESULT ret = CallOriginalDirect3DCreate9Ex(v, &m_pd3dEx);
	if (FAILED(ret))
	{
		return ret;
	}

	if (m_pd3d)
	{
		m_pd3d->Release();
	}
	m_pd3dEx.Get()->QueryInterface(__uuidof(Direct3DBase), reinterpret_cast<void**>(&m_pd3d));
	return ret;
}
#endif

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
#ifndef TOUHOU_ON_D3D8
	else if (m_pd3dEx && riid == __uuidof(Direct3DExBase))
	{
		AddRef();
		*ppvObj = this;
		return S_OK;
	}
#endif
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
	D3DPRESENT_PARAMETERS *pPresentationParameters,
	Direct3DDeviceBase **ppReturnedDeviceInterface)
{
	auto pRetDev = new THRotatorDirect3DDevice();
	HRESULT ret = pRetDev->Init(Adapter, this, DeviceType, hFocusWindow, BehaviorFlags,
		pPresentationParameters);

	if (FAILED(ret))
	{
		pRetDev->Release();
		return ret;
	}
	*ppReturnedDeviceInterface = pRetDev;

	return ret;
}

#ifndef TOUHOU_ON_D3D8
HRESULT THRotatorDirect3D::CreateDeviceEx(UINT Adapter,
	D3DDEVTYPE DeviceType,
	HWND hFocusWindow,
	DWORD BehaviorFlags,
	D3DPRESENT_PARAMETERS* pPresentationParameters,
	D3DDISPLAYMODEEX* pFullscreenDisplayMode,
	IDirect3DDevice9Ex** ppReturnedDeviceInterface)
{
	auto pRetDev = new THRotatorDirect3DDevice();
	HRESULT ret = pRetDev->InitEx(Adapter, this, DeviceType, hFocusWindow, BehaviorFlags,
		pPresentationParameters, pFullscreenDisplayMode);

	if (FAILED(ret))
	{
		pRetDev->Release();
		return ret;
	}
	*ppReturnedDeviceInterface = pRetDev;

	return ret;
}
#endif

THRotatorDirect3DDevice::THRotatorDirect3DDevice()
	: m_bInitialized(false)
	, m_referenceCount(1)
	, m_d3dpp{}
	, m_requestedWidth(0)
	, m_requestedHeight(0)
	, m_deviceType(D3DDEVTYPE_HAL)
	, m_resetRevision(0)
{
}

THRotatorDirect3DDevice::~THRotatorDirect3DDevice()
{
	OutputLogMessage(LogSeverity::Info, "Destructing THRotatorDirect3DDevice");

	THRotatorImGui_Shutdown();
}

HRESULT THRotatorDirect3DDevice::Init(UINT Adapter, THRotatorDirect3D* pMyD3D, D3DDEVTYPE DeviceType, HWND hFocusWindow,
	DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	return InternalInit(Adapter, pMyD3D, DeviceType, hFocusWindow, BehaviorFlags,
#ifdef TOUHOU_ON_D3D8
		pPresentationParameters);
#else
		pPresentationParameters, false, nullptr);
#endif
}

#ifndef TOUHOU_ON_D3D8
HRESULT THRotatorDirect3DDevice::InitEx(UINT Adapter, THRotatorDirect3D * pMyD3D, D3DDEVTYPE DeviceType, HWND hFocusWindow,
	DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pModeEx)
{
	return InternalInit(Adapter, pMyD3D, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, true, pModeEx);
}
#endif

HRESULT THRotatorDirect3DDevice::InternalInit(UINT Adapter,
	THRotatorDirect3D* pMyD3D,
	D3DDEVTYPE DeviceType,
	HWND hFocusWindow,
	DWORD BehaviorFlags,
#ifdef TOUHOU_ON_D3D8
	D3DPRESENT_PARAMETERS* pPresentationParameters)
#else
	D3DPRESENT_PARAMETERS* pPresentationParameters,
	bool bIsEx,
	D3DDISPLAYMODEEX* pModeEx)
#endif
{
#ifndef TOUHOU_ON_D3D8
	if (BehaviorFlags & D3DCREATE_ADAPTERGROUP_DEVICE)
	{
		D3DCAPS9 deviceCapabilities;
		pMyD3D->GetDeviceCaps(Adapter, DeviceType, &deviceCapabilities);
		if (deviceCapabilities.NumberOfAdaptersInGroup > 1)
		{
			MessageBox(hFocusWindow, _T("THRotator does not support D3DCREATE_ADAPTERGROUP_DEVICE with multiple adapters in a group."), nullptr, MB_ICONSTOP);
			return E_FAIL;
		}
	}
#endif

	m_d3dpp = *pPresentationParameters;

	m_requestedWidth = pPresentationParameters->BackBufferWidth;
	m_requestedHeight = pPresentationParameters->BackBufferHeight;

	if (m_requestedWidth == 0 || m_requestedHeight == 0)
	{
		HWND hMainWindow = pPresentationParameters->hDeviceWindow;
		if (hMainWindow == nullptr)
		{
			hMainWindow = hFocusWindow;
		}

		assert(hMainWindow != nullptr);

		RECT rectMainWindow;
		GetClientRect(hMainWindow, &rectMainWindow);

		m_requestedWidth = rectMainWindow.right - rectMainWindow.left;
		m_requestedHeight = rectMainWindow.bottom - rectMainWindow.top;
	}

	m_pEditorContext = THRotatorEditorContext::CreateOrGetEditorContext(hFocusWindow);
	if (!m_pEditorContext)
	{
		return E_FAIL;
	}

	m_resetRevision = m_pEditorContext->GetResetRevision();
	m_pMyD3D = pMyD3D;
	m_deviceType = DeviceType;

	UpdateResolution(Adapter);

	OutputLogMessagef(LogSeverity::Info, "Resolution requested: {0}x{1}, Modified resolution: {2}x{3}",
		m_requestedWidth, m_requestedHeight, m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight);
	OutputLogMessagef(LogSeverity::Info, "Back buffer format: {}", m_d3dpp.BackBufferFormat);

	HRESULT ret;

#ifndef TOUHOU_ON_D3D8
	if (bIsEx)
	{
		OutputLogMessagef(LogSeverity::Info, "Initializing D3D9 Ex device");
		if (pModeEx == nullptr)
		{
			ret = pMyD3D->m_pd3dEx->CreateDeviceEx(Adapter,
				DeviceType, hFocusWindow, BehaviorFlags,
				&m_d3dpp, pModeEx, &m_pd3dDevEx);
		}
		else
		{
			D3DDISPLAYMODEEX modeExtra = *pModeEx;
			modeExtra.Width = m_d3dpp.BackBufferWidth;
			modeExtra.Height = m_d3dpp.BackBufferHeight;

			ret = pMyD3D->m_pd3dEx->CreateDeviceEx(Adapter,
				DeviceType, hFocusWindow, BehaviorFlags,
				&m_d3dpp, &modeExtra, &m_pd3dDevEx);
		}
		
		if (FAILED(ret))
		{
			OutputLogMessagef(LogSeverity::Error, "Failed to initialize device");
			return ret;
		}

		m_pd3dDev = m_pd3dDevEx;
	}
	else
#endif
	{
#ifdef TOUHOU_ON_D3D8
		OutputLogMessagef(LogSeverity::Info, "Initializing D3D8 device");
#else
		OutputLogMessagef(LogSeverity::Info, "Initializing D3D9 device");
#endif

		ret = pMyD3D->m_pd3d->CreateDevice(Adapter,
			DeviceType, hFocusWindow, BehaviorFlags,
			&m_d3dpp, &m_pd3dDev);
		if (FAILED(ret))
		{
			OutputLogMessagef(LogSeverity::Error, "Failed to initialize device");
			return ret;
		}

		//m_pd3dDev.As<Direct3DDeviceExBase>(&m_pd3dDevEx);
	}

	THRotatorImGui_Initialize(m_pEditorContext->GetTouhouWindow(), m_pd3dDev.Get());

	ret = InitResources();
	if (FAILED(ret))
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to initialize render resources");
		return ret;
	}

	// CreateDevice() overwrites D3DPRESENT_PARAMETERS
	*pPresentationParameters = m_d3dpp;
	pPresentationParameters->BackBufferWidth = m_requestedWidth;
	pPresentationParameters->BackBufferHeight = m_requestedHeight;

	m_bInitialized = true;
	return S_OK;
}

HRESULT THRotatorDirect3DDevice::InitResources()
{
	OutputLogMessagef(LogSeverity::Info, "Initializing resources");

	HRESULT hr = m_pd3dDev->CreateTexture(m_requestedWidth, m_requestedHeight, 1,
		D3DUSAGE_RENDERTARGET, m_d3dpp.BackBufferFormat, D3DPOOL_DEFAULT,
#ifdef TOUHOU_ON_D3D8
		&m_pTex);
#else
		&m_pTex, NULL);
#endif
	if (FAILED(hr))
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to create texture");
		return hr;
	}

	m_pTex->GetSurfaceLevel(0, &m_pTexSurface);

	hr = m_pd3dDev->CreateVertexBuffer(sizeof(THRotatorSpriteVertex::vertexBufferContent),
		0, THRotatorSpriteVertex::FVF, D3DPOOL_DEFAULT,
#ifdef TOUHOU_ON_D3D8
		&m_pSpriteVertexBuffer);
#else
		&m_pSpriteVertexBuffer, nullptr);
#endif
	if (FAILED(hr))
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to create vertex buffer");
		return hr;
	}

	void* pLockedSpriteVertexBufferMemory = nullptr;

#ifdef TOUHOU_ON_D3D8
	auto ppLockedBufferMemory = reinterpret_cast<BYTE**>(&pLockedSpriteVertexBufferMemory);
#else
	auto ppLockedBufferMemory = reinterpret_cast<void**>(&pLockedSpriteVertexBufferMemory);
#endif

	hr = m_pSpriteVertexBuffer->Lock(0, 0, ppLockedBufferMemory, D3DLOCK_DISCARD);
	if (FAILED(hr))
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to lock vertex buffer");
		return hr;
	}

	memcpy(pLockedSpriteVertexBufferMemory, THRotatorSpriteVertex::vertexBufferContent,
		sizeof(THRotatorSpriteVertex::vertexBufferContent));

	pLockedSpriteVertexBufferMemory = nullptr;
	m_pSpriteVertexBuffer->Unlock();

	hr = m_pd3dDev->CreateRenderTarget(m_requestedWidth, m_requestedHeight,
		m_d3dpp.BackBufferFormat, m_d3dpp.MultiSampleType,
#ifdef TOUHOU_ON_D3D8
		TRUE, &m_pRenderTarget);
#else
		m_d3dpp.MultiSampleQuality, TRUE, &m_pRenderTarget, nullptr);
#endif

	if (FAILED(hr))
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to create render target");
		return hr;
	}

	ComPtr<Direct3DSurfaceBase> pSurf;

	if (FAILED(hr = m_pd3dDev->GetDepthStencilSurface(&pSurf)))
	{
		OutputLogMessagef(LogSeverity::Info, "Application has no depth stencil surface");
		m_pDepthStencil.Reset();
	}
	else
	{
		D3DSURFACE_DESC surfDesc;
		if (FAILED(hr = pSurf->GetDesc(&surfDesc)))
		{
			OutputLogMessagef(LogSeverity::Error, "Failed to get descriptor of depth stencil surface");
			return hr;
		}

		hr = m_pd3dDev->CreateDepthStencilSurface(m_requestedWidth, m_requestedHeight,
			surfDesc.Format, surfDesc.MultiSampleType,
#ifdef TOUHOU_ON_D3D8
			&m_pDepthStencil);
#else
			surfDesc.MultiSampleQuality, m_d3dpp.Flags & D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL ? TRUE : FALSE, &m_pDepthStencil, nullptr);
#endif

		if (FAILED(hr))
		{
			OutputLogMessagef(LogSeverity::Error, "Failed to create depth stencil surface");
			return hr;
		}
	}

#ifdef TOUHOU_ON_D3D8
	if (FAILED(hr = m_pd3dDev->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer)))
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to get back buffer");
		return hr;
	}

	if (FAILED(hr = m_pd3dDev->SetRenderTarget(m_pRenderTarget.Get(), m_pDepthStencil.Get())))
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to overwrite render target");
		return hr;
	}
#else
	if (FAILED(hr = m_pd3dDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer)))
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to get back buffer");
		return hr;
	}

	if (FAILED(hr = m_pd3dDev->SetRenderTarget(0, m_pRenderTarget.Get())))
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to overwrite render target");
		return hr;
	}

	if (FAILED(hr = m_pd3dDev->SetDepthStencilSurface(m_pDepthStencil.Get())))
	{
		OutputLogMessagef(LogSeverity::Error, "Failed to overwrite depth stencil surface");
		return hr;
	}
#endif

	THRotatorImGui_CreateDeviceObjects();

	return S_OK;
}

void THRotatorDirect3DDevice::ReleaseResources()
{
	OutputLogMessagef(LogSeverity::Info, "Releasing resources");

	THRotatorImGui_InvalidateDeviceObjects();

	m_pSpriteVertexBuffer.Reset();

	m_pDepthStencil.Reset();
	m_pRenderTarget.Reset();
	m_pTexSurface.Reset();
	m_pTex.Reset();
	m_pBackBuffer.Reset();
}

void THRotatorDirect3DDevice::UpdateResolution(UINT Adapter)
{
	if (m_d3dpp.Windowed)
	{
		m_pEditorContext->UpdateWindowResolution(m_requestedWidth, m_requestedHeight,
			m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight);
		return;
	}

	m_d3dpp.BackBufferWidth = GetSystemMetrics(SM_CXSCREEN);
	m_d3dpp.BackBufferHeight = GetSystemMetrics(SM_CYSCREEN);

	D3DFORMAT adapterFormatForEnumeration = m_d3dpp.BackBufferFormat;

#ifdef TOUHOU_ON_D3D8
	UINT count = m_pMyD3D->m_pd3d->GetAdapterModeCount(Adapter);
#else
	// Formats that IDirect3D9::EnumAdapterModes() accepts.
	D3DFORMAT formatsForAdapterModeEnumeration[] =
	{
		D3DFMT_A8R8G8B8,
		D3DFMT_X8R8G8B8,
		D3DFMT_R5G6B5,
		D3DFMT_X1R5G5B5,
		D3DFMT_A1R5G5B5,
		D3DFMT_A2R10G10B10,
	};

	// Find the format that is accepted by IDirect3D9::EnumAdapterModes()
	// and is compatible with the requested pixel format.
	for (int candidateIndex = 0; candidateIndex < _countof(formatsForAdapterModeEnumeration); candidateIndex++)
	{
		HRESULT hr = m_pMyD3D->m_pd3d->CheckDeviceType(Adapter, m_deviceType, formatsForAdapterModeEnumeration[candidateIndex],
			m_d3dpp.BackBufferFormat, FALSE);
		if (SUCCEEDED(hr))
		{
			adapterFormatForEnumeration = formatsForAdapterModeEnumeration[candidateIndex];
			break;
		}
	}

	UINT count = m_pMyD3D->m_pd3d->GetAdapterModeCount(Adapter, adapterFormatForEnumeration);
#endif

	D3DDISPLAYMODE d3ddm;

	std::vector<D3DDISPLAYMODE> availableModes;

	// Find the largest available resolution with refresh rate of 60 Hz.
	for (UINT i = 0; i < count; ++i)
	{
		m_pMyD3D->m_pd3d->EnumAdapterModes(Adapter,
#ifndef TOUHOU_ON_D3D8
			adapterFormatForEnumeration,
#endif
			i, &d3ddm);

		if (d3ddm.RefreshRate != 60 && d3ddm.RefreshRate != 59)
		{
			continue;
		}

#ifdef TOUHOU_ON_D3D8
		if (d3ddm.Format != adapterFormatForEnumeration)
		{
			continue;
		}
#endif
		availableModes.push_back(d3ddm);
		if (m_d3dpp.BackBufferWidth * m_d3dpp.BackBufferHeight < d3ddm.Width*d3ddm.Height)
		{
			m_d3dpp.BackBufferWidth = d3ddm.Width;
			m_d3dpp.BackBufferHeight = d3ddm.Height;
		}
	}

	// Choose as small resolution as possible with same aspect ratio to minimize memory footprint.
	for (std::vector<D3DDISPLAYMODE>::const_iterator itr = availableModes.cbegin();
		itr != availableModes.cend(); ++itr)
	{
		if (m_d3dpp.BackBufferWidth * itr->Height != m_d3dpp.BackBufferHeight * itr->Width)
		{
			continue;
		}

		if (m_d3dpp.BackBufferWidth <= itr->Width || m_d3dpp.BackBufferHeight <= itr->Height)
		{
			continue;
		}

		if (itr->Width >= m_requestedWidth && itr->Height >= m_requestedHeight)
		{
			m_d3dpp.BackBufferWidth = itr->Width;
			m_d3dpp.BackBufferHeight = itr->Height;
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
#ifndef TOUHOU_ON_D3D8
	else if (m_pd3dDevEx && riid == __uuidof(IDirect3DDevice9Ex))
	{
		AddRef();
		*ppvObj = this;
		return S_OK;
	}
#endif
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
	m_pd3dDev->SetDepthStencilSurface(nullptr);
#endif

	{
		// Copying surface from dummy target to texture as surface

#ifdef TOUHOU_ON_D3D8
		RECT sourceRectToCopy;
		sourceRectToCopy.left = 0;
		sourceRectToCopy.top = 0;
		sourceRectToCopy.right = m_requestedWidth;
		sourceRectToCopy.bottom = m_requestedHeight;

		POINT destRectPoint{};

		m_pd3dDev->CopyRects(m_pRenderTarget.Get(), &sourceRectToCopy, 1, m_pTexSurface.Get(), &destRectPoint);
#else
		m_pd3dDev->StretchRect(m_pRenderTarget.Get(), nullptr, m_pTexSurface.Get(), nullptr, D3DTEXF_NONE);
#endif
	}

	THRotatorImGui_NewFrame();

#ifdef _DEBUG
	// Shows only GUI of THRotator.
	// Useful when capturing screen for user's manual without any material from orignal Touhou.
	static bool bGuiOnly = false;
	if (ImGui::Begin("THRotator GUI only"))
	{
		ImGui::Checkbox("GUI only", &bGuiOnly);
	}

	ImGui::End();

	DWORD colorForClear = bGuiOnly ? 0x00BFBFBF : 0x00000000;
#else
	const bool bGuiOnly = false;
	const DWORD colorForClear = 0x00000000;
#endif

	m_pd3dDev->Clear(0, NULL, D3DCLEAR_TARGET, colorForClear, 1.f, 0);

	if (!bGuiOnly)
	{
#ifndef TOUHOU_ON_D3D8
		ComPtr<IDirect3DStateBlock9> pStateBlock;
		m_pd3dDev->CreateStateBlock(D3DSBT_ALL, &pStateBlock);

		m_pd3dDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		m_pd3dDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		m_pd3dDev->SetSamplerState(0, D3DSAMP_MAGFILTER, m_pEditorContext->GetFilterType());
		m_pd3dDev->SetSamplerState(0, D3DSAMP_MINFILTER, m_pEditorContext->GetFilterType());
		m_pd3dDev->SetSamplerState(0, D3DSAMP_MIPFILTER, m_pEditorContext->GetFilterType());
		m_pd3dDev->SetSamplerState(0, D3DSAMP_MAXMIPLEVEL, 0);
		m_pd3dDev->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 0);
		m_pd3dDev->SetSamplerState(0, D3DSAMP_MIPMAPLODBIAS, 0);
		m_pd3dDev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);
#endif

		PossiblyScopedRS scopedRenderStates[] =
		{
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_ALPHABLENDENABLE, TRUE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_ALPHAFUNC, D3DCMP_GREATER),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_ALPHATESTENABLE, FALSE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_BLENDOP, D3DBLENDOP_ADD),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_CLIPPING, TRUE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_CLIPPLANEENABLE, FALSE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_CULLMODE, D3DCULL_NONE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_FILLMODE, D3DFILL_SOLID),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_FOGENABLE, FALSE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_LIGHTING, FALSE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_RANGEFOGENABLE, FALSE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_SHADEMODE, D3DSHADE_GOURAUD),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_SPECULARENABLE, FALSE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_SRCBLEND, D3DBLEND_SRCALPHA),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_STENCILENABLE, FALSE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_VERTEXBLEND, FALSE),
			PossiblyScopedRS(m_pd3dDev.Get(), D3DRS_WRAP0, 0),
		};

		PossiblyScopedTSS scopedTextureStageStates[] =
		{
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_COLORARG1, D3DTA_TEXTURE),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_COLOROP, D3DTOP_MODULATE),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_TEXCOORDINDEX, 0),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2),
			PossiblyScopedTSS(m_pd3dDev.Get(), 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE),
			PossiblyScopedTSS(m_pd3dDev.Get(), 1, D3DTSS_COLOROP, D3DTOP_DISABLE),
#ifdef TOUHOU_ON_D3D8
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_MAGFILTER, m_pEditorContext->GetFilterType()),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_MINFILTER, m_pEditorContext->GetFilterType()),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_MIPFILTER, m_pEditorContext->GetFilterType()),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_MAXMIPLEVEL, 0),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_MAXANISOTROPY, 0),
			PossiblyScopedTSS(m_pd3dDev.Get(), 0, D3DTSS_MIPMAPLODBIAS, 0),
#endif
		};

		PossiblyScopedTex scopedTexture(m_pd3dDev.Get(), 0, m_pTex.Get());
		PossiblyScopedStreamSource scopedStreamSource(m_pd3dDev.Get(), 0, m_pSpriteVertexBuffer.Get(), sizeof(THRotatorSpriteVertex));
		PossiblyScopedFVF scopedFVF(m_pd3dDev.Get(), THRotatorSpriteVertex::FVF);

		auto matrixProjection = DirectX::XMMatrixOrthographicOffCenterLH(
			0.0f, static_cast<float>(m_d3dpp.BackBufferWidth),
			static_cast<float>(m_d3dpp.BackBufferHeight), 0.0f,
			0.0f, 1.0f);
		auto matrixView = DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0.5f, 0.5f, -1.0f, 1.0f),
			DirectX::XMVectorSet(0.5f, 0.5f, 0.0f, 1.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

		PossiblyScopedTransform scopedTransforms[] =
		{
			PossiblyScopedTransform(m_pd3dDev.Get(), D3DTS_PROJECTION, ToD3DMATRIX(matrixProjection)),
			PossiblyScopedTransform(m_pd3dDev.Get(), D3DTS_VIEW, ToD3DMATRIX(matrixView)),
			PossiblyScopedTransform(m_pd3dDev.Get(), D3DTS_WORLD),
			PossiblyScopedTransform(m_pd3dDev.Get(), D3DTS_TEXTURE0),
		};

		EndSceneInternal();

#ifndef TOUHOU_ON_D3D8
		pStateBlock->Apply();
#endif
	}

	// Dirty texture transform matrix glitches ImGui.
	// Disabling texture transform matrix temporarily.
	DWORD previousTextureTransformFlags;
	m_pd3dDev->GetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, &previousTextureTransformFlags);
	m_pd3dDev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, FALSE);

	m_pEditorContext->RenderAndUpdateEditor(!m_d3dpp.Windowed);

	m_pd3dDev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, previousTextureTransformFlags);

#ifdef TOUHOU_ON_D3D8
	m_pd3dDev->SetRenderTarget(m_pRenderTarget.Get(), m_pDepthStencil.Get());
#else
	m_pd3dDev->SetRenderTarget(0, m_pRenderTarget.Get());
	m_pd3dDev->SetDepthStencilSurface(m_pDepthStencil.Get());
#endif

	return m_pd3dDev->EndScene();
}

void THRotatorDirect3DDevice::EndSceneInternal()
{
	auto rotationAngle = m_pEditorContext->GetRotationAngle();

	bool aspectLessThanRequested;
	if (rotationAngle % 2 == 0)
	{
		aspectLessThanRequested = m_d3dpp.BackBufferWidth * m_requestedHeight < m_d3dpp.BackBufferHeight * m_requestedWidth;
	}
	else
	{
		aspectLessThanRequested = m_d3dpp.BackBufferHeight * m_requestedHeight < m_d3dpp.BackBufferWidth * m_requestedWidth;
	}

	// To show enemy's mark, extend screen size.

	POINT mainScreenTopLeft = m_pEditorContext->GetMainScreenTopLeft();
	SIZE mainScreenSize = m_pEditorContext->GetMainScreenSize();
	if (mainScreenSize.cx * m_requestedWidth < mainScreenSize.cy * m_requestedHeight)
	{
		mainScreenTopLeft.x -= (mainScreenSize.cy * m_requestedHeight / m_requestedWidth - mainScreenSize.cx) / 2;
		mainScreenSize.cx = mainScreenSize.cy * m_requestedHeight / m_requestedWidth;
	}
	else if (mainScreenSize.cx * m_requestedWidth > mainScreenSize.cy * m_requestedHeight)
	{
		mainScreenTopLeft.y -= (mainScreenSize.cx * m_requestedWidth / m_requestedHeight - mainScreenSize.cy) / 2;
		mainScreenSize.cy = mainScreenSize.cx * m_requestedWidth / m_requestedHeight;
	}

	mainScreenTopLeft.y -= m_pEditorContext->GetYOffset();

	UINT correctedBaseScreenWidth = (std::max)(BASE_SCREEN_WIDTH, BASE_SCREEN_HEIGHT * m_requestedWidth / m_requestedHeight);
	UINT correctedBaseScreenHeight = (std::max)(BASE_SCREEN_HEIGHT, BASE_SCREEN_WIDTH * m_requestedHeight / m_requestedWidth);

	bool bNeedsRearrangeHUD = m_pEditorContext->IsHUDRearrangeForced()
		|| aspectLessThanRequested && m_pEditorContext->IsViewportSetCountOverThreshold();
	LONG baseDestRectWidth = bNeedsRearrangeHUD ? mainScreenSize.cx : static_cast<LONG>(correctedBaseScreenWidth);
	LONG baseDestRectHeight = bNeedsRearrangeHUD ? mainScreenSize.cy : static_cast<LONG>(correctedBaseScreenHeight);

	/***** Computation of transformation matrix *****

	Transformation steps are as follow:

	1. Scale dummy back buffer to 640x480
	2. Move the center of rectangle from the source poisition specified by user to the origin (0, 0)
	3. Scale the rectangle by the inverted size of source rectangle specified by user
	4. Rotate the rectangle by the angle specified by user
	5. Scale the rectangle by the size of destination rectangle specified by user
	6. Move the rectangle from the origin (0, 0) to the destination position specified by user
	7. Scale from main screen size to the resolution THRotator internally allocates
	8. Rotate the rectangle by current rotation of screen
	9. Move the center of screen from the origin (0, 0) to the center of back buffer

	Steps 2-6 require the parameters of a rectangle.
	Step 1, and Steps 7-9 is precomputed as preRectTransform and postRectTransform, respectively.

	*****/

	using namespace DirectX;

	XMMATRIX preRectTransform;
	{
		// step 1
		XMMATRIX baseSrcScale = XMMatrixScaling(correctedBaseScreenWidth / static_cast<float>(m_requestedWidth), correctedBaseScreenHeight / static_cast<float>(m_requestedHeight), 1.0f);

		preRectTransform = baseSrcScale;
	}

	XMMATRIX postRectTransform;
	{
		float baseDestScaleScalar = rotationAngle % 2 == 0 ?
			(std::min)(m_d3dpp.BackBufferWidth / static_cast<float>(baseDestRectWidth), m_d3dpp.BackBufferHeight / static_cast<float>(baseDestRectHeight)) :
			(std::min)(m_d3dpp.BackBufferWidth / static_cast<float>(baseDestRectHeight), m_d3dpp.BackBufferHeight / static_cast<float>(baseDestRectWidth));
		XMMATRIX baseDestScale = XMMatrixScaling(baseDestScaleScalar, baseDestScaleScalar, 1.0f);

		XMMATRIX rotation = XMMatrixRotationZ(RotationAngleToRadian(rotationAngle));

		XMMATRIX baseDestTranslation = XMMatrixTranslation(0.5f * m_d3dpp.BackBufferWidth, 0.5f * m_d3dpp.BackBufferHeight, 0.0f);

		postRectTransform = baseDestScale * rotation * baseDestTranslation;
	}

	auto rectDrawer = [this, &preRectTransform, &postRectTransform, correctedBaseScreenWidth, correctedBaseScreenHeight](
		const POINT& srcPos,
		const SIZE& srcSize,
		const POINT& destPos,
		const SIZE& destSize,
		const SIZE& mainScreenSize,
		RotationAngle rotation)
	{
		SIZE srcRectTopLeftOffset = {};

		XMMATRIX translateSrcCenterToOrigin = XMMatrixTranslation(
			-0.5f * srcSize.cx + srcRectTopLeftOffset.cx,
			-0.5f * srcSize.cy + srcRectTopLeftOffset.cy, 0.0f);

		XMMATRIX rectScaleSrcInv = XMMatrixScaling(1.0f / srcSize.cx, 1.0f / srcSize.cy, 1.0f);

		XMMATRIX rectRotation = XMMatrixRotationZ(RotationAngleToRadian(rotation));

		XMMATRIX rectScaleDest = XMMatrixScaling(static_cast<float>(destSize.cx), static_cast<float>(destSize.cy), 1.0f);

		XMMATRIX rectTranslation = XMMatrixTranslation(
			static_cast<float>(destPos.x) + 0.5f * (destSize.cx - mainScreenSize.cx),
			static_cast<float>(destPos.y) + 0.5f * (destSize.cy - mainScreenSize.cy), 0.0f);

		XMMATRIX finalTransform
			= preRectTransform
			* translateSrcCenterToOrigin
			* rectScaleSrcInv
			* rectRotation
			* rectScaleDest
			* rectTranslation
			* postRectTransform;

		RECT scaledSourceRect;
		float scaleFactorForSourceX = static_cast<float>(m_requestedWidth) / correctedBaseScreenWidth;
		float scaleFactorForSourceY = static_cast<float>(m_requestedHeight) / correctedBaseScreenHeight;
		scaledSourceRect.left = static_cast<LONG>(srcPos.x * scaleFactorForSourceX);
		scaledSourceRect.right = static_cast<LONG>((srcPos.x + srcSize.cx) * scaleFactorForSourceX);
		scaledSourceRect.top = static_cast<LONG>(srcPos.y * scaleFactorForSourceY);
		scaledSourceRect.bottom = static_cast<LONG>((srcPos.y + srcSize.cy) * scaleFactorForSourceY);

		XMMATRIX spriteVertexScale = XMMatrixScaling(srcSize.cx * scaleFactorForSourceX, srcSize.cy * scaleFactorForSourceY, 1.0f);
		finalTransform = spriteVertexScale * finalTransform;
		m_pd3dDev->SetTransform(D3DTS_WORLD, &ToD3DMATRIX(finalTransform));

		XMFLOAT4X4A textureMatrixScale;
		XMStoreFloat4x4A(&textureMatrixScale, XMMatrixScaling(srcSize.cx * scaleFactorForSourceX / m_requestedWidth, srcSize.cy * scaleFactorForSourceY / m_requestedHeight, 1.0f));
		textureMatrixScale.m[2][0] = srcPos.x * scaleFactorForSourceX / m_requestedWidth;
		textureMatrixScale.m[2][1] = srcPos.y * scaleFactorForSourceY / m_requestedHeight;

		m_pd3dDev->SetTransform(D3DTS_TEXTURE0, reinterpret_cast<const D3DMATRIX*>(&textureMatrixScale));

		m_pd3dDev->DrawPrimitive(THRotatorSpriteVertex::vertexBufferPrimitiveType, 0, THRotatorSpriteVertex::vertexBufferPrimitiveCount);
	};

	if (!bNeedsRearrangeHUD)
	{
		POINT pointZero = {};
		SIZE rectSize = { static_cast<LONG>(correctedBaseScreenWidth), static_cast<LONG>(correctedBaseScreenHeight) };
		rectDrawer(pointZero, rectSize, pointZero, rectSize, rectSize, Rotation_0);
	}
	else
	{
		POINT pointZero = {};
		rectDrawer(mainScreenTopLeft, mainScreenSize, pointZero, mainScreenSize, mainScreenSize, Rotation_0);

		for (const auto& rectData : m_pEditorContext->GetRectTransfers())
		{
			if (IsZeroSizedRectTransfer(rectData))
			{
				continue;
			}

			rectDrawer(rectData.sourcePosition, rectData.sourceSize, rectData.destPosition, rectData.destSize,
				mainScreenSize, rectData.rotation);
		}
	}
}

HRESULT WINAPI THRotatorDirect3DDevice::Present(CONST RECT *pSourceRect,
	CONST RECT *pDestRect,
	HWND hDestWindowOverride,
	CONST RGNDATA *pDirtyRegion)
{
	return InternalPresent(pSourceRect, pDestRect, hDestWindowOverride,
#ifdef TOUHOU_ON_D3D8
		pDirtyRegion);
#else
		pDirtyRegion, false, 0);
#endif
}

#ifndef TOUHOU_ON_D3D8
HRESULT WINAPI THRotatorDirect3DDevice::PresentEx(CONST RECT *pSourceRect,
	CONST RECT *pDestRect,
	HWND hDestWindowOverride,
	CONST RGNDATA *pDirtyRegion,
	DWORD dwFlags)
{
	return InternalPresent(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, true, dwFlags);
}
#endif

HRESULT THRotatorDirect3DDevice::InternalPresent(CONST RECT *pSourceRect,
	CONST RECT *pDestRect,
	HWND hDestWindowOverride,
#ifdef TOUHOU_ON_D3D8
	CONST RGNDATA *pDirtyRegion)
#else
	CONST RGNDATA *pDirtyRegion,
	bool bIsEx,
	DWORD dwFlags)
#endif
{
	m_pEditorContext->SubmitViewportSetCountToEditor();

	HRESULT ret;
#ifndef TOUHOU_ON_D3D8
	if (bIsEx)
	{
		ret = m_pd3dDevEx->PresentEx(pSourceRect, pDestRect,
			hDestWindowOverride, pDirtyRegion, dwFlags);
	}
	else
#endif
	{
		ret = m_pd3dDev->Present(pSourceRect, pDestRect,
			hDestWindowOverride, pDirtyRegion);
	}

	if (FAILED(ret))
	{
		return ret;
	}

	if (m_pEditorContext->ConsumeScreenCaptureRequest())
	{
		D3DSURFACE_DESC renderTargetDesc;
		m_pRenderTarget->GetDesc(&renderTargetDesc);

		D3DLOCKED_RECT lockedRect;
		if (SUCCEEDED(m_pRenderTarget->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY)))
		{
			namespace fs = boost::filesystem;

			auto snapshotDirectory = GetTouhouPlayerDataDirectory() / L"snapshot";

			fs::create_directory(snapshotDirectory);

			// search for available filename
			for (int i = 0; i >= 0; ++i)
			{
				fs::path filename = snapshotDirectory / fmt::format(L"throt{:03d}.bmp", i);
				if (fs::exists(filename))
				{
					continue;
				}

				SaveBitmapImage(filename.generic_wstring(),
					renderTargetDesc.Format, lockedRect.pBits,
					m_requestedWidth, m_requestedHeight,
					lockedRect.Pitch);

				break;
			}

			m_pRenderTarget->UnlockRect();
		}
	}

	if (m_resetRevision != m_pEditorContext->GetResetRevision())
	{
		return D3DERR_DEVICELOST;
	}

	return ret;
}

HRESULT WINAPI THRotatorDirect3DDevice::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
#ifdef TOUHOU_ON_D3D8
	return InternalReset(pPresentationParameters);
#else
	return InternalReset(pPresentationParameters, false, nullptr);
#endif
}

#ifndef TOUHOU_ON_D3D8
HRESULT WINAPI THRotatorDirect3DDevice::ResetEx(D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode)
{
	return InternalReset(pPresentationParameters, true, pFullscreenDisplayMode);
}
#endif

HRESULT THRotatorDirect3DDevice::InternalReset(
#ifdef TOUHOU_ON_D3D8
	D3DPRESENT_PARAMETERS* pPresentationParameters)
#else
	D3DPRESENT_PARAMETERS* pPresentationParameters, bool bIsEx, D3DDISPLAYMODEEX* pFullscreenDisplayMode)
#endif
{
	assert(m_bInitialized);

	OutputLogMessagef(LogSeverity::Info, "Resetting Direct3D device");

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

	UpdateResolution(creationParameters.AdapterOrdinal);

	HRESULT ret;
#ifndef TOUHOU_ON_D3D8
	if (bIsEx)
	{
		if (pFullscreenDisplayMode == nullptr)
		{
			ret = m_pd3dDevEx->ResetEx(&m_d3dpp, pFullscreenDisplayMode);
		}
		else
		{
			D3DDISPLAYMODEEX modeExtra = *pFullscreenDisplayMode;
			modeExtra.Width = m_d3dpp.BackBufferWidth;
			modeExtra.Height = m_d3dpp.BackBufferHeight;

			ret = m_pd3dDevEx->ResetEx(&m_d3dpp, &modeExtra);
		}
	}
	else
#endif
	{
		ret = m_pd3dDev->Reset(&m_d3dpp);
	}

	if (FAILED(ret))
	{
		return ret;
	}

	m_resetRevision = m_pEditorContext->GetResetRevision();
	ret = InitResources();

	if (FAILED(ret))
	{
		return ret;
	}

	// Reset() overwrites D3DPRESENT_PARAMETERS
	*pPresentationParameters = m_d3dpp;
	pPresentationParameters->BackBufferWidth = m_requestedWidth;
	pPresentationParameters->BackBufferHeight = m_requestedHeight;

	return ret;
}

extern "C"
{
#ifndef TOUHOU_ON_D3D8
	__declspec(dllexport) HRESULT WINAPI d_Direct3DCreate9Ex(UINT v, Direct3DExBase** pp)
	{
		auto pd3dEx = new THRotatorDirect3D();

		HRESULT ret = pd3dEx->InitEx(v);
		if (FAILED(ret))
		{
			pd3dEx->Release();
			return ret;
		}

		*pp = pd3dEx;
		return ret;
	}
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

		if (!pd3d->Init(v))
		{
			pd3d->Release();
			return nullptr;
		}

		return pd3d;
	}
}
