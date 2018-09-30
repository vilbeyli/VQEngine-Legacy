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

// GAUSSIAN BLUR 1D KERNELS  
//---------------------------------------------------------------------------------------------------------------------------------- 
// src: http://dev.theomader.com/gaussian-kernel-calculator/
// Sigma: 1.7515
// Kernel Size: [9, 21]: odd numbers
//
// App should define the following:
// #define KERNEL_DIMENSION 5
//----------------------------------------------------------------------------------------------------------------------------------


// KERNEL_RANGE is this: center + half width of the kernel
//
// consider a blur kernel 5x5 - '*' indicates the center of the kernel
//
// x   x   x   x   x
// x   x   x   x   x
// x   x   x*  x   x
// x   x   x   x   x
// x   x   x   x   x
//
//
// separate into 1D kernels
//
// x   x   x*  x   x
//         ^-------^
//        KERNEL_RANGE
//
#ifndef KERNEL_RANGE
#define KERNEL_RANGE  ((KERNEL_DIMENSION - 1) / 2) + 1
#endif

const half KERNEL_WEIGHTS[KERNEL_RANGE] =
//----------------------------------------------------------------------------------------------------------------------------------
#if KERNEL_RANGE == 5
	#if USE_LEARNOPENGL_KERNEL
		{ 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
	#else
		{ 0.22703, 0.193731, 0.120373, 0.054452, 0.017929 };
	#endif
#endif
#if KERNEL_RANGE == 6
{ 0.225096, 0.192081, 0.119348, 0.053988, 0.017776, 0.004259 };
#endif
#if KERNEL_RANGE == 7
{ 0.224762, 0.191796, 0.119171, 0.053908, 0.01775, 0.004253, 0.000741 };
#endif
#if KERNEL_RANGE == 8
{ 0.22472, 0.19176, 0.119148, 0.053898, 0.017747, 0.004252, 0.000741, 0.000094 };
#endif
#if KERNEL_RANGE == 9
{ 0.224716, 0.191757, 0.119146, 0.053897, 0.017746, 0.004252, 0.000741, 0.000094, 0.000009 };
#endif
#if KERNEL_RANGE == 10
{ 0.224716, 0.191756, 0.119146, 0.053897, 0.017746, 0.004252, 0.000741, 0.000094, 0.000009, 0.000001 };
#endif
#if KERNEL_RANGE == 11
{ 0.224716, 0.191756, 0.119146, 0.053897, 0.017746, 0.004252, 0.000741, 0.000094, 0.000009, 0.000001, 0 };
#endif
//----------------------------------------------------------------------------------------------------------------------------------