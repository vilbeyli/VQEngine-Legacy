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

#include "Light.h"
#include "Engine/GameObject.h"

#include "Utilities/Log.h"

#include <unordered_map>

using std::string;

DirectX::XMMATRIX Light::GetProjectionMatrix() const
{
	switch (mType)
	{
	case Light::POINT:
	{
		constexpr float ASPECT_RATIO = 1.0f; // cubemap aspect ratio
		return XMMatrixPerspectiveFovLH(PI_DIV2, ASPECT_RATIO, mNearPlaneDistance, mFarPlaneDistance);
	}
	case Light::SPOT:
	{
		constexpr float ASPECT_RATIO = 1.0f;
		//return XMMatrixPerspectiveFovLH(mSpotOuterConeAngleDegrees * DEG2RAD, ASPECT_RATIO, mNearPlaneDistance, mFarPlaneDistance);
		return XMMatrixPerspectiveFovLH(PI_DIV2, ASPECT_RATIO, mNearPlaneDistance, mFarPlaneDistance);
	}
	case Light::DIRECTIONAL:
	{
		if (mViewportX < 1.0f) return XMMatrixIdentity();
		return XMMatrixOrthographicLH(mViewportX, mViewportY, mNearPlaneDistance, mFarPlaneDistance);
	}
	default:
		Log::Warning("GetProjectionMatrix() called on invalid light type!");
		return XMMatrixIdentity();
	}
}

DirectX::XMMATRIX Light::GetViewMatrix(Texture::CubemapUtility::ECubeMapLookDirections lookDir /*= DEFAULT_POINT_LIGHT_LOOK_DIRECTION*/) const
{
	switch (mType)
	{
	case Light::POINT:
	{
		return Texture::CubemapUtility::GetViewMatrix(lookDir, mTransform._position);
	}
	case Light::SPOT:
	{
		XMVECTOR up = vec3::Up;
		XMVECTOR lookAt = vec3::Forward;	// spot light default orientation looks up
		lookAt = XMVector3TransformCoord(lookAt, mTransform.RotationMatrix());
		up = XMVector3TransformCoord(up, mTransform.RotationMatrix());
		XMVECTOR pos = mTransform._position;
		XMVECTOR taraget = pos + lookAt;
		return XMMatrixLookAtLH(pos, taraget, up);
	}
	case Light::DIRECTIONAL:
	{
		if (mViewportX < 1.0f) return XMMatrixIdentity();

		const XMVECTOR up = vec3::Up;
		const XMVECTOR lookAt = vec3::Zero;
		const XMMATRIX mRot = mTransform.RotationMatrix();
		const vec3 direction = XMVector3Transform(vec3::Forward, mRot);
		const XMVECTOR lightPos = direction * -mDistanceFromOrigin;	// away from the origin along the direction vector 
		return XMMatrixLookAtLH(lightPos, lookAt, up);
	}
	default:
		Log::Warning("GetViewMatrix() called on invalid light type!");
		return XMMatrixIdentity();
	}
}



void Light::GetGPUData(DirectionalLightGPU& l) const
{
	assert(mType == ELightType::DIRECTIONAL);
	const XMMATRIX mRot = mTransform.RotationMatrix();
	const vec3 direction = XMVector3Transform(vec3::Forward, mRot);

	l.brightness = this->mBrightness;
	l.color = this->mColor;

	l.lightDirection = direction;
	l.depthBias = this->mDepthBias;

	l.shadowing = this->mbCastingShadows;
	l.enabled = this->mbEnabled;
}
void Light::GetGPUData(SpotLightGPU& l) const
{
	assert(mType == ELightType::SPOT);
	const vec3 spotDirection = XMVector3TransformCoord(vec3::Forward, mTransform.RotationMatrix());

	l.position = mTransform._position;
	l.halfAngle = mSpotOuterConeAngleDegrees * DEG2RAD;

	l.color = mColor.Value();
	l.brightness = mBrightness;

	l.spotDir = spotDirection;
	l.depthBias = mDepthBias;

	l.innerConeAngle = mSpotInnerConeAngleDegrees * DEG2RAD;
}
void Light::GetGPUData(PointLightGPU& l) const
{
	assert(mType == ELightType::POINT);

	l.position = mTransform._position;
	l.range = mRange;

	l.color = mColor.Value();
	l.brightness = mBrightness;
	
	l.attenuation = vec3(mAttenuationConstant, mAttenuationLinear, mAttenuationQuadratic);
	l.depthBias = mDepthBias;
}

void Light::GetGPUData(CylinderLightGPU& l) const
{
	assert(mType == ELightType::CYLINDER);

	l.position = mTransform._position;
	l.color = mColor.Value();
	l.brightness = mBrightness;
	l.radius = mRadius;
	l.height = mHeight;
}

void Light::GetGPUData(RectangleLightGPU& l) const
{
	assert(mType == ELightType::RECTANGLE);

	l.position = mTransform._position;
	l.color = mColor.Value();
	l.brightness = mBrightness;
	l.width = mWidth;
	l.height = mHeight;
}

Settings::ShadowMap Light::GetSettings() const
{
	Settings::ShadowMap settings;
	settings.directionalShadowMapDimensions = static_cast<size_t>(mViewportX);
	return settings;
}

