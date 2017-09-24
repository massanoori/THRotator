// (c) 2017 massanoori

#pragma once

#ifdef TOUHOU_ON_D3D8

using Direct3DBase = IDirect3D8;
using Direct3DExBase = IDirect3D8;
using Direct3DDeviceBase = IDirect3DDevice8;
using Direct3DDeviceExBase = IDirect3DDevice8;
using Direct3DSurfaceBase = IDirect3DSurface8;
using Direct3DTextureBase = IDirect3DTexture8;
using Direct3DBaseTextureBase = IDirect3DBaseTexture8;
using Direct3DCubeTextureBase = IDirect3DCubeTexture8;
using Direct3DVolumeTextureBase = IDirect3DVolumeTexture8;
using Direct3DVertexBufferBase = IDirect3DVertexBuffer8;
using Direct3DIndexBufferBase = IDirect3DIndexBuffer8;
using Direct3DSwapChainBase = IDirect3DSwapChain8;
using LockedPointer = BYTE*;

#define ARG_DECLARE_SHARED_HANDLE(handleVarName)
#define ARG_PASS_SHARED_HANDLE(handleVarName)

#define INVALID_STATE_BLOCK_HANDLE 0xFFFFFFFF

#else

using Direct3DBase = IDirect3D9;
using Direct3DExBase = IDirect3D9Ex;
using Direct3DDeviceBase = IDirect3DDevice9;
using Direct3DDeviceExBase = IDirect3DDevice9Ex;
using Direct3DSurfaceBase = IDirect3DSurface9;
using Direct3DTextureBase = IDirect3DTexture9;
using Direct3DBaseTextureBase = IDirect3DBaseTexture9;
using Direct3DCubeTextureBase = IDirect3DCubeTexture9;
using Direct3DVolumeTextureBase = IDirect3DVolumeTexture9;
using Direct3DVertexBufferBase = IDirect3DVertexBuffer9;
using Direct3DIndexBufferBase = IDirect3DIndexBuffer9;
using Direct3DSwapChainBase = IDirect3DSwapChain9;
using LockedPointer = void*;

#define ARG_DECLARE_SHARED_HANDLE(handleVarName) , HANDLE* handleVarName
#define ARG_PASS_SHARED_HANDLE(handleVarName) , handleVarName

#endif
