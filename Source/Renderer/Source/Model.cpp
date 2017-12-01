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

#include "Model.h"

void Model::SetDiffuseColor(const LinearColor & diffuseColor)
{
	mBlinnPhong_Material.diffuse = mBRDF_Material.diffuse = diffuseColor;
}
void Model::SetDiffuseAlpha(const LinearColor & diffuseColor, float alpha)
{
	mBlinnPhong_Material.diffuse = mBRDF_Material.diffuse = diffuseColor;
	mBlinnPhong_Material.alpha = mBRDF_Material.alpha = alpha;
}

void Model::SetNormalMap(const TextureID normalMap)
{
	mBlinnPhong_Material.normalMap = mBRDF_Material.normalMap = normalMap;
}

void Model::SetDiffuseMap(const TextureID diffuseMap)
{
	mBlinnPhong_Material.diffuseMap = mBRDF_Material.diffuseMap = diffuseMap;
}

void Model::SetTextureTiling(const vec2 & tiling)
{
	mBlinnPhong_Material.tiling = mBRDF_Material.tiling = tiling;
}