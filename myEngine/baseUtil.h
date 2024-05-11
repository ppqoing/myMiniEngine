#pragma once
#include"logSystem/logSystem.h"
#include"Sigleton.h"
#include"config_manager/config_manager.h"



#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include<iostream>
#include<atlstr.h>
#include<map>
using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace std;
enum psoStyle
{
	default_pso = 0,
	skinned_pso,
	shadow_map,
	skin_shadow,
	drawing_normal,
	skin_drawing_normal,
	ssao_pso,
	sky_pso,
};