#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

using namespace DirectX;

class Shader
{
	friend class Renderer;

public:
	Shader(const std::string& shaderFileName);
	~Shader();

	virtual void Begin(ID3D11DeviceContext* deviceContext, int idxCount);
	virtual void End(ID3D11DeviceContext* deviceContext);

	bool SetShaderParameters(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* tex);
	bool SetShaderParameters(ID3D11DeviceContext* deviceContext, XMFLOAT4X4 worldMatrix, XMFLOAT4X4 viewMatrix, XMFLOAT4X4 projectionMatrix);

	std::string	GetName() const;
	bool		IsInitialized() const;

protected:
	//virtual bool Init(ID3D11Device* device, HWND hwnd, WCHAR* shaderFileName);

private:
	bool Compile(ID3D11Device* device, HWND hwnd, WCHAR* shaderFileName);
	void OutputShadeErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFileName);

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader*	m_pixelShader;
	ID3D11InputLayout*	m_layout;
	ID3D11Buffer*		m_matrixBuffer;
	std::string			m_name;
	//ShaderID			m_id;

private:
	struct MatrixBuffer
	{
		XMMATRIX mWorld;
		XMMATRIX mView;
		XMMATRIX mProj;
	};
};

