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




void UI::RenderBackground(const vec3& color, float alpha, const vec2& size, const vec2& screenPosition) const
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
	mpRenderer->SetConstant1f("alpha", alpha);
	mpRenderer->SetConstant4x4f("worldViewProj", matTransformation);
	mpRenderer->SetBlendState(EDefaultBlendState::ALPHA_BLEND);
	mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
	mpRenderer->Apply();
	mpRenderer->DrawIndexed();

}



VQEngine::UI::UI(const std::vector<Mesh>& BuiltInMeshes, const EngineConfig& engineConfig)
	: mBuiltInMeshes(BuiltInMeshes)
	, mEngineControls(engineConfig)
{}

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


// todo: more stats
// - meshes
// - main view cull count
// - shadow view cull count per light type
//
const char* FrameStats::statNames[FrameStats::numStat] =
{
	"FPS : ",
	// spaces are there to align the final result with the current font.
	"Vertices      : ",
	"Indices        : ",
	"Draw Calls : ",
	"Triangles    : ",

	"# Objects        : ",
	"# Spot Lights  : ",
	"# Point Lights : ",

	"[Cull] MainView  : ",
	"[Cull] SpotViews : ",
	"[Cull] PointViews: ",
	//"[Cull] DirectionalView : ",
	"[Cull] PointLights: ",
};
constexpr size_t RENDER_ORDER_FRAME_STATS_ROW_1[] = { 0, 3, 4, 1, 2};
constexpr size_t RENDER_ORDER_FRAME_STATS_ROW_2[] = { 5, 6, 7, 8, 9, 10, 11 };

auto GetFPSColor = [](int FPS) -> LinearColor
{
	if (FPS > 144)			return LinearColor::medium_pruple;
	else if (FPS > 90)		return LinearColor::cyan;
	else if (FPS > 60)		return LinearColor::green;
	else if (FPS > 33)		return LinearColor::yellow;
	else if (FPS > 20)		return LinearColor::orange;
	else					return LinearColor::red;
};

// TODO: data drive these and call it a day.
// Background constants
constexpr float X_MARGIN_PX = 10.0f;    // leave 10px margin on the X-axis to cover
constexpr float Y_OFFSET_PX = 24.0f;    // we offset Y equal to height of a letter to fit the background on text
constexpr float BACKGROUND_NORMALIZED_LENGTH_X = 0.252f;
constexpr float BACKGROUND_ALPHA = 0.6f;
static const LinearColor sBackgroundColor = LinearColor::black;

constexpr float X_NORMALIZED_POSITION_CONTROLS = 0.005f;
constexpr float Y_NORMALIZED_POSITION_CONTROLS = 0.009f;


constexpr float X_NORMALIZED_POSITION_FRAME_STATS = 0.745f;
constexpr float Y_NORMALIZED_POSITION_FRAME_STATS = 0.550f;

constexpr float X_NORMALIZED_POSITION_PROFILER_CPU = X_NORMALIZED_POSITION_FRAME_STATS;
constexpr float Y_NORMALIZED_POSITION_PROFILER_CPU = 0.665f;

constexpr float CPU_GPU_GAP = 0.115f;
constexpr float X_NORMALIZED_POSITION_PROFILER_GPU = X_NORMALIZED_POSITION_FRAME_STATS + CPU_GPU_GAP;
constexpr float Y_NORMALIZED_POSITION_PROFILER_GPU = Y_NORMALIZED_POSITION_PROFILER_CPU;

constexpr float LINE_HEIGHT_IN_PX = 17.0f;
constexpr int PX_OFFSET_FRAMESTATS_PERFNUMBERS = 40;

void VQEngine::UI::RenderPerfStats(const FrameStats& stats) const
{
	TextDrawDescription drawDesc;
	drawDesc.color = LinearColor::white;
	drawDesc.scale = 0.28f;

	const vec2 screenSizeInPixels = mpRenderer->GetWindowDimensionsAsFloat2();

	// these are all pixel positions, starting from top left corner.
	const float X_PX_POS_FRAMESTATS = screenSizeInPixels.x() * X_NORMALIZED_POSITION_FRAME_STATS;
	const float Y_PX_POS_FRAMESTATS = screenSizeInPixels.y() * Y_NORMALIZED_POSITION_FRAME_STATS;
	const vec2  PX_POS_FRAMESTATS   = vec2(X_PX_POS_FRAMESTATS, Y_PX_POS_FRAMESTATS);

	const float X_PX_POS_PROFILER_CPU = screenSizeInPixels.x() * X_NORMALIZED_POSITION_PROFILER_CPU;
	const float Y_PX_POS_PROFILER_CPU = Y_PX_POS_FRAMESTATS + LINE_HEIGHT_IN_PX * (sizeof(RENDER_ORDER_FRAME_STATS_ROW_1) / sizeof(size_t)) + PX_OFFSET_FRAMESTATS_PERFNUMBERS;
	const vec2  PX_POS_PROFILER_CPU = vec2(X_PX_POS_PROFILER_CPU, Y_PX_POS_PROFILER_CPU);

	const float X_PX_POS_PROFILER_GPU = screenSizeInPixels.x() * X_NORMALIZED_POSITION_PROFILER_GPU;
	const float Y_PX_POS_PROFILER_GPU = Y_PX_POS_FRAMESTATS + LINE_HEIGHT_IN_PX * (sizeof(RENDER_ORDER_FRAME_STATS_ROW_1) / sizeof(size_t)) + PX_OFFSET_FRAMESTATS_PERFNUMBERS;
	const vec2  PX_POS_PROFILER_GPU = vec2(X_PX_POS_PROFILER_GPU, Y_PX_POS_PROFILER_GPU);

	const bool bSortStats = true;

	mpRenderer->BeginEvent("Perf Stats UI Text");

	// BACKGROUND
	//
	const vec2 CPUProfilerAreaBounds = mProfilerStack.pCPU->GetEntryAreaBounds(screenSizeInPixels);
	const vec2 GPUProfilerAreaBounds = mProfilerStack.pGPU->GetEntryAreaBounds(screenSizeInPixels);
	const vec2 ProfilerAreaBounds(BACKGROUND_NORMALIZED_LENGTH_X, std::max(CPUProfilerAreaBounds.y(), GPUProfilerAreaBounds.y()) );

	vec2 sz = ProfilerAreaBounds +vec2(0.0f, (8 * LINE_HEIGHT_IN_PX) / screenSizeInPixels.y());
	vec2 pos = PX_POS_FRAMESTATS - vec2(X_MARGIN_PX, Y_OFFSET_PX);
	RenderBackground(sBackgroundColor, BACKGROUND_ALPHA, sz, pos);

	// PROFILER STATS
	//
	const size_t cpu_perf_rows = mProfilerStack.pCPU->RenderPerformanceStats(mpTextRenderer, PX_POS_PROFILER_CPU, drawDesc, bSortStats);
	mProfilerStack.pGPU->RenderPerformanceStats(mpTextRenderer, PX_POS_PROFILER_GPU, drawDesc, bSortStats);


	// FRAME STATS
	//
	mpTextRenderer->RenderText(drawDesc);
	for (size_t i = 0; i < sizeof(RENDER_ORDER_FRAME_STATS_ROW_1) / sizeof(size_t); ++i)
	{
		drawDesc.screenPosition = vec2(PX_POS_FRAMESTATS.x(), PX_POS_FRAMESTATS.y() + i * LINE_HEIGHT_IN_PX);
		drawDesc.color = RENDER_ORDER_FRAME_STATS_ROW_1[i] == 0	// if we're drawing the FPS
			? GetFPSColor(stats[RENDER_ORDER_FRAME_STATS_ROW_1[i]])
			: LinearColor::white;
		drawDesc.text = FrameStats::statNames[RENDER_ORDER_FRAME_STATS_ROW_1[i]] + StrUtil::CommaSeparatedNumber(std::to_string(stats[RENDER_ORDER_FRAME_STATS_ROW_1[i]]));
		mpTextRenderer->RenderText(drawDesc);
	}
	const float offset = CPU_GPU_GAP * screenSizeInPixels.x();
	for (size_t i = 0; i < sizeof(RENDER_ORDER_FRAME_STATS_ROW_2) / sizeof(size_t); ++i)
	{
		drawDesc.screenPosition = vec2(PX_POS_FRAMESTATS.x() + offset, PX_POS_FRAMESTATS.y() + i * LINE_HEIGHT_IN_PX);
		drawDesc.color = LinearColor::white;
		drawDesc.text = FrameStats::statNames[RENDER_ORDER_FRAME_STATS_ROW_2[i]] + StrUtil::CommaSeparatedNumber(std::to_string(stats[RENDER_ORDER_FRAME_STATS_ROW_2[i]]));
		mpTextRenderer->RenderText(drawDesc);
	}
	mpRenderer->EndEvent();


}


void VQEngine::UI::RenderEngineControls() const
{
	const vec2 screenSizeInPixels = mpRenderer->GetWindowDimensionsAsFloat2();

	const float X_PX_POS_CONTROLS = screenSizeInPixels.x()  * X_NORMALIZED_POSITION_CONTROLS;
	const float Y_PX_POS_CONTROLS = screenSizeInPixels.y() * Y_NORMALIZED_POSITION_CONTROLS;
	const vec2 PX_POS_CONTROLS = vec2(X_PX_POS_CONTROLS, Y_PX_POS_CONTROLS);


	TextDrawDescription drawDesc;
	drawDesc.color = LinearColor::white;
	drawDesc.scale = 0.28f;
	drawDesc.color = vec3(1, 1, 0.1f) * 0.65f;
	int numLine = FrameStats::numStat + 1;

	const float LINE_HEIGHT_PX = 18.0f;
	const float longestStringLength = static_cast<float>(std::string("F5 - Toggle Rendering AABBs: Off").size());

	mpRenderer->BeginEvent("Render Controls UI Text");

	// todo: rename/remove the magic numbers.
	auto ToogleToString = [](const bool b) { return b ? "On" : "Off"; };
	const std::vector<std::string> ControlEntries =
	{	// this might use some optimization. maybe. profile this.
		std::string("F1 - Toggle Displaying Controls"),
		std::string("F2 - SSAO: ") + ToogleToString(mEngineControls.bSSAO),
		std::string("F3 - Bloom: ") + ToogleToString(mEngineControls.bBloom),
		std::string("F4 - Display Render Targets: ") + ToogleToString(mEngineControls.bRenderTargets),
		std::string(" "),
		std::string("F5 - Toggle Rendering AABBs: ") + ToogleToString(mEngineControls.bBoundingBoxes),
		std::string("F6 - Render Mode: ") + (!mEngineControls.bDeferredOrForward ? "Forward" : "Deferred"),


//#if _DEBUG
//		std::string("F7 - Frustum Cull Main View: ") + ToogleToString(mEngineControls.bBoundingBoxes),
//		std::string("F8 - Frustum Cull Local Lights: ") + ToogleToString(mEngineControls.bBoundingBoxes),
//		std::string(" "),
//		std::string("F9 - Sort Render Lists: ") + ToogleToString(mEngineControls.bBoundingBoxes),
//#endif
	};
	const float backGroundLineSpan = static_cast<float>(ControlEntries.size() + 2);	// back ground a little bigger - 2 lines bigger.
	const float avgLetterWidthInPixels = 7.0f;		// hardcoded... determines background width

	vec2 sz = vec2(avgLetterWidthInPixels * longestStringLength / screenSizeInPixels.x(), LINE_HEIGHT_PX * backGroundLineSpan / screenSizeInPixels.y());
	RenderBackground(sBackgroundColor, BACKGROUND_ALPHA, sz, PX_POS_CONTROLS - vec2(X_MARGIN_PX, X_MARGIN_PX));


	int line = 1;
	std::for_each(RANGE(ControlEntries), [&](const std::string& Entry)
	{
		drawDesc.screenPosition = vec2(PX_POS_CONTROLS.x(), PX_POS_CONTROLS.y() + line++ * LINE_HEIGHT_PX);
		drawDesc.text = Entry;
		mpTextRenderer->RenderText(drawDesc);
	});

	mpRenderer->EndEvent();
}

