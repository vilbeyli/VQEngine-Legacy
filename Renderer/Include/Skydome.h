#pragma once
#include <GameObject.h>

// forward decl
class Renderer;


class Skydome
{
	typedef int ShaderID;

public:
	Skydome();
	~Skydome();

	void Render(Renderer* renderer, const XMMATRIX& view, const XMMATRIX& proj) const;
	void Init(Renderer* renderer_in, const char* tex, float scale, int shader);
private:
	GameObject	skydomeObj;
	TextureID	skydomeTex;
	ShaderID	skydomeShader;
	Renderer*	renderer;
};

