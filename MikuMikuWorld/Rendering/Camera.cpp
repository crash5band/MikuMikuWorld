#include "Camera.h"
#include <algorithm>

namespace MikuMikuWorld
{
	Camera::Camera() : fov{ 45 }, yaw{ -90 }, pitch{ 0 }
	{
		viewMatrix = DirectX::XMMatrixIdentity();
		inverseViewMatrix = DirectX::XMMatrixIdentity();
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

		DirectX::XMVECTOR _front {
			cos(rYaw) * cos(rPitch),
			-sin(rPitch),
			-sin(rYaw) * cos(rPitch),
			1.0f
		};

		front = DirectX::XMVector3Normalize(_front);

		DirectX::XMVECTOR tgt = front;
		tgt = DirectX::XMVectorAdd(tgt, position);

		viewMatrix = DirectX::XMMatrixLookAtRH(position, tgt, DirectX::XMVECTOR{ 0.0f, -1.0f, 0.0f, 0.0f });
		inverseViewMatrix = DirectX::XMMatrixIdentity();
		inverseViewMatrix *= DirectX::XMMatrixInverse(nullptr, viewMatrix);
		inverseViewMatrix.r[3] = DirectX::XMVECTOR{ 0, 0, 0, 1 };
	}

	void Camera::setPosition(DirectX::XMVECTOR pos)
	{
		position = pos;
	}

	void Camera::setRotation(float yaw, float pitch)
	{
		this->yaw = yaw;
		this->pitch = std::clamp(pitch, -89.0f, 89.0f);
	}

	void Camera::setFov(float fov)
	{
		this->fov = fov;
	}

	void Camera::rotate(float x, float y)
	{
		yaw += x * -0.1f;
		pitch += y * 0.1f;
		pitch = std::clamp(pitch, -89.0f, 89.0f);
	}

	void Camera::zoom(float delta)
	{
		position = DirectX::XMVectorAdd(position, DirectX::XMVectorScale(front, delta));
	}

	const DirectX::XMMATRIX& Camera::getViewMatrix() const
	{
		return viewMatrix;
	}

	const DirectX::XMMATRIX& Camera::getInverseViewMatrix() const
	{
		return inverseViewMatrix;
	}

	DirectX::XMMATRIX Camera::getProjectionMatrix(float aspect, float near, float far) const
	{
		return DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(fov), aspect, near, far);
	}
}