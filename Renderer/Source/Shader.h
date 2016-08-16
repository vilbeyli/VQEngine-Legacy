#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

using namespace DirectX;

typedef int ShaderID;

struct MatrixBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProj;
};

class Shader
{
	friend class Renderer;

public:
	Shader(const std::string& shaderFileName);
	~Shader();

	std::string	Name() const;
	ShaderID	ID() const;

private:
	void Compile(ID3D11Device* device, HWND hwnd, const std::string& shaderFileName);
	void OutputShaderErrorMessage(ID3D10Blob* errorMessage, const CHAR* shaderFileName);
	void HandleCompileError(ID3D10Blob* errorMessage, const std::string& shdPath);
	void AssignID(ShaderID id);

	ID3D11VertexShader*			m_vertexShader;
	ID3D11PixelShader*			m_pixelShader;
	ID3D11InputLayout*			m_layout;
	ID3D11Buffer*				m_matrixBuffer;
	const std::string			m_name;
	ShaderID					m_id;

};

