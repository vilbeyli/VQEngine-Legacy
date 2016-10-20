#include "Skydome.h"
#include "Renderer.h"

Skydome::Skydome(){}
Skydome::~Skydome(){}

void Skydome::Render(Renderer* renderer, const XMMATRIX& view, const XMMATRIX& proj) const
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


void Skydome::Init(Renderer* renderer_in, const char* tex, float scale, int shader)
{
	renderer = renderer_in;
	skydomeObj.m_transform.SetScaleUniform(scale);
	skydomeTex = renderer->AddTexture(tex, "Data/Textures/");
	skydomeShader = shader;	
}