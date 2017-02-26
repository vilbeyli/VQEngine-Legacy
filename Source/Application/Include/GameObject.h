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

#define xENABLE_PHY_CODE

#include "Components/Transform.h"
#include "Model.h"

#ifdef ENABLE_PHY_CODE
#include "RigidBody.h"
#endif

class GameObject
{
public:
	GameObject();
	~GameObject();
	GameObject(const GameObject& obj);

	GameObject& operator=(const GameObject& obj);

public:
	Transform	m_transform;
	Model		m_model;
#ifdef ENABLE_PHY_CODE
	RigidBody	m_rb;
#endif
};

