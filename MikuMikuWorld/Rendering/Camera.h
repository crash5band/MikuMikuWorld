#pragma once
#include <DirectXMath.h>

namespace MikuMikuWorld
{
	class Camera
	{
	private:
		const DirectX::XMVECTOR up{ 0.0f, 1.0f, 0.0, 1.0f };

	public:
		float yaw, pitch, fov;
		DirectX::XMVECTOR position{ 0.0f, 0.0f, -1.0f, 1.0f };
		DirectX::XMVECTOR target{ 0.0f, 0.0f, 0.0f, -1.0f };
		DirectX::XMVECTOR front{ 0.0f, 0.0f, 0.0f, 1.0f };

		Camera();

		void positionCamNormal();
		void rotate(float x, float y);
		void zoom(float delta);
		DirectX::XMMATRIX getViewMatrix() const;
		DirectX::XMMATRIX getProjectionMatrix(float aspect, float near, float far) const;
		static DirectX::XMMATRIX getOrthographicProjection(float width, float height);
		static DirectX::XMMATRIX getOffCenterOrthographicProjection(float xmin, float xmax, float ymin, float ymax);
	};
}