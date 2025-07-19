#include "Camera.h"

namespace MikuMikuWorld
{
	Camera::Camera()
	{

	}

	void Camera::setPositionY(float posY)
	{
		DirectX::XMVectorSetY(position, posY);
	}

    DirectX::XMMATRIX Camera::getOrthographicProjection(float width, float height)
	{
		return DirectX::XMMatrixOrthographicRH(width, height, 0.001f, 100);
	}

	DirectX::XMMATRIX Camera::getOffCenterOrthographicProjection(float xmin, float xmax, float ymin, float ymax)
	{
		return DirectX::XMMatrixOrthographicOffCenterRH(xmin, xmax, ymin, ymax, 0.001f, 100.0f);
	}

	DirectX::XMMATRIX Camera::getViewMatrix() const
	{
		return DirectX::XMMatrixLookAtRH(position, target, up);
	}
}