#pragma once
#include <DirectXMath.h>

namespace MikuMikuWorld
{
	class Camera
	{
	  private:
		float yaw, pitch;
		DirectX::XMVECTOR position{ 0.0f, 0.0f, -1.0f, 1.0f };
		DirectX::XMVECTOR target{ 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMVECTOR front{ 0.0f, 0.0f, 0.0f, 1.0f };
		const DirectX::XMVECTOR up{ 0.0f, 1.0f, 0.0, 1.0f };

	  public:
		Camera();

		void setPositionY(float posY);

		DirectX::XMMATRIX getViewMatrix() const;
		DirectX::XMMATRIX getOrthographicProjection(float width, float height) const;
		DirectX::XMMATRIX getOffCenterOrthographicProjection(float left, float right, float up, float down) const;
		DirectX::XMMATRIX getPerspectiveProjection() const;
	};
}