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

#pragma once

#include "Engine/Mesh.h"


class Renderer;
class TextRenderer;

struct vec3;
class CPUProfiler;
class GPUProfiler;
struct FrameStats;
struct EngineConfig;

namespace VQEngine
{
class UI
{
public:
	struct ProfilerStack
	{
		CPUProfiler* pCPU = nullptr;
		GPUProfiler* pGPU = nullptr;
	};


public:
	UI(const EngineConfig& engineConfig);

	void Initialize(Renderer* pRenderer, TextRenderer* pTextRenderer, ProfilerStack& profilers);
	void Update();
	void Exit();


	static void RenderBackground(Renderer* mpRenderer, const vec3& color, float alpha, const vec2& size, const vec2& screenPosition);

	void RenderPerfStats(const FrameStats& stats) const;
	void RenderEngineControls() const;

private:
	const EngineConfig& mEngineControls;

	Renderer*		mpRenderer;
	TextRenderer*	mpTextRenderer;

	ProfilerStack	mProfilerStack;

};
}