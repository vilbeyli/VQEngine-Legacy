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


#include "Engine.h"
#include "Scene.h"

#include "Application/Application.h"

#include "Renderer/Renderer.h"

void Engine::HandleInput()
{
	if (mpInput->IsKeyUp("ESC"))
	{
		if (mbMouseCaptured)
		{
			mbMouseCaptured = false;
			mpApp->CaptureMouse(mbMouseCaptured);
		}
		else if (!this->IsLoading())
		{
			mbStartEngineShutdown = true;
		}
	}

	if (mpInput->IsKeyTriggered("Backspace"))	TogglePause();

	if (mpInput->IsKeyTriggered("F1")) ToggleControlsTextRendering();
	if (mpInput->IsKeyTriggered("F2")) ToggleAmbientOcclusion();
	if (mpInput->IsKeyTriggered("F3")) ToggleBloom();
	if (mpInput->IsKeyTriggered("F4")) mEngineConfig.bRenderTargets = !mEngineConfig.bRenderTargets;
	if (mpInput->IsKeyTriggered("F5")) mEngineConfig.bBoundingBoxes = !mEngineConfig.bBoundingBoxes;
	if (mpInput->IsKeyTriggered("F6")) ToggleRenderingPath();

	//if (mpInput->IsKeyTriggered("'")) 
	if (mpInput->IsKeyTriggered("F"))// && mpInput->AreKeysDown(2, "ctrl", "shift"))
	{
		if (mpInput->IsKeyDown("ctrl") && mpInput->IsKeyDown("shift"))
			ToggleProfilerRendering();
	}

	// The following input will not be handled if the engine is currently loading a level
	if (mbLoading)
		return;

	// ----------------------------------------------------------------------------------------------

	if (mpInput->IsKeyTriggered("\\")) mpRenderer->ReloadShaders();

#if SSAO_DEBUGGING
	// todo: wire this to some UI text/control
	if (mEngineConfig.bSSAO)
	{
		if (mpInput->IsKeyDown("Shift") && mpInput->IsKeyDown("Ctrl"))
		{
			if (mpInput->IsWheelUp())		mAOPass.ChangeAOTechnique(+1);
			if (mpInput->IsWheelDown())	mAOPass.ChangeAOTechnique(-1);
		}
		else if (mpInput->IsKeyDown("Shift"))
		{
			const float step = 0.1f;
			if (mpInput->IsWheelUp()) { mAOPass.intensity += step; Log::Info("SSAO Intensity: %.2f", mAOPass.intensity); }
			if (mpInput->IsWheelDown()) { mAOPass.intensity -= step; if (mAOPass.intensity < 0.301) mAOPass.intensity = 1.0f; Log::Info("SSAO Intensity: %.2f", mAOPass.intensity); }
			//if (mpInput->IsMouseDown(Input::EMouseButtons::MOUSE_BUTTON_MIDDLE)) mAOPass.ChangeAOQuality(-1);
		}
		else if (mpInput->IsKeyDown("Ctrl"))
		{
			if (mpInput->IsWheelUp())		mAOPass.ChangeBlurQualityLevel(+1);
			if (mpInput->IsWheelDown())	mAOPass.ChangeBlurQualityLevel(-1);
		}
		else
		{
			const float step = 0.5f;
			if (mpInput->IsWheelUp()) { mAOPass.radius += step; Log::Info("SSAO Radius: %.2f", mAOPass.radius); }
			if (mpInput->IsWheelDown()) { mAOPass.radius -= step; if (mAOPass.radius < 0.301) mAOPass.radius = 1.0f; Log::Info("SSAO Radius: %.2f", mAOPass.radius); }
			//if (mpInput->IsMouseDown(Input::EMouseButtons::MOUSE_BUTTON_MIDDLE)) mAOPass.ChangeAOQuality(+1);
			if (mpInput->IsKeyTriggered("K")) mAOPass.ChangeAOQuality(+1);
			if (mpInput->IsKeyTriggered("J")) mAOPass.ChangeAOQuality(-1);
		}
	}
#endif

#if BLOOM_DEBUGGING
	if (mEngineConfig.bBloom)
	{
		Settings::Bloom& bloom = mPostProcessPass._settings.bloom;
		float& threshold = bloom.brightnessThreshold;
		int& blurStrength = bloom.blurStrength;
		const float step = 0.05f;
		const float threshold_hi = 3.0f;
		const float threshold_lo = 0.05f;
		if (mpInput->IsWheelUp() && !mpInput->IsKeyDown("Shift") && !mpInput->IsKeyDown("Ctrl")) { threshold += step; if (threshold > threshold_hi) threshold = threshold_hi; Log::Info("Bloom Brightness Cutoff Threshold: %.2f", threshold); }
		if (mpInput->IsWheelDown() && !mpInput->IsKeyDown("Shift") && !mpInput->IsKeyDown("Ctrl")) { threshold -= step; if (threshold < threshold_lo) threshold = threshold_lo; Log::Info("Bloom Brightness Cutoff Threshold: %.2f", threshold); }
		if (mpInput->IsWheelUp() && mpInput->IsKeyDown("Shift")) { blurStrength += 1; Log::Info("Bloom Blur Strength = %d", blurStrength); }
		if (mpInput->IsWheelDown() && mpInput->IsKeyDown("Shift")) { blurStrength -= 1; if (blurStrength == 0) blurStrength = 1; Log::Info("Bloom Blur Strength = %d", blurStrength); }
		if ((mpInput->IsWheelDown() || mpInput->IsWheelUp()) && mpInput->IsKeyDown("Ctrl"))
		{
			const int direction = mpInput->IsWheelDown() ? -1 : +1;
			int nextShader = mPostProcessPass._bloomPass.mSelectedBloomShader + direction;
			if (nextShader < 0)
				nextShader = BloomPass::BloomShader::NUM_BLOOM_SHADERS - 1;
			if (nextShader == BloomPass::BloomShader::NUM_BLOOM_SHADERS)
				nextShader = 0;
			mPostProcessPass._bloomPass.mSelectedBloomShader = static_cast<BloomPass::BloomShader>(nextShader);
		}
	}
#endif

#if FULLSCREEN_DEBUG_TEXTURE
	if (mpInput->IsKeyTriggered("Space")) mbOutputDebugTexture = !mbOutputDebugTexture;
#endif

#if GBUFFER_DEBUGGING
	if (mpInput->IsWheelPressed())
	{
		mDeferredRenderingPasses.mbUseDepthPrepass = !mDeferredRenderingPasses.mbUseDepthPrepass;
		Log::Info("GBuffer Depth PrePass: ", (mDeferredRenderingPasses.mbUseDepthPrepass ? "Enabled" : "Disabled"));
	}
#endif

	// SCENE -------------------------------------------------------------
	if (mpInput->IsKeyTriggered("1"))	mLevelLoadQueue.push(0);
	if (mpInput->IsKeyTriggered("2"))	mLevelLoadQueue.push(1);
	if (mpInput->IsKeyTriggered("3"))	mLevelLoadQueue.push(2);
	if (mpInput->IsKeyTriggered("4"))	mLevelLoadQueue.push(3);
	if (mpInput->IsKeyTriggered("5"))	mLevelLoadQueue.push(4);
	if (mpInput->IsKeyTriggered("6"))	mLevelLoadQueue.push(5);
	if (mpInput->IsKeyTriggered("7"))	mLevelLoadQueue.push(6);
	if (mpActiveScene)
	{
		if (mpInput->IsKeyTriggered("R"))
		{
			if (mpInput->IsKeyDown("Shift")) ReloadScene();
			else							 mpActiveScene->ResetActiveCamera();
		}

		// index using enums. first element of environment map presets starts with cubemap preset count, as if both lists were concatenated.
		constexpr EEnvironmentMapPresets firstPreset = static_cast<EEnvironmentMapPresets>(CUBEMAP_PRESET_COUNT);
		constexpr EEnvironmentMapPresets lastPreset = static_cast<EEnvironmentMapPresets>(
			static_cast<EEnvironmentMapPresets>(CUBEMAP_PRESET_COUNT) + ENVIRONMENT_MAP_PRESET_COUNT - 1
			);

		EEnvironmentMapPresets selectedEnvironmentMap = mpActiveScene->GetActiveEnvironmentMapPreset();
		if (selectedEnvironmentMap == -1)
			return; // if no skymap is selected, ignore input to change it
		if (ENGINE->INP()->IsKeyTriggered("PageUp"))	selectedEnvironmentMap = selectedEnvironmentMap == lastPreset ? firstPreset : static_cast<EEnvironmentMapPresets>(selectedEnvironmentMap + 1);
		if (ENGINE->INP()->IsKeyTriggered("PageDown"))	selectedEnvironmentMap = selectedEnvironmentMap == firstPreset ? lastPreset : static_cast<EEnvironmentMapPresets>(selectedEnvironmentMap - 1);
		if (ENGINE->INP()->IsKeyTriggered("PageUp") || ENGINE->INP()->IsKeyTriggered("PageDown"))
			mpActiveScene->SetEnvironmentMap(selectedEnvironmentMap);
	}
}