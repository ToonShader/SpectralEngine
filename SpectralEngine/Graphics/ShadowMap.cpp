#include "ShadowMap.h"

using namespace Spectral::Graphics;

namespace
{
	void UpdateDirectionalShadowTransform(ShadowMap& shadowMap)
	{
		// Only the first "main" light casts a shadow.
		XMVECTOR lightDir = XMLoadFloat3(&shadowMap.SceneLight->Direction);
		XMVECTOR lightPos = (-2.0f * shadowMap.BoundingSphere.Radius * lightDir) + XMLoadFloat3(&shadowMap.BoundingSphere.Center);
		XMVECTOR targetPos = XMLoadFloat3(&shadowMap.BoundingSphere.Center);
		XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
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

		// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
		XMMATRIX T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);

		XMMATRIX S = lightView * lightProj*T;
		XMStoreFloat4x4(&shadowMap.LightView, lightView);
		XMStoreFloat4x4(&shadowMap.LightProj, lightProj);
		XMStoreFloat4x4(&shadowMap.ShadowTransform, S);
	}

	void UpdateSpotShadowTransform(ShadowMap& shadowMap)
	{
		assert(false);
	}

	void UpdatePointShadowTransform(ShadowMap& shadowMap)
	{
		assert(false);
	}
}

//ShadowMap::ShadowMap(LightType lightType, Light* sceneLight, const DirectX::BoundingSphere& boundingSphere, UINT width, UINT height)
//	: mSceneLight(sceneLight), mBoundingSphere(boundingSphere), mWidth(width), mHeight(height)
//{
//	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
//	mScissorRect = { 0, 0, (int)width, (int)height };
//}

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
