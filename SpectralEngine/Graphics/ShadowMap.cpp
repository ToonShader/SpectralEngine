#include "ShadowMap.h"

using namespace Spectral::Graphics;

namespace
{
	XMFLOAT4 DEFAULT_LIGHT_UP_VECTOR = { 0.0f, 1.0f, 0.0f, 0.0f };
	float DEFAULT_LIGHT_NEAR_Z = 0.1f;
	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX SHADOW_NDC_TO_TEX_COORD (
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	void UpdateDirectionalShadowTransform(ShadowMap& shadowMap)
	{
		XMVECTOR lightDir = XMLoadFloat3(&shadowMap.SceneLight->Direction);
		XMVECTOR lightPos = (-2.0f * shadowMap.BoundingSphere.Radius * lightDir) + XMLoadFloat3(&shadowMap.BoundingSphere.Center);
		XMVECTOR targetPos = XMLoadFloat3(&shadowMap.BoundingSphere.Center);
		XMVECTOR lightUp = XMLoadFloat4(&DEFAULT_LIGHT_UP_VECTOR);
		XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

		XMStoreFloat3(&shadowMap.LightVirtualPosW, lightPos);

		// Transform bounding sphere to light space.
		XMFLOAT3 sphereCenterLS;
		XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

		// Orthographic frustum in light space encloses scene.
		float l = sphereCenterLS.x - shadowMap.BoundingSphere.Radius;
		float b = sphereCenterLS.y - shadowMap.BoundingSphere.Radius;
		float n = sphereCenterLS.z - shadowMap.BoundingSphere.Radius;
		float r = sphereCenterLS.x + shadowMap.BoundingSphere.Radius;
		float t = sphereCenterLS.y + shadowMap.BoundingSphere.Radius;
		float f = sphereCenterLS.z + shadowMap.BoundingSphere.Radius;

		shadowMap.LightNearZ = n;
		shadowMap.LightFarZ = f;
		XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

		XMMATRIX shadowTransform = lightView * lightProj * SHADOW_NDC_TO_TEX_COORD;
		XMStoreFloat4x4(&shadowMap.LightView, lightView);
		XMStoreFloat4x4(&shadowMap.LightProj, lightProj);
		XMStoreFloat4x4(&shadowMap.ShadowTransform, shadowTransform);
	}

	void UpdateSpotShadowTransform(ShadowMap& shadowMap)
	{
		XMVECTOR lightDir = XMLoadFloat3(&shadowMap.SceneLight->Direction);
		XMVECTOR lightPos = XMLoadFloat3(&shadowMap.SceneLight->Position);
		XMVECTOR targetPos = lightPos + lightDir;
		XMVECTOR lightUp = XMLoadFloat4(&DEFAULT_LIGHT_UP_VECTOR);
		XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

		XMStoreFloat3(&shadowMap.LightVirtualPosW, lightPos);

		shadowMap.LightNearZ = DEFAULT_LIGHT_NEAR_Z;
		shadowMap.LightFarZ = shadowMap.SceneLight->FalloffEnd;
		// TODO: Expose FovY and AspectRatio
		// TODO: Test making farz infinity
		XMMATRIX lightProj =  XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(800) / 600, shadowMap.LightNearZ, shadowMap.LightFarZ);


		XMMATRIX shadowTransform = lightView * lightProj * SHADOW_NDC_TO_TEX_COORD;
		XMStoreFloat4x4(&shadowMap.LightView, lightView);
		XMStoreFloat4x4(&shadowMap.LightProj, lightProj);
		XMStoreFloat4x4(&shadowMap.ShadowTransform, shadowTransform);
	}

	void UpdatePointShadowTransform(ShadowMap& shadowMap)
	{
		assert(false);
	}
}

void Spectral::Graphics::UpdateShadowTransform(ShadowMap& shadowMap)
{
	switch (shadowMap.LightType)
	{
	case LightType::Directional:
		UpdateDirectionalShadowTransform(shadowMap);
		break;
	case LightType::Spot:
		UpdateSpotShadowTransform(shadowMap);
		break;
	case LightType::Point:
		UpdatePointShadowTransform(shadowMap);
		break;
	}
}
