//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com

#include "Utilities/Log.h"


///// https://walbourn.github.io/dxgi-debug-device/
///// enable debug device 
///#include <dxgi1_3.h>
#include <dxgidebug.h>

#define ENABLE_D3D_LIVE_OBJECT_REPORTING 0
#define ENABLE_D3D_RESOURCE_DEBUG_NAMES 0

#define xFORCE_DEBUG
#if USE_DX12

#endif