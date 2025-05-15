// zlib open source license
//
// Copyright (c) 2017 to 2023 David Forsgren Piuva
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.

#ifndef DFPSR_RENDER_RENDERING
#define DFPSR_RENDER_RENDERING

#include <cstdint>
#include "../../image/PackOrder.h"
#include "../../image/Image.h"
#include "../../image/Texture.h"
#include "../ITriangle2D.h"
#include "shaderMethods.h"
#include "shaderTypes.h"

namespace dsr {

struct TriangleTexCoords {
	FVector3D u1, v1, u2, v2;
	TriangleTexCoords() {}
	TriangleTexCoords(const FVector3D &u1, const FVector3D &v1, const FVector3D &u2, const FVector3D &v2) :
	  u1(u1), v1(v1), u2(u2), v2(v2) {}
	TriangleTexCoords(const FVector4D &a, const FVector4D &b, const FVector4D &c) :
	  u1(FVector3D(a.x, b.x, c.x)), v1(FVector3D(a.y, b.y, c.y)), u2(FVector3D(a.z, b.z, c.z)), v2(FVector3D(a.w, b.w, c.w)) {}
};

struct TriangleColors {
	FVector3D red, green, blue, alpha;
	TriangleColors() {}
	explicit TriangleColors(float monochrome) : red(monochrome), green(monochrome), blue(monochrome), alpha(monochrome) {}
	TriangleColors(const FVector3D &red, const FVector3D &green, const FVector3D &blue, const FVector3D &alpha) : red(red), green(green), blue(blue), alpha(alpha) {}
	TriangleColors(const FVector4D &a, const FVector4D &b, const FVector4D &c) :
	  red(FVector3D(a.x, b.x, c.x)), green(FVector3D(a.y, b.y, c.y)), blue(FVector3D(a.z, b.z, c.z)), alpha(FVector3D(a.w, b.w, c.w)) {}
	TriangleColors getScaled(float scalar) const {
		return TriangleColors(this->red * scalar, this->green * scalar, this->blue * scalar, this->alpha * scalar);
	}
};

struct TriangleInput {
	const TextureRgbaU8 diffuseMap;
	const TextureRgbaU8 lightMap;
	const TriangleTexCoords texCoords;
	const TriangleColors colors;
	TriangleInput(const TextureRgbaU8 &diffuseMap, const TextureRgbaU8 &lightMap, const TriangleTexCoords &texCoords, const TriangleColors &colors)
	: diffuseMap(diffuseMap), lightMap(lightMap), texCoords(texCoords), colors(colors) {}
};

// The template for function pointers doing the work
inline void drawCallbackTemplate(const TriangleInput &triangleInput, const ImageRgbaU8 &colorBuffer, const ImageF32 &depthBuffer, const ITriangle2D &triangle, const Projection &projection, const RowShape &shape, Filter filter) {}
using DRAW_CALLBACK_TYPE = decltype(&drawCallbackTemplate);

}

#endif

