#pragma once

#include <d3d12.h>
#include "DepthStencilBuffer.h"
#include "Lighting.h"

namespace Spectral
{
	namespace Graphics
	{
		struct ShadowMap
		{
			ShadowMap() = default;
			// TODO: Resolve
			//ShadowMap(const ShadowMap& rhs) = delete;

			// TODO: Should these be a reference???
			// TODO: Should this light be required to be a submitted light?
			// TODO: This shouldn't even be a light
			Light* SceneLight = nullptr;
			LightType LightType = LightType::Directional;
			DirectX::BoundingSphere BoundingSphere;

			D3D12_VIEWPORT Viewport;
			D3D12_RECT ScissorRect;

			XMFLOAT3 LightVirtualPosW = XMFLOAT3(0.0f, 0.0f, 0.0f);
			float LightNearZ = 0.0f;
			float LightFarZ = 0.0f;
			// TODO: Where to store these for general case shadow maps/lights? I think maps and lights should be independent. Perhaps ShadowCaster? Perhaps this is a View/Frustum class?
			XMFLOAT4X4 LightView = Spectral::Math::XMF4x4Identity(); 
			XMFLOAT4X4 LightProj = Spectral::Math::XMF4x4Identity();
			XMFLOAT4X4 ShadowTransform = Spectral::Math::XMF4x4Identity();

		private:
			// TODO: Could reverse the relationship and then this could be kept internal to the renderer
			friend class GraphicsCore;
			DepthStencilBuffer* _DepthStencilBuffer = nullptr;
		};

		void UpdateShadowTransform(ShadowMap& shadowMap);
	}
}


