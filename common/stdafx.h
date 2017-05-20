#pragma once

#include <vector>
#include <boost/filesystem.hpp>
#include <tchar.h>
#include <cstdint>

#ifdef TOUHOU_ON_D3D8
#include <d3d8.h>
#include <d3dx8.h>

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
#include <d3d9.h>
#include <d3dx9.h>

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
