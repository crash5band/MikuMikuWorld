#pragma once
#include <DirectXMath.h>

namespace MikuMikuWorld
{
	class Camera
	{
	public:
		float yaw, pitch;
		DirectX::XMVECTOR position{ 0.0f, 0.0f, -1.0f, 1.0f };
		DirectX::XMVECTOR front{ 0.0f, 0.0f, 1.0f, 1.0f };
		DirectX::XMVECTOR right{ 1.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMVECTOR up{ 0.0f, 1.0f, 0.0, 1.0f };

		Camera();

		void setPositionY(float posY);
		void updateCameraVectors();
		
		DirectX::XMMATRIX getViewMatrix() const;
		DirectX::XMMATRIX getOrthographicProjection(float width, float height) const;
		DirectX::XMMATRIX getOffCenterOrthographicProjection(float left, float right, float up, float down) const;
		DirectX::XMMATRIX getPerspectiveProjection(float fov, float aspectRatio, float zNear, float zFar) const;
	};
}