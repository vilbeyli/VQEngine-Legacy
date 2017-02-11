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

// forward decl
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
	BufferObject* Sphere(float radius, unsigned ringCount, unsigned sliceCount);
	BufferObject* Grid(float width, float depth, unsigned m, unsigned n);
	BufferObject* Cylinder(float height, float topRadius, float bottomRadius, unsigned sliceCount, unsigned stackCount);

private:
	ID3D11Device* m_device;
};

