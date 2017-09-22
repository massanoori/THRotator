// (c) 2017 massanoori

#pragma once

#ifdef TOUHOU_ON_D3D8

typedef IDirect3D8 Direct3DBase;
typedef IDirect3D8 Direct3DExBase;
typedef IDirect3DDevice8 Direct3DDeviceBase;
typedef IDirect3DDevice8 Direct3DDeviceExBase;
typedef IDirect3DSurface8 Direct3DSurfaceBase;
typedef IDirect3DTexture8 Direct3DTextureBase;
typedef IDirect3DBaseTexture8 Direct3DBaseTextureBase;
typedef IDirect3DVertexBuffer8 Direct3DVertexBufferBase;
typedef IDirect3DIndexBuffer8 Direct3DIndexBufferBase;
typedef IDirect3DSwapChain8 Direct3DSwapChainBase;

#else

typedef IDirect3D9 Direct3DBase;
typedef IDirect3D9Ex Direct3DExBase;
typedef IDirect3DDevice9 Direct3DDeviceBase;
typedef IDirect3DDevice9Ex Direct3DDeviceExBase;
typedef IDirect3DSurface9 Direct3DSurfaceBase;
typedef IDirect3DTexture9 Direct3DTextureBase;
typedef IDirect3DBaseTexture9 Direct3DBaseTextureBase;
typedef IDirect3DVertexBuffer9 Direct3DVertexBufferBase;
typedef IDirect3DIndexBuffer9 Direct3DIndexBufferBase;
typedef IDirect3DSwapChain9 Direct3DSwapChainBase;
#endif

// Structs for updating and restoring device states by RAII

struct ScopedRS
{
	Direct3DDeviceBase* pd3dDevice;
	D3DRENDERSTATETYPE renderStateType;
	DWORD previousValue;

	ScopedRS(Direct3DDeviceBase* inDevice, D3DRENDERSTATETYPE inRenderStateType, DWORD value)
		: pd3dDevice(inDevice), renderStateType(inRenderStateType)
	{
		pd3dDevice->GetRenderState(renderStateType, &previousValue);
		pd3dDevice->SetRenderState(renderStateType, value);
	}

	~ScopedRS()
	{
		pd3dDevice->SetRenderState(renderStateType, previousValue);
	}
};

struct ScopedTSS
{
	Direct3DDeviceBase* pd3dDevice;
	DWORD textureStage;
	D3DTEXTURESTAGESTATETYPE textureStageStateType;
	DWORD previousValue;

	ScopedTSS(Direct3DDeviceBase* inDevice, DWORD inTextureStage, D3DTEXTURESTAGESTATETYPE inTextureStageStateType, DWORD value)
		: pd3dDevice(inDevice)
		, textureStage(inTextureStage)
		, textureStageStateType(inTextureStageStateType)
	{
		pd3dDevice->GetTextureStageState(textureStage, textureStageStateType, &previousValue);
		pd3dDevice->SetTextureStageState(textureStage, textureStageStateType, value);
	}

	~ScopedTSS()
	{
		pd3dDevice->SetTextureStageState(textureStage, textureStageStateType, previousValue);
	}
};

struct ScopedTex
{
	Direct3DDeviceBase* pd3dDevice;
	DWORD textureStage;
	Microsoft::WRL::ComPtr<Direct3DBaseTextureBase> previousTexture;

	ScopedTex(Direct3DDeviceBase* inDevice, DWORD inTextureStage, Direct3DBaseTextureBase* texture)
		: pd3dDevice(inDevice)
		, textureStage(inTextureStage)
	{
		pd3dDevice->GetTexture(textureStage, &previousTexture);
		pd3dDevice->SetTexture(textureStage, texture);
	}

	ScopedTex(Direct3DDeviceBase* inDevice, DWORD inTextureStage)
		: pd3dDevice(inDevice)
		, textureStage(inTextureStage)
	{
		pd3dDevice->GetTexture(textureStage, &previousTexture);
	}

	~ScopedTex()
	{
		pd3dDevice->SetTexture(textureStage, previousTexture.Get());
	}
};

struct ScopedTransform
{
	Direct3DDeviceBase* pd3dDevice;
	D3DTRANSFORMSTATETYPE transformStateType;
	D3DMATRIX previousTransformMatrix;

	ScopedTransform(Direct3DDeviceBase* inDevice, D3DTRANSFORMSTATETYPE inTransformType, const D3DMATRIX& transformMatrix)
		: pd3dDevice(inDevice)
		, transformStateType(inTransformType)
	{
		pd3dDevice->GetTransform(transformStateType, &previousTransformMatrix);
		pd3dDevice->SetTransform(transformStateType, &transformMatrix);
	}

	ScopedTransform(Direct3DDeviceBase* inDevice, D3DTRANSFORMSTATETYPE inTransformType)
		: pd3dDevice(inDevice)
		, transformStateType(inTransformType)
	{
		pd3dDevice->GetTransform(transformStateType, &previousTransformMatrix);
	}

	~ScopedTransform()
	{
		pd3dDevice->SetTransform(transformStateType, &previousTransformMatrix);
	}
};

#ifdef TOUHOU_ON_D3D8

struct ScopedStreamSource
{
	Direct3DDeviceBase* pd3dDevice;
	Microsoft::WRL::ComPtr<Direct3DVertexBufferBase> previousVertexBuffer;
	UINT streamNumber;
	UINT previousStride;

	ScopedStreamSource(Direct3DDeviceBase* inDevice, UINT inStreamNumber, Direct3DVertexBufferBase* vertexBuffer, UINT stride)
		: pd3dDevice(inDevice)
		, streamNumber(inStreamNumber)
	{
		pd3dDevice->GetStreamSource(streamNumber, &previousVertexBuffer, &previousStride);
		pd3dDevice->SetStreamSource(streamNumber, vertexBuffer, stride);
	}

	ScopedStreamSource(Direct3DDeviceBase* inDevice, UINT inStreamNumber)
		: pd3dDevice(inDevice)
		, streamNumber(inStreamNumber)
	{
		pd3dDevice->GetStreamSource(streamNumber, &previousVertexBuffer, &previousStride);
	}

	~ScopedStreamSource()
	{
		pd3dDevice->SetStreamSource(streamNumber, previousVertexBuffer.Get(), previousStride);
	}
};

struct ScopedIndices
{
	Direct3DDeviceBase* pd3dDevice;
	Microsoft::WRL::ComPtr<Direct3DIndexBufferBase> previousIndexBuffer;
	UINT previousOffset;

	ScopedIndices(Direct3DDeviceBase* inDevice, Direct3DIndexBufferBase* indices, UINT offset)
		: pd3dDevice(inDevice)
	{
		pd3dDevice->GetIndices(&previousIndexBuffer, &previousOffset);
		pd3dDevice->SetIndices(indices, offset);
	}

	ScopedIndices(Direct3DDeviceBase* inDevice)
		: pd3dDevice(inDevice)
	{
		pd3dDevice->GetIndices(&previousIndexBuffer, &previousOffset);
	}

	~ScopedIndices()
	{
		pd3dDevice->SetIndices(previousIndexBuffer.Get(), previousOffset);
	}
};

struct ScopedVS
{
	Direct3DDeviceBase* pd3dDevice;
	DWORD previousVertexShader;

	ScopedVS(Direct3DDeviceBase* inDevice, DWORD vertexShader)
		: pd3dDevice(inDevice)
	{
		pd3dDevice->GetVertexShader(&previousVertexShader);
		pd3dDevice->SetVertexShader(vertexShader);
	}

	ScopedVS(Direct3DDeviceBase* inDevice)
		: pd3dDevice(inDevice)
	{
		pd3dDevice->GetVertexShader(&previousVertexShader);
	}

	~ScopedVS()
	{
		pd3dDevice->SetVertexShader(previousVertexShader);
	}
};

using ScopedFVF = ScopedVS;

struct ScopedPS
{
	Direct3DDeviceBase* pd3dDevice;
	DWORD previousPixelShader;

	ScopedPS(Direct3DDeviceBase* inDevice, DWORD pixelShader)
		: pd3dDevice(inDevice)
	{
		pd3dDevice->GetPixelShader(&previousPixelShader);
		pd3dDevice->SetPixelShader(pixelShader);
	}

	ScopedPS(Direct3DDeviceBase* inDevice)
		: pd3dDevice(inDevice)
	{
		pd3dDevice->GetPixelShader(&previousPixelShader);
	}

	~ScopedPS()
	{
		pd3dDevice->SetPixelShader(previousPixelShader);
	}
};

struct ScopedVP
{
	Direct3DDeviceBase* pd3dDevice;
	D3DVIEWPORT8 previousViewport;

	ScopedVP(Direct3DDeviceBase* inDevice, const D3DVIEWPORT8& viewport)
		: pd3dDevice(inDevice)
	{
		pd3dDevice->GetViewport(&previousViewport);
		pd3dDevice->SetViewport(&viewport);
	}

	ScopedVP(Direct3DDeviceBase* inDevice)
		: pd3dDevice(inDevice)
	{
		pd3dDevice->GetViewport(&previousViewport);
	}

	~ScopedVP()
	{
		pd3dDevice->SetViewport(&previousViewport);
	}
};

#endif
