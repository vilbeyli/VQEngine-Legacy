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

float2 PaniniProjection(float2 OM, float d, float s)
{
	const float PaniniDirectionXZInvLength = rsqrt(1.0f + OM.x * OM.x);
	const float SinPhi = OM.x * PaniniDirectionXZInvLength;
	const float TanTheta = OM.y * PaniniDirectionXZInvLength;
	const float CosPhi = sqrt(1.0f - SinPhi * SinPhi);
	const float S = (d + 1.0f) / (d + CosPhi);
	return S * float2(SinPhi, lerp(TanTheta, TanTheta / CosPhi, s));
}

float2 PaniniProjectionScreenPosition(float2 InScreenPos, float fovH)
{
	const float D = 1.0f;    const float S = 0.0f;
	const float OutScreenPosScale = 1.0f;

#if 1
	const float2 ScreenSpaceToPaniniFactor = tan(0.5f * fovH);
	const float2 PaniniToScreenSpaceFactor = 1.0f / ScreenSpaceToPaniniFactor;
#else
	const float2 ScreenSpaceToPaniniFactor = float2(-0.839,0);// GetTanHalfFieldOfView();
	const float2 PaniniToScreenSpaceFactor = float2(0,0);// GetCotanHalfFieldOfView();
#endif

	const float2 PaniniDirection = InScreenPos * ScreenSpaceToPaniniFactor;
	const float2 PaniniPosition = PaniniProjection(PaniniDirection, D, S);

	// Return the new position back in the screen space frame
	return PaniniPosition * PaniniToScreenSpaceFactor * OutScreenPosScale;
}
