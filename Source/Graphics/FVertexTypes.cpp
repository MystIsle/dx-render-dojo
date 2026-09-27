#include "Graphics/FVertexTypes.h"

#include <cstddef>

const std::array<D3D11_INPUT_ELEMENT_DESC, 2> FVertexPositionColor::InputElements = {{
    {"POSITION",
     0,
     DXGI_FORMAT_R32G32B32_FLOAT,
     0,
     offsetof(FVertexPositionColor, Position),
     D3D11_INPUT_PER_VERTEX_DATA,
     0},
    {"COLOR",
     0,
     DXGI_FORMAT_R32G32B32A32_FLOAT,
     0,
     offsetof(FVertexPositionColor, Color),
     D3D11_INPUT_PER_VERTEX_DATA,
     0},
}};
