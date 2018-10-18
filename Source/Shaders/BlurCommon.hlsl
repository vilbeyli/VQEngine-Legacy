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

#include "ShadingMath.hlsl"

inline bool IsOnImageBorder(uint px, uint imageSize) { return px == 0 || px == (imageSize - 1); }

inline void LoadLDSFromOutput(
	  const in uint TEXTURE_READ_COUNT_PER_THREAD
	, const in uint TEXTURE_READ_STRIDE

)
{
	[unroll] for (uint i = 0; i < TEXTURE_READ_COUNT_PER_THREAD; ++i)
	{
		//TODO: impl
//		const uint2 outTexel = dtxy + uint2(TEXTURE_READ_STRIDE * i, 0);

// USE CASES BELOW:
//--------------------------------------------------------------------------------------------------------------------
//		const uint2 outTexel = dispatchTID.xy + uint2(THREAD_GROUP_SIZE_X * i, 0);
//		if (outTexel.x >= IMAGE_SIZE_X)
//			break;
//		const half3 color = texColorOut[outTexel.yx].xyz;
//
//		if (IsOnImageBorder(outTexel.x, IMAGE_SIZE_X))
//		{
//			const int offset = outTexel.x / (IMAGE_SIZE_X - 1);
//			[unroll] for (int krn = 0; krn < KERNEL_RANGE_EXCLUDING_MIDDLE; ++krn)
//			{
//				gColorLine[outTexel.x + KERNEL_RANGE_EXCLUDING_MIDDLE * offset + krn + offset] = color.xyz;
//			}
//		}
//		gColorLine[outTexel.x + KERNEL_RANGE_EXCLUDING_MIDDLE] = color.xyz;
//--------------------------------------------------------------------------------------------------------------------
//		const uint2 outTexel = dispatchTID.xy + uint2(THREAD_GROUP_SIZE_X * i, 0);
//		if (outTexel.x >= IMAGE_SIZE_X)
//			break;
//
//		const float occlusion = texBlurredOcclusionOut[outTexel.xy];
//
//		const bool bFirstPixel = outTexel.x == 0;
//		const bool bLastPixel = outTexel.x == (IMAGE_SIZE_X - 1);
//		if (bFirstPixel || bLastPixel)
//		{
//			const int offset = outTexel.x / (IMAGE_SIZE_X - 1);
//			[unroll] for (int krn = 0; krn < KERNEL_RANGE_EXCLUDING_MIDDLE; ++krn)
//			{
//				gOcclusion[outTexel.x + KERNEL_RANGE_EXCLUDING_MIDDLE * offset + krn + offset] = occlusion;
//			}
//		}
//		gOcclusion[outTexel.x + KERNEL_RANGE_EXCLUDING_MIDDLE] = occlusion;
//--------------------------------------------------------------------------------------------------------------------
//#if VERTICAL
//		const uint2 outTexel = dispatchTID.xy + uint2(0, THREAD_GROUP_SIZE_Y * px_);
//		gColorLine[outTexel.y] = texColorOut[outTexel].xyz;
//#else
//		const uint2 outTexel = dispatchTID.xy + uint2(THREAD_GROUP_SIZE_X * px_, 0);
//		gColorLine[outTexel.x] = texColorOut[outTexel].xyz;
//#endif
//--------------------------------------------------------------------------------------------------------------------
	}
}

void LoadLDSFromInput()
{

}