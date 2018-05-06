// (c) 2018 massanoori

#pragma once

bool InitializeExports();
void FinalizeExports();

/**
* Creators of Direct3D interfaces.
*/

#if TOUHOU_ON_D3D8

// Direct3D 8
IDirect3D8* CallOriginalDirect3DCreate(UINT version);
#else

// Direct3D 9
IDirect3D9* CallOriginalDirect3DCreate(UINT version);

// Direct3D 9 Ex
HRESULT CallOriginalDirect3DCreate9Ex(UINT version, IDirect3D9Ex** direct3d);
#endif
