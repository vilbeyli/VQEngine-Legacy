//	DX11Renderer - VDemo | DirectX11 Renderer
//	Copyright(C) 2016  - Volkan Ilbeyli
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
#include <GameObject.h>
#include <memory>

// forward decl
class Renderer;

class Skydome
{
	typedef int ShaderID;

public:
	Skydome();
	~Skydome();

	void Render(const XMMATRIX& view, const XMMATRIX& proj) const;
	void Init(Renderer* renderer_in, const char* tex, float scale, int shader);
private:
	GameObject	skydomeObj;
	TextureID	skydomeTex;
	ShaderID	skydomeShader;
	Renderer*	pRenderer;
};

