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

#include "Skydome.h"
#include "Renderer.h"

Skydome::Skydome(){}
Skydome::~Skydome(){}

void Skydome::Render(const XMMATRIX& view, const XMMATRIX& proj) const
{
	pRenderer->SetShader(SHADERS::UNLIT);

	XMMATRIX world = skydomeObj.m_transform.WorldTransformationMatrix();
	pRenderer->SetConstant4x4f("world", world);
	pRenderer->SetConstant4x4f("view", view);
	pRenderer->SetConstant4x4f("proj", proj);
	pRenderer->SetConstant1f("isDiffuseMap", 1.0f);
	pRenderer->SetTexture("gDiffuseMap", skydomeTex);
	pRenderer->SetConstant3f("diffuse", vec3(1.0f, 1.0f, 1.0f));	// must set this or yellow?
	pRenderer->SetBufferObj(MESH_TYPE::SPHERE);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
}


void Skydome::Initialize(Renderer* renderer, const char* tex, float scale, int shader)
{
	pRenderer = renderer;
	skydomeObj.m_transform.SetUniformScale(scale);
	skydomeTex = renderer->TextureFromFile(tex, "Data/Textures/")._id;
	skydomeShader = shader;	
}