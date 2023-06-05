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

#ifndef DFPSR_RENDER_SHADER_RGBA_MULTIPLY
#define DFPSR_RENDER_SHADER_RGBA_MULTIPLY

#include <stdio.h>
#include <stdint.h>
#include <cassert>
#include <algorithm>
#include "Shader.h"
#include "../../image/ImageRgbaU8.h"

namespace dsr {

template <bool HAS_DIFFUSE_MAP, bool HAS_LIGHT_MAP, bool HAS_VERTEX_FADING, bool COLORLESS, bool DISABLE_MIPMAP>
class Shader_RgbaMultiply : public Shader {
private:
	const TextureRgba *diffuseMap; // The full diffuseMap mipmap pyramid to use without DISABLE_MIPMAP
	const TextureRgbaLayer *diffuseLayer; // Layer 0 of diffuseMap to use with DISABLE_MIPMAP
	const TextureRgbaLayer *lightLayer;
	// Planar format with each vector representing the three triangle corners
	const TriangleTexCoords texCoords;
	const TriangleColors colors;
	// Normalize the color product by pre-multiplying the vertex colors
	float getVertexScale() {
		float result = 255.0f; // Scale from normalized to byte for the output
		if (HAS_DIFFUSE_MAP) {
			result /= 255.0f; // Normalize the diffuse map from 0..255 to 0..1 by dividing the vertex color
		}
		if (HAS_LIGHT_MAP) {
			result /= 255.0f; // Normalize the light map from 0..255 to 0..1 by dividing the vertex color
		}
		return result;
	}
	explicit Shader_RgbaMultiply(const TriangleInput &triangleInput) :
	  diffuseMap(triangleInput.diffuseImage ? &(triangleInput.diffuseImage->texture) : nullptr),
	  diffuseLayer(triangleInput.diffuseImage ? &(triangleInput.diffuseImage->texture.mips[0]) : nullptr),
	  lightLayer(triangleInput.lightImage ? &(triangleInput.lightImage->texture.mips[0]) : nullptr),
	  texCoords(triangleInput.texCoords.getPositive()), colors(triangleInput.colors.getScaled(getVertexScale())) {
		// Texture coordinates must be on the positive side to allow using truncation as a floor function
		if (HAS_DIFFUSE_MAP) {
			// Incorrect tests?
			if (DISABLE_MIPMAP) {
				assert(this->diffuseLayer != nullptr); // Cannot sample null
				assert(this->diffuseLayer->exists()); // Cannot sample regular images
			} else {
				assert(this->diffuseMap != nullptr); // Cannot sample null
				assert(this->diffuseMap->exists()); // Cannot sample regular images
			}
		}
		if (HAS_LIGHT_MAP) {
			assert(this->lightLayer != nullptr); // Cannot sample null
			assert(this->lightLayer->exists()); // Cannot sample regular images
		}
	}
public:
	// The process method to take a function pointer to.
	//    Must have the same signature as drawCallbackTemplate in Shader.h.
	static void processTriangle(const TriangleInput &triangleInput, ImageRgbaU8Impl *colorBuffer, ImageF32Impl *depthBuffer, const ITriangle2D &triangle, const Projection &projection, const RowShape &shape, Filter filter) {
		Shader_RgbaMultiply tempShader(triangleInput);
		tempShader.fillShape(colorBuffer, depthBuffer, triangle, projection, shape, filter);
	}
	Rgba_F32 getPixels_2x2(const F32x4x3 &vertexWeights) const override {
		if (HAS_DIFFUSE_MAP && !HAS_LIGHT_MAP && COLORLESS) {
			// Optimized for diffuse only
			F32x4 u1(shaderMethods::interpolate(this->texCoords.u1, vertexWeights));
			F32x4 v1(shaderMethods::interpolate(this->texCoords.v1, vertexWeights));
			if (DISABLE_MIPMAP) {
				return shaderMethods::sample_F32<Interpolation::BL, false>(this->diffuseLayer, u1, v1);
			} else {
				return shaderMethods::sample_F32<Interpolation::BL, false>(this->diffuseMap, u1, v1);
			}
		} else if (HAS_LIGHT_MAP && !HAS_DIFFUSE_MAP && COLORLESS) {
			// Optimized for light only
			F32x4 u2(shaderMethods::interpolate(this->texCoords.u2, vertexWeights));
			F32x4 v2(shaderMethods::interpolate(this->texCoords.v2, vertexWeights));
			return shaderMethods::sample_F32<Interpolation::BL, false>(this->lightLayer, u2, v2);
		} else {
			// Interpolate the vertex color
			Rgba_F32 color = HAS_VERTEX_FADING ?
			  shaderMethods::interpolateVertexColor(this->colors.red, this->colors.green, this->colors.blue, this->colors.alpha, vertexWeights) :
			  Rgba_F32(F32x4(this->colors.red.x), F32x4(this->colors.green.x), F32x4(this->colors.blue.x), F32x4(this->colors.alpha.x));
			// Sample diffuse
			if (HAS_DIFFUSE_MAP) {
				F32x4 u1(shaderMethods::interpolate(this->texCoords.u1, vertexWeights));
				F32x4 v1(shaderMethods::interpolate(this->texCoords.v1, vertexWeights));
				if (DISABLE_MIPMAP) {
					color = color * shaderMethods::sample_F32<Interpolation::BL, false>(this->diffuseLayer, u1, v1);
				} else {
					color = color * shaderMethods::sample_F32<Interpolation::BL, false>(this->diffuseMap, u1, v1);
				}
			}
			// Sample lightmap
			if (HAS_LIGHT_MAP) {
				F32x4 u2(shaderMethods::interpolate(this->texCoords.u2, vertexWeights));
				F32x4 v2(shaderMethods::interpolate(this->texCoords.v2, vertexWeights));
				color = color * shaderMethods::sample_F32<Interpolation::BL, false>(this->lightLayer, u2, v2);
			}
			return color;
		}
	}
};

}

#endif
