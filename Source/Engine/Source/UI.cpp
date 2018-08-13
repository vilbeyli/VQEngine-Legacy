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

#include "Renderer/Renderer.h"
#include "Renderer/TextRenderer.h"

#include "Utilities/vectormath.h"

using namespace VQEngine;

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
	const auto IABuffers = mBuiltInMeshes[EGeometry::QUAD].GetIABuffers();

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

void VQEngine::UI::Initialize(Renderer* pRenderer, TextRenderer* pTextRenderer)
{
	mpRenderer = pRenderer;
	mpTextRenderer = pTextRenderer;
}

void VQEngine::UI::Exit()
{
	; //empty
}

