// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
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

#include <stdint.h>
#include "../../image/PackOrder.h"
#include "../../image/ImageRgbaU8.h"
#include "../../image/ImageF32.h"
#include "../ITriangle2D.h"
#include "shaderMethods.h"
#include "shaderTypes.h"

namespace dsr {

inline float getMinimum(const FVector3D &coordinates) {
	float result = coordinates.x;
	if (coordinates.y < result) { result = coordinates.y; }
	if (coordinates.z < result) { result = coordinates.z; }
	return result;
}

struct TriangleTexCoords {
	FVector3D u1, v1, u2, v2;
	TriangleTexCoords() {}
	TriangleTexCoords(const FVector3D &u1, const FVector3D &v1, const FVector3D &u2, const FVector3D &v2) :
	  u1(u1), v1(v1), u2(u2), v2(v2) {}
	TriangleTexCoords(const FVector4D &a, const FVector4D &b, const FVector4D &c) :
	  u1(FVector3D(a.x, b.x, c.x)), v1(FVector3D(a.y, b.y, c.y)), u2(FVector3D(a.z, b.z, c.z)), v2(FVector3D(a.w, b.w, c.w)) {}
	TriangleTexCoords getPositive() const {
		return TriangleTexCoords(
		  this->u1 + FVector3D(1 - (int)getMinimum(this->u1)),
		  this->v1 + FVector3D(1 - (int)getMinimum(this->v1)),
		  this->u2 + FVector3D(1 - (int)getMinimum(this->u2)),
		  this->v2 + FVector3D(1 - (int)getMinimum(this->v2))
		);
	}
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
	const ImageRgbaU8Impl *diffuseImage;
	const ImageRgbaU8Impl *lightImage;
	const TriangleTexCoords texCoords;
	const TriangleColors colors;
	TriangleInput(const ImageRgbaU8Impl *diffuseImage, const ImageRgbaU8Impl *lightImage, const TriangleTexCoords &texCoords, const TriangleColors &colors)
	: diffuseImage(diffuseImage), lightImage(lightImage), texCoords(texCoords), colors(colors) {}
};

// The template for function pointers doing the work
inline void drawCallbackTemplate(const TriangleInput &triangleInput, ImageRgbaU8Impl *colorBuffer, ImageF32Impl *depthBuffer, const ITriangle2D &triangle, const Projection &projection, const RowShape &shape, Filter filter) {}
#define DRAW_CALLBACK_TYPE decltype(&drawCallbackTemplate)

// Inherit this class for pixel shaders
class Shader {
public:
	void fillShape(ImageRgbaU8Impl *colorBuffer, ImageF32Impl *depthBuffer, const ITriangle2D &triangle, const Projection &projection, const RowShape &shape, Filter filter);
	// The main call that defines the pixel shader
	virtual Rgba_F32 getPixels_2x2(const F32x4x3 &vertexWeights) const = 0;
};

}

#endif

