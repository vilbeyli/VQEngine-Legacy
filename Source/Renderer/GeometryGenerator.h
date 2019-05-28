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

#pragma once

#include "Engine/Mesh.h"

#include "Renderer.h"

namespace GeometryGenerator
{
	Mesh Triangle(float scale);
	Mesh Quad(float scale);
	Mesh FullScreenQuad();
	Mesh Cube();
	Mesh Sphere(float radius, unsigned ringCount, unsigned sliceCount, int numLODLevels = 1);
	Mesh Grid(float width, float depth, unsigned m, unsigned n, int numLODLevels = 1);
	Mesh Cylinder(float height, float topRadius, float bottomRadius, unsigned sliceCount, unsigned stackCount, int numLODLevels = 1);
	Mesh Cone(float height, float radius, unsigned sliceCount, int numLODLevels = 1);

	bool Is2DGeometry(EGeometry meshID);
	void CalculateTangentsAndBitangents(std::vector<DefaultVertexBufferData>& vertices, const std::vector<unsigned> indices);	// Only Tangents
};

