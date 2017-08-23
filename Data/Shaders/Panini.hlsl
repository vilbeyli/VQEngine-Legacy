// experimenting with panini projection
// source: https://docs.unrealengine.com/latest/INT/Engine/Rendering/PostProcessEffects/PaniniProjection/
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

	const float2 ScreenSpaceToPaniniFactor = tan(0.5f * fovH);
	const float2 PaniniToScreenSpaceFactor = 1.0f / ScreenSpaceToPaniniFactor;

	const float2 PaniniDirection = InScreenPos * ScreenSpaceToPaniniFactor;
	const float2 PaniniPosition = PaniniProjection(PaniniDirection, D, S);

	// Return the new position back in the screen space frame
	return PaniniPosition * PaniniToScreenSpaceFactor * OutScreenPosScale;
}
