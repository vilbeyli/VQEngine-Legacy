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

namespace VQEngine
{
class UI
{
public:
	UI(const std::vector<Mesh>& BuiltInMeshes) : mBuiltInMeshes(BuiltInMeshes) {}

	void Initialize(Renderer* pRenderer, TextRenderer* pTextRenderer);
	void Exit();

	void RenderBackground(const vec3& color, const vec2& size, const vec2& screenPosition) const;


private:
	const std::vector<Mesh>& mBuiltInMeshes;

	Renderer*		mpRenderer;
	TextRenderer*	mpTextRenderer;
};
}