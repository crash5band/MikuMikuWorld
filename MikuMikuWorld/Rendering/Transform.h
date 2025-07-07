#pragma once
#include <array>
#include "DirectXMath.h"

namespace DirectX 
{
    inline bool XMMatrixIsNull(FXMMATRIX M) {
        XMVECTOR zero = XMVectorZero();
        return  XMVector4Equal(M.r[0], zero) &&
                XMVector4Equal(M.r[1], zero) &&
                XMVector4Equal(M.r[2], zero) &&
                XMVector4Equal(M.r[3], zero);
    }
}

namespace MikuMikuWorld
{
    class Transform
    {
        DirectX::XMMATRIX xx;
        DirectX::XMMATRIX xy;
        DirectX::XMMATRIX yx;
        DirectX::XMMATRIX yy;
        
        bool xyHasValue, yxHasValue;
        
        public:
        Transform(float v[64]);
        std::array<DirectX::XMFLOAT4, 4> apply(const std::array<DirectX::XMFLOAT4, 4>& vPos) const;
    };
}