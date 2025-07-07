#include "Transform.h"

namespace MikuMikuWorld
{
    Transform::Transform(float v[64]) : xx(v), xy(v + 16), yx(v + 32), yy(v + 48) 
    {
        xyHasValue = !DirectX::XMMatrixIsNull(xy);
        yxHasValue = !DirectX::XMMatrixIsNull(yx);   
    }

    std::array<DirectX::XMFLOAT4, 4> Transform::apply(const std::array<DirectX::XMFLOAT4, 4> &vPos) const
    {
        std::array<DirectX::XMFLOAT4, 4> transformed;
        
        DirectX::XMVECTOR x = DirectX::XMVectorSet(vPos[0].x, vPos[1].x, vPos[2].x, vPos[3].x);
        DirectX::XMVECTOR y = DirectX::XMVectorSet(vPos[0].y, vPos[1].y, vPos[2].y, vPos[3].y);
        DirectX::XMVECTOR
            tx = !xyHasValue ? DirectX::XMVector4Transform(x, xx) : DirectX::XMVectorAdd(DirectX::XMVector4Transform(x, xx), DirectX::XMVector4Transform(x, xy)),
            ty = !yxHasValue ? DirectX::XMVector4Transform(y, yy) : DirectX::XMVectorAdd(DirectX::XMVector4Transform(y, yx), DirectX::XMVector4Transform(y, yy));
        return {{
            { DirectX::XMVectorGetX(tx), DirectX::XMVectorGetX(ty), vPos[0].z, vPos[0].z },
            { DirectX::XMVectorGetY(tx), DirectX::XMVectorGetY(ty), vPos[1].z, vPos[1].z },
            { DirectX::XMVectorGetZ(tx), DirectX::XMVectorGetZ(ty), vPos[2].z, vPos[2].z },
            { DirectX::XMVectorGetW(tx), DirectX::XMVectorGetW(ty), vPos[3].z, vPos[3].z }
        }};
    }
}