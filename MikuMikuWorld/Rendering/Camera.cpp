#include "Camera.h"

namespace MikuMikuWorld
{
	Camera::Camera() : yaw{ 0 }, pitch{ 0 }
	{

	}

	void Camera::setPositionY(float posY)
	{
		position.m128_f32[1] = posY;
	}

	DirectX::XMMATRIX Camera::getOrthographicProjection(float width, float height) const
	{
		return DirectX::XMMatrixOrthographicRH(width, height, 0.001f, 100);
	}

	DirectX::XMMATRIX Camera::getOffCenterOrthographicProjection(float left, float right, float up, float down) const
	{
		return DirectX::XMMatrixOrthographicOffCenterRH(left, right, down, up, 0.001f, 100.0f);
	}

	DirectX::XMMATRIX Camera::getPerspectiveProjection(float fov, float aspectRatio, float zNear, float zFar) const
	{
		return DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(fov), aspectRatio, zNear, zFar);
	}

	DirectX::XMMATRIX Camera::getViewMatrix() const
	{
		return DirectX::XMMatrixLookAtLH(position, front, DirectX::XMVECTOR{ 0.0f, 1.0f, 0.0f, 1.0f });
	}

	void Camera::updateCameraVectors()
	{
		DirectX::XMVECTOR _front;
		_front.m128_f32[0] = cosf(DirectX::XMConvertToRadians(yaw)) * cosf(DirectX::XMConvertToRadians(pitch));
		_front.m128_f32[1] = sinf(DirectX::XMConvertToRadians(pitch));
		_front.m128_f32[2] = sinf(DirectX::XMConvertToRadians(yaw)) * cosf(DirectX::XMConvertToRadians(pitch));

		front = DirectX::XMVector3Normalize(_front);
		right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(front, DirectX::XMVECTOR{ 0.0f, 1.0f, 0.0f, 1.0f }));
		up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(right, front));
	}
}