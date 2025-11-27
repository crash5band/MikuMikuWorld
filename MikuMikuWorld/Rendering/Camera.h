#pragma once
#include <DirectXMath.h>

namespace MikuMikuWorld
{
	class Camera
	{
	private:
		DirectX::XMVECTOR position{ 0.0f, 0.0f, -1.0f, 1.0f };
		DirectX::XMVECTOR front{ 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMMATRIX viewMatrix;
		DirectX::XMMATRIX inverseViewMatrix;
		float yaw, pitch, fov;

	public:
		Camera();

		DirectX::XMVECTOR getPsotion() const { return position; }
		void setPosition(DirectX::XMVECTOR pos);

		DirectX::XMVECTOR getRotation() const { return {pitch, yaw, 0.f, 0.f}; }
		void setRotation(float yaw, float pitch);

		float getFov() const { return fov; }
		void setFov(float fov);

		void positionCamNormal();
		void rotate(float x, float y);
		void zoom(float delta);

		const DirectX::XMMATRIX& getViewMatrix() const;
		const DirectX::XMMATRIX& getInverseViewMatrix() const;
		DirectX::XMMATRIX getProjectionMatrix(float aspect, float near, float far) const;
		static DirectX::XMMATRIX getOrthographicProjection(float width, float height);
		static DirectX::XMMATRIX getOffCenterOrthographicProjection(float xmin, float xmax, float ymin, float ymax);
	};
}