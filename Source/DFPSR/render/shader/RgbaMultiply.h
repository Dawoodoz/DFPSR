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

#ifndef DFPSR_RENDER_SHADER_RGBA_MULTIPLY
#define DFPSR_RENDER_SHADER_RGBA_MULTIPLY

#include <cstdio>
#include <cstdint>
#include <cassert>
#include <algorithm>
#include "Shader.h"
#include "fillerTemplates.h"
#include "../../image/ImageRgbaU8.h"

namespace dsr {

struct RgbaMultiply_data {
	const TextureRgba *diffuseMap; // Mip-mapping is allowed for diffuse textures.
	const TextureRgba *lightMap; // Mip-mapping is not allowed for lightmaps, because it would increase the number of shaders to compile and still look worse.
	// Planar format with each vector representing the three triangle corners
	const TriangleTexCoords texCoords;
	const TriangleColors colors;
	// Normalize the color product by pre-multiplying the vertex colors
	float getVertexScale() {
		float result = 255.0f; // Scale from normalized to byte for the output
		if (this->diffuseMap) {
			result *= 1.0f / 255.0f; // Normalize the diffuse map from 0..255 to 0..1 by dividing the vertex color
		}
		if (this->lightMap) {
			result *= 1.0f / 255.0f; // Normalize the light map from 0..255 to 0..1 by dividing the vertex color
		}
		return result;
	}
	explicit RgbaMultiply_data(const TriangleInput &triangleInput) :
	  diffuseMap(triangleInput.diffuseImage ? &(triangleInput.diffuseImage->texture) : nullptr),
	  lightMap(triangleInput.lightImage ? &(triangleInput.lightImage->texture) : nullptr),
	  texCoords(triangleInput.texCoords), colors(triangleInput.colors.getScaled(getVertexScale())) {
		// Texture coordinates must be on the positive side to allow using truncation as a floor function
		if (this->diffuseMap) {
			assert(this->diffuseMap != nullptr); // Cannot sample null
			assert(this->diffuseMap->exists()); // Cannot sample regular images
		}
		if (this->lightMap) {
			assert(this->lightMap != nullptr); // Cannot sample null
			assert(this->lightMap->exists()); // Cannot sample regular images
		}
	}
};

template <bool HAS_DIFFUSE_MAP, bool HAS_LIGHT_MAP, bool HAS_VERTEX_FADING, bool COLORLESS, bool DISABLE_MIPMAP>
static Rgba_F32 getPixels_2x2(void *data, const F32x4x3 &vertexWeights) {
	if (HAS_DIFFUSE_MAP && !HAS_LIGHT_MAP && COLORLESS) {
		// Optimized for diffuse only
		F32x4 u1 = shaderMethods::interpolate(((RgbaMultiply_data*)data)->texCoords.u1, vertexWeights);
		F32x4 v1 = shaderMethods::interpolate(((RgbaMultiply_data*)data)->texCoords.v1, vertexWeights);
		return shaderMethods::sample_F32<Interpolation::BL, DISABLE_MIPMAP, false>(((RgbaMultiply_data*)data)->diffuseMap, u1, v1);
	} else if (HAS_LIGHT_MAP && !HAS_DIFFUSE_MAP && COLORLESS) {
		// Optimized for light only
		F32x4 u2 = shaderMethods::interpolate(((RgbaMultiply_data*)data)->texCoords.u2, vertexWeights);
		F32x4 v2 = shaderMethods::interpolate(((RgbaMultiply_data*)data)->texCoords.v2, vertexWeights);
		return shaderMethods::sample_F32<Interpolation::BL, true, false>(((RgbaMultiply_data*)data)->lightMap, u2, v2);
	} else {
		// Interpolate the vertex color
		Rgba_F32 color = HAS_VERTEX_FADING ?
		  shaderMethods::interpolateVertexColor(((RgbaMultiply_data*)data)->colors.red, ((RgbaMultiply_data*)data)->colors.green, ((RgbaMultiply_data*)data)->colors.blue, ((RgbaMultiply_data*)data)->colors.alpha, vertexWeights) :
		  Rgba_F32(F32x4(((RgbaMultiply_data*)data)->colors.red.x), F32x4(((RgbaMultiply_data*)data)->colors.green.x), F32x4(((RgbaMultiply_data*)data)->colors.blue.x), F32x4(((RgbaMultiply_data*)data)->colors.alpha.x));
		// Sample diffuse
		if (HAS_DIFFUSE_MAP) {
			F32x4 u1 = shaderMethods::interpolate(((RgbaMultiply_data*)data)->texCoords.u1, vertexWeights);
			F32x4 v1 = shaderMethods::interpolate(((RgbaMultiply_data*)data)->texCoords.v1, vertexWeights);
			color = color * shaderMethods::sample_F32<Interpolation::BL, DISABLE_MIPMAP, false>(((RgbaMultiply_data*)data)->diffuseMap, u1, v1);
		}
		// Sample lightmap
		if (HAS_LIGHT_MAP) {
			F32x4 u2 = shaderMethods::interpolate(((RgbaMultiply_data*)data)->texCoords.u2, vertexWeights);
			F32x4 v2 = shaderMethods::interpolate(((RgbaMultiply_data*)data)->texCoords.v2, vertexWeights);
			color = color * shaderMethods::sample_F32<Interpolation::BL, true, false>(((RgbaMultiply_data*)data)->lightMap, u2, v2);
		}
		return color;
	}
}

// The process method to take a function pointer to.
//    Must have the same signature as drawCallbackTemplate in Shader.h.
static void processTriangle_RgbaMultiply(const TriangleInput &triangleInput, ImageRgbaU8Impl *colorBuffer, ImageF32Impl *depthBuffer, const ITriangle2D &triangle, const Projection &projection, const RowShape &shape, Filter filter) {
	RgbaMultiply_data data = RgbaMultiply_data(triangleInput);
	bool hasVertexFade = !(almostSame(data.colors.red) && almostSame(data.colors.green) && almostSame(data.colors.blue) && almostSame(data.colors.alpha));
	bool colorless = almostOne(data.colors.red) && almostOne(data.colors.green) && almostOne(data.colors.blue) && almostOne(data.colors.alpha);
	if (data.diffuseMap) {
		bool hasDiffusePyramid = data.diffuseMap->hasMipBuffer();
		if (data.lightMap) {
			if (hasVertexFade) { // DiffuseLightVertex
				if (hasDiffusePyramid) { // With mipmap
					fillShape(&data, getPixels_2x2<true, true, true, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				} else { // Without mipmap
					fillShape(&data, getPixels_2x2<true, true, true, false, true>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				}
			} else { // DiffuseLight
				if (hasDiffusePyramid) { // With mipmap
					fillShape(&data, getPixels_2x2<true, true, false, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				} else { // Without mipmap
					fillShape(&data, getPixels_2x2<true, true, false, false, true>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				}
			}
		} else {
			if (hasVertexFade) { // DiffuseVertex
				if (hasDiffusePyramid) { // With mipmap
					fillShape(&data, getPixels_2x2<false, false, false, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				} else { // Without mipmap
					fillShape(&data, getPixels_2x2<true, false, true, false, true>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				}
			} else {
				if (colorless) { // Diffuse without normalization
					if (hasDiffusePyramid) { // With mipmap
						fillShape(&data, getPixels_2x2<true, false, false, true, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
					} else { // Without mipmap
					fillShape(&data, getPixels_2x2<true, false, false, true, true>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
					}
				} else { // Diffuse
					if (hasDiffusePyramid) { // With mipmap
						fillShape(&data, getPixels_2x2<true, false, false, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
					} else { // Without mipmap
						fillShape(&data, getPixels_2x2<true, false, false, false, true>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
					}
				}
			}
		}
	} else {
		if (data.lightMap) {
			if (hasVertexFade) { // LightVertex
				fillShape(&data, getPixels_2x2<false, true, true, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
			} else {
				if (colorless) { // Light without normalization
					fillShape(&data, getPixels_2x2<false, true, false, true, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				} else { // Light
					fillShape(&data, getPixels_2x2<false, true, false, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				}
			}
		} else {
			if (hasVertexFade) { // Vertex
				fillShape(&data, getPixels_2x2<false, false, true, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
			} else { // Single color
				fillShape(&data, getPixels_2x2<false, false, false, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
			}
		}
	}
}

}

#endif
