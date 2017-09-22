#pragma once

#include <vector>
#include <boost/filesystem.hpp>
#include <tchar.h>
#include <cstdint>
#include <string>

#ifdef TOUHOU_ON_D3D8
#include <d3d8.h>
#else
#include <d3d9.h>
#endif

#include <fmt/format.h>

#include <wrl.h>
