// (c) 2018 massanoori

#pragma once

#include "Direct3DUtils.h"

Direct3DBase* THRotatorDirect3DCreate(UINT version);

#ifndef TOUHOU_ON_D3D8
HRESULT THRotatorDirect3DCreateEx(UINT version, Direct3DExBase** ppD3D);
#endif
