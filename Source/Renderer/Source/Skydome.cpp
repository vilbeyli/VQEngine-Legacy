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
	renderer->Reset();
	renderer->SetShader(0);

	XMMATRIX world = skydomeObj.m_transform.WorldTransformationMatrix();
	renderer->SetConstant4x4f("world", world);
	renderer->SetConstant4x4f("view", view);
	renderer->SetConstant4x4f("proj", proj);
	renderer->SetConstant1f("isDiffuseMap", 1.0f);
	renderer->SetTexture("gDiffuseMap", skydomeTex);
	renderer->SetConstant3f("diffuse", XMFLOAT3(1.0f, 1.0f, 1.0f));	// must set this or yellow?
	renderer->SetBufferObj(MESH_TYPE::SPHERE);
	renderer->Apply();
	renderer->DrawIndexed();
}


void Skydome::Init(pRenderer renderer_in, const char* tex, float scale, int shader)
{
	renderer = renderer_in;
	skydomeObj.m_transform.SetScaleUniform(scale);
	skydomeTex = renderer->AddTexture(tex, "Data/Textures/");
	skydomeShader = shader;	
}