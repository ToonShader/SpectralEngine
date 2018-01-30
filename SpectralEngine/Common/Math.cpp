#include "Math.h"

using namespace Spectral;
using namespace Math;

XMFLOAT4X4 Math::XMF4x4Identity()
{
	return { 1.0f, 0.0f, 0.0f, 0.0f,
			 0.0f, 1.0f, 0.0f, 0.0f,
			 0.0f, 0.0f, 1.0f, 0.0f,
			 0.0f, 0.0f, 0.0f, 1.0f };
}
