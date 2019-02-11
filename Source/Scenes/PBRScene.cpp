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

#include "PBRScene.h"
#include "Application/Input.h"
#include "Engine/Engine.h"

#include "Renderer/Renderer.h"

#undef max


#if DO_NOT_LOAD_SCENES
void PBRScene::Load(SerializedScene& scene) {}
void PBRScene::Unload() {}
void PBRScene::Update(float dt) {}
void PBRScene::RenderUI() const {}
#else
void PBRScene::Load(SerializedScene& scene)
{
	mTimeAccumulator = 0.0f;

	SetEnvironmentMap(EEnvironmentMapPresets::TROPICAL_RUINS);
}

void PBRScene::Unload()
{

}

void PBRScene::Update(float dt)
{

}

void PBRScene::RenderUI() const {}
#endif