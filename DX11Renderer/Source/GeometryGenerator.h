#pragma once

#include <DirectXMath.h>
using namespace DirectX;

class BufferObject;
struct ID3D11Device;

class GeometryGenerator
{
public:
	GeometryGenerator();
	~GeometryGenerator();

	void SetDevice(ID3D11Device* dev) { m_device = dev; }

	BufferObject* Triangle();
	BufferObject* Quad();
	BufferObject* Cube();
	BufferObject* Sphere();

private:
	ID3D11Device* m_device;
};

