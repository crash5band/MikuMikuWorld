#include "Camera.h"

namespace MikuMikuWorld
{
	Camera::Camera() : fov{ 45 }
	{

	}

	DirectX::XMMATRIX Camera::getOrthographicProjection(float width, float height)
	{
		return DirectX::XMMatrixOrthographicRH(width, height, 0.001f, 100);
	}

	DirectX::XMMATRIX Camera::getOffCenterOrthographicProjection(float xmin, float xmax, float ymin, float ymax)
	{
		return DirectX::XMMatrixOrthographicOffCenterRH(xmin, xmax, ymin, ymax, 0.001f, 100.0f);
	}

	void Camera::positionCamNormal()
	{
		float rYaw = DirectX::XMConvertToRadians(yaw);
		float rPitch = DirectX::XMConvertToRadians(pitch);

		DirectX::XMVECTOR _front = DirectX::XMVectorSet(
			cos(rYaw) * cos(rPitch),
			-sin(rPitch),
			-sin(rYaw) * cos(rPitch),
			1
		);
		front = DirectX::XMVector3Normalize(_front);
	}

	DirectX::XMMATRIX Camera::getViewMatrix() const
	{
		DirectX::XMVECTOR tgt = front;
		tgt = DirectX::XMVectorAdd(tgt, position);

		return DirectX::XMMatrixLookAtRH(position, tgt, DirectX::XMVECTOR{ 0.0f, -1.0f, 0.0f, 1.0f });
	}

	DirectX::XMMATRIX Camera::getProjectionMatrix(float aspect, float near, float far) const
	{
		return DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(fov), aspect, near, far);
	}
}