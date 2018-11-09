#pragma once

#define STRICT
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#define WINVER _WIN32_WINNT_WIN7
#define WIN32_WINNT _WIN32_WINNT_WIN7

#include <windows.h>
#include <windowsx.h>
#include <VersionHelpers.h>
#include <stddef.h>

#include <utility>
#include <memory>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <random>
#include <ctime>
#include <stack>
#include <chrono>
#include <shared_mutex>
#include <sstream>
#include <locale>
#include <codecvt>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include <dxgi1_5.h>
#include <d3d11.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#include <DirectXMath.h>
using namespace DirectX;

#include "ThreadPool.h"
#include "Utils.h"

#include "Z80.h"
