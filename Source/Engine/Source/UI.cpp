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

#include "UI.h"
#include "Transform.h"
#include "DataStructures.h"

#include "Renderer/Renderer.h"
#include "Renderer/TextRenderer.h"

#include "Utilities/vectormath.h"
#include "Utilities/Profiler.h"

using namespace VQEngine;



// Background constants
const float X_MARGIN_PX = 10.0f;    // leave 3px margin on the X-axis to cover
const float Y_OFFSET_PX = 24.0f;    // we offset Y equal to height of a letter to fit the background on text
static const LinearColor sBackgroundColor = LinearColor::black;

void UI::RenderBackground(const vec3& color, const vec2& size, const vec2& screenPosition) const
{
	assert(mpRenderer);
	const vec2 windowSizeXY = mpRenderer->GetWindowDimensionsAsFloat2();
	const vec2 CoordsNDC = (screenPosition );// *2.0f - vec2(1.0f, 1.0f); // [-1, +1]
	//const vec2 

	Transform tf;
	tf.SetScale(size.x(), size.y(), 1.0f);

	const vec2 pos = ( (vec2(CoordsNDC.x(), -CoordsNDC.y()) ) / windowSizeXY) * 2.0 - vec2(1.0f, -1.0f) + vec2(tf._scale.x(), -tf._scale.y());
	tf.SetPosition(pos.x(), pos.y(), 0.0f);
	
	const XMMATRIX proj = XMMatrixOrthographicLH(windowSizeXY.x(), windowSizeXY.y(), 0.1f, 1000.0f);
	const XMMATRIX matTransformation = tf.WorldTransformationMatrix();
	const auto IABuffers = mBuiltInMeshes[EGeometry::FULLSCREENQUAD].GetIABuffers();

	mpRenderer->SetShader(EShaders::UNLIT);
	mpRenderer->SetVertexBuffer(IABuffers.first);
	mpRenderer->SetIndexBuffer(IABuffers.second);
	mpRenderer->SetConstant3f("diffuse", color);
	mpRenderer->SetConstant3f("isDiffuseMap", 0.0f);
	mpRenderer->SetConstant1f("alpha", 0.6f);
	mpRenderer->SetConstant4x4f("worldViewProj", matTransformation);
	mpRenderer->SetBlendState(EDefaultBlendState::ALPHA_BLEND);
	mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
	mpRenderer->Apply();
	mpRenderer->DrawIndexed();

}



void VQEngine::UI::Initialize(Renderer* pRenderer, TextRenderer* pTextRenderer, ProfilerStack& profilers)
{
	mpRenderer = pRenderer;
	mpTextRenderer = pTextRenderer;
	mProfilerStack = profilers;
}

void VQEngine::UI::Exit()
{
	; //empty
}



const char* FrameStats::statNames[FrameStats::numStat] =
{
	"FPS: ",
	"Objects: ",
	"Culled Objects: ",
	"Vertices: ",
	"Indices: ",
	"Draw Calls: ",
};

void VQEngine::UI::RenderPerfStats(const FrameStats& stats) const
{
	TextDrawDescription drawDesc;
	drawDesc.color = LinearColor::white;
	drawDesc.scale = 0.28f;

	const vec2 screenSizeInPixels = mpRenderer->GetWindowDimensionsAsFloat2();

	// these are all pixel positions, starting from top left corner.
	float X_PX_POS_PROFILER = screenSizeInPixels.x() * 0.790f;
	float Y_PX_POS_PROFILER = screenSizeInPixels.y() * 0.700f;
	const  vec2 PX_POS_PROFILER = vec2(X_PX_POS_PROFILER, Y_PX_POS_PROFILER);


	const float X_PX_POS_FRAMESTATS = screenSizeInPixels.x()  * 0.812f;
	      float Y_PX_POS_FRAMESTATS = screenSizeInPixels.y() * 0.915f;
	const  vec2 PX_POS_FRAMESTATS = vec2(X_PX_POS_FRAMESTATS, Y_PX_POS_FRAMESTATS);

	const bool bSortStats = true;

	mpRenderer->BeginEvent("Perf Stats UI Text");

	// CPU STATS
	vec2 sz = mProfilerStack.pCPU->GetEntryAreaBounds(screenSizeInPixels);
	vec2 pos = PX_POS_PROFILER - vec2(X_MARGIN_PX, Y_OFFSET_PX);

	RenderBackground(sBackgroundColor, sz, pos);
	const size_t cpu_perf_rows = mProfilerStack.pCPU->RenderPerformanceStats(mpTextRenderer, PX_POS_PROFILER, drawDesc, bSortStats);

#if 0
	// advance Y position for GPU stats
	// +30 px down the CPU results, start rendering GPU results
	Y_PX_POS_PROFILER += cpu_perf_rows * PERF_TREE_ENTRY_DRAW_Y_OFFSET_PER_LINE + 30;
#else
	X_PX_POS_PROFILER += sz.x() * screenSizeInPixels.x() * 0.9f;
#endif


	// GPU STATS
	sz = mProfilerStack.pGPU->GetEntryAreaBounds(screenSizeInPixels);
	pos = vec2(X_PX_POS_PROFILER, Y_PX_POS_PROFILER) - vec2(X_MARGIN_PX, Y_OFFSET_PX);

	RenderBackground(sBackgroundColor, sz, pos);
	mProfilerStack.pGPU->RenderPerformanceStats(mpTextRenderer, vec2(X_PX_POS_PROFILER, Y_PX_POS_PROFILER), drawDesc, bSortStats);

	mpRenderer->EndEvent();

	// todo: rename/remove the magic numbers.
	constexpr float LINE_HEIGHT_IN_PX = 17.0f;
	const float rowcount = FrameStats::numStat;
	const float backGroundLineSpan = static_cast<float>(rowcount + 2);	// back ground a little bigger - 2 lines bigger.
	const float avgLetterWidthInPixels = 7.0f;		// hardcoded... determines background width
	const float longestStringLength = static_cast<float>(std::string("Culled Objects: 10000").size());

	sz = vec2(avgLetterWidthInPixels * longestStringLength, backGroundLineSpan * LINE_HEIGHT_IN_PX) / screenSizeInPixels;
	pos = vec2(PX_POS_FRAMESTATS.x(), PX_POS_FRAMESTATS.y()) - vec2(X_MARGIN_PX, Y_OFFSET_PX);
	RenderBackground(sBackgroundColor, sz, pos);



	// FRAME STATS
	//
	mpRenderer->BeginEvent("Frame Stats UI Text");
	constexpr size_t RENDER_ORDER[] = { 0, 5, 3, 4, 1, 2 };

	auto GetFPSColor = [](int FPS) -> LinearColor
	{
		if (FPS > 90)			return LinearColor::cyan;
		else if (FPS > 60)		return LinearColor::green;
		else if (FPS > 33)		return LinearColor::yellow;
		else if (FPS > 20)		return LinearColor::orange;
		else					return LinearColor::red;
	};
	
	mpTextRenderer->RenderText(drawDesc);
	for (size_t i = 0; i < FrameStats::numStat; ++i)
	{
		drawDesc.screenPosition = vec2(PX_POS_FRAMESTATS.x(), PX_POS_FRAMESTATS.y() + i * LINE_HEIGHT_IN_PX);
		drawDesc.color = RENDER_ORDER[i] == 0	// if we're drawing the FPS
			? GetFPSColor(stats[RENDER_ORDER[i]])
			: LinearColor::white;
		drawDesc.text = FrameStats::statNames[RENDER_ORDER[i]] + StrUtil::CommaSeparatedNumber(std::to_string(stats[RENDER_ORDER[i]]));
		mpTextRenderer->RenderText(drawDesc);
	}
	mpRenderer->EndEvent();
}


void VQEngine::UI::RenderEngineControls(EngineControlsUIData&& controls) const
{
	const vec2 screenSizeInPixels = mpRenderer->GetWindowDimensionsAsFloat2();

	const float X_PX_POS_CONTROLS = screenSizeInPixels.x()  * 0.89f;
	float Y_PX_POS_CONTROLS = screenSizeInPixels.y() * 0.80f;
	vec2 PX_POS_CONTROLS = vec2(X_PX_POS_CONTROLS, Y_PX_POS_CONTROLS);


	TextDrawDescription drawDesc;
	drawDesc.color = LinearColor::white;
	drawDesc.scale = 0.28f;
	drawDesc.color = vec3(1, 1, 0.1f) * 0.65f;
	int numLine = FrameStats::numStat + 1;

	const float LINE_HEIGHT_PX = 18.0f;
	const float longestStringLength = static_cast<float>(std::string("F5 - Toggle Rendering AABBs: Off").size());

	mpRenderer->BeginEvent("Render Controls UI Text");

	// todo: rename/remove the magic numbers.
	const float rowcount = 5.0f;					// controls end in F5 -> 5 rows...
	const float backGroundLineSpan = static_cast<float>(rowcount + 2);	// back ground a little bigger - 2 lines bigger.
	const float avgLetterWidthInPixels = 7.0f;		// hardcoded... determines background width

	vec2 sz = vec2(avgLetterWidthInPixels * longestStringLength / screenSizeInPixels.x(), backGroundLineSpan * LINE_HEIGHT_PX / screenSizeInPixels.y());
	vec2 pos = vec2(PX_POS_CONTROLS.x(), PX_POS_CONTROLS.y() + numLine * LINE_HEIGHT_PX) - vec2(X_MARGIN_PX, Y_OFFSET_PX);

	RenderBackground(sBackgroundColor, sz, pos);

	auto ToogleToString = [](const bool b) { return b ? "On" : "Off"; };
	const std::vector<std::string> ControlEntries = 
	{	// this might use some optimization. maybe. profile this.
		std::string("F1 - Render Mode: ") + (controls.bForwardOrDeferred ? "Forward" : "Deferred"),
		std::string("F2 - SSAO: ") + ToogleToString(controls.bSSAO),
		std::string("F3 - Bloom: ") + ToogleToString(controls.bBloom),
		std::string("F4 - Display Render Targets: ") + ToogleToString(controls.bRenderTargets),
		std::string("F5 - Toggle Rendering AABBs: ") + ToogleToString(controls.bBoundingBoxes)
	};

	std::for_each(RANGE(ControlEntries), [&](const std::string& Entry)
	{
		drawDesc.screenPosition = vec2(PX_POS_CONTROLS.x(), PX_POS_CONTROLS.y() + numLine++ * LINE_HEIGHT_PX);
		drawDesc.text = Entry;
		mpTextRenderer->RenderText(drawDesc);
	});

	mpRenderer->EndEvent();
}

