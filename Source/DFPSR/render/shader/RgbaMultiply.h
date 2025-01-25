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
#include "../../image/Image.h"

namespace dsr {

struct RgbaMultiply_data {
	const TextureRgbaU8 *diffuseMap; // Mip-mapping is allowed for diffuse textures.
	const TextureRgbaU8 *lightMap; // Mip-mapping is not allowed for lightmaps, because it would increase the number of shaders to compile and still look worse.
	// Planar format with each vector representing the three triangle corners
	const TriangleTexCoords texCoords;
	const TriangleColors colors;
	// Normalize the color product by pre-multiplying the vertex colors
	float getVertexScale() {
		float result = 255.0f; // Scale from normalized to byte for the output
		if (texture_exists(*(this->diffuseMap))) {
			result *= 1.0f / 255.0f; // Normalize the diffuse map from 0..255 to 0..1 by dividing the vertex color
		}
		if (texture_exists(*(this->lightMap))) {
			result *= 1.0f / 255.0f; // Normalize the light map from 0..255 to 0..1 by dividing the vertex color
		}
		return result;
	}
	explicit RgbaMultiply_data(const TriangleInput &triangleInput) :
	  diffuseMap(triangleInput.diffuseMap),
	  lightMap(triangleInput.lightMap),
	  texCoords(triangleInput.texCoords), colors(triangleInput.colors.getScaled(getVertexScale())) {}
};

// TODO: Simplify by merging boolean flags into named states.
//       A texture can be:
//         Non-existing
//         Of a single layer
//         Of multiple layers
//         Of enough layers to avoid clamping the mip index
//       A color can be:
//         White
//         Constant
//         Faded
// TODO: Skip converting to and from float colors when only sampling one texture or color.
// TODO: Because colors are converted to 16-bit channels during multiplication, the shader's return value might as well use a 16-bit color format that is faster to multiply and shift.
//       8 low bits for visible light, and 8 high bits for spill from multiplications before shifting results 8 bits to the right.
//       High intensity vertex colors multiplied at the end can use the high range for 10-bit image formats.
template <bool HAS_DIFFUSE_MAP, bool DIFFUSE_SINGLE_LAYER, bool HAS_LIGHT_MAP, bool HAS_VERTEX_FADING, bool COLORLESS>
inline Rgba_F32<U32x4, F32x4> getPixels_2x2(void *data, const F32x4x3 &vertexWeights) {
	RgbaMultiply_data *assets = (RgbaMultiply_data*)data;
	if (HAS_DIFFUSE_MAP && !HAS_LIGHT_MAP && COLORLESS) {
		// Optimized for diffuse only
		F32x4 u1 = shaderMethods::interpolate(assets->texCoords.u1, vertexWeights);
		F32x4 v1 = shaderMethods::interpolate(assets->texCoords.v1, vertexWeights);
		return shaderMethods::sample_F32<Interpolation::BL, false, DIFFUSE_SINGLE_LAYER, false, false, false>(assets->diffuseMap, u1, v1);
	} else if (HAS_LIGHT_MAP && !HAS_DIFFUSE_MAP && COLORLESS) {
		// Optimized for light only
		F32x4 u2 = shaderMethods::interpolate(assets->texCoords.u2, vertexWeights);
		F32x4 v2 = shaderMethods::interpolate(assets->texCoords.v2, vertexWeights);
		return shaderMethods::sample_F32<Interpolation::BL, false, false, false, false, true>(assets->lightMap, u2, v2);
	} else {
		// Interpolate the vertex color
		Rgba_F32<U32x4, F32x4> color = HAS_VERTEX_FADING ?
		  shaderMethods::interpolateVertexColor(assets->colors.red, assets->colors.green, assets->colors.blue, assets->colors.alpha, vertexWeights) :
		  Rgba_F32<U32x4, F32x4>(F32x4(assets->colors.red.x), F32x4(assets->colors.green.x), F32x4(assets->colors.blue.x), F32x4(assets->colors.alpha.x));
		// Sample diffuse
		if (HAS_DIFFUSE_MAP) {
			F32x4 u1 = shaderMethods::interpolate(assets->texCoords.u1, vertexWeights);
			F32x4 v1 = shaderMethods::interpolate(assets->texCoords.v1, vertexWeights);
			color = color * shaderMethods::sample_F32<Interpolation::BL, false, DIFFUSE_SINGLE_LAYER, false, false, false>(assets->diffuseMap, u1, v1);
		}
		// Sample lightmap
		if (HAS_LIGHT_MAP) {
			F32x4 u2 = shaderMethods::interpolate(assets->texCoords.u2, vertexWeights);
			F32x4 v2 = shaderMethods::interpolate(assets->texCoords.v2, vertexWeights);
			color = color * shaderMethods::sample_F32<Interpolation::BL, false, false, false, false, true>(assets->lightMap, u2, v2);
		}
		return color;
	}
}

// The process method to take a function pointer to.
//    Must have the same signature as drawCallbackTemplate in Shader.h.
static void processTriangle_RgbaMultiply(const TriangleInput &triangleInput, ImageRgbaU8 *colorBuffer, ImageF32 *depthBuffer, const ITriangle2D &triangle, const Projection &projection, const RowShape &shape, Filter filter) {
	// The pointers to textures may not be null, but can point to empty textures.
	RgbaMultiply_data data = RgbaMultiply_data(triangleInput);
	bool hasVertexFade = !(almostSame(data.colors.red) && almostSame(data.colors.green) && almostSame(data.colors.blue) && almostSame(data.colors.alpha));
	bool colorless = almostOne(data.colors.red) && almostOne(data.colors.green) && almostOne(data.colors.blue) && almostOne(data.colors.alpha);
	// TODO: Should non-existing textures use null pointers in the data, or pointers to empty textures?
	if (texture_exists(*(data.diffuseMap))) {
		bool hasDiffusePyramid = texture_hasPyramid(*(data.diffuseMap));
		// TODO: Avoid generating mip levels for the lightmap texture instead of hard-coding it to no mip levels.
		if (texture_exists(*(data.lightMap))) {
			if (hasVertexFade) { // DiffuseLightVertex
				if (hasDiffusePyramid) { // With mipmap
					fillShape(&data, getPixels_2x2<true, false, true, true, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				} else { // Without mipmap
					fillShape(&data, getPixels_2x2<true, true, true, true, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				}
			} else { // DiffuseLight
				if (hasDiffusePyramid) { // With mipmap
					fillShape(&data, getPixels_2x2<true, false, true, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				} else { // Without mipmap
					fillShape(&data, getPixels_2x2<true, true, true, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				}
			}
		} else {
			if (hasVertexFade) { // DiffuseVertex
				if (hasDiffusePyramid) { // With mipmap
					fillShape(&data, getPixels_2x2<false, false, false, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				} else { // Without mipmap
					fillShape(&data, getPixels_2x2<true, true, false, true, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				}
			} else {
				if (colorless) { // Diffuse without normalization
					if (hasDiffusePyramid) { // With mipmap
						fillShape(&data, getPixels_2x2<true, false, false, false, true>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
					} else { // Without mipmap
					fillShape(&data, getPixels_2x2<true, true, false, false, true>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
					}
				} else { // Diffuse
					if (hasDiffusePyramid) { // With mipmap
						fillShape(&data, getPixels_2x2<true, false, false, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
					} else { // Without mipmap
						fillShape(&data, getPixels_2x2<true, true, false, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
					}
				}
			}
		}
	} else {
		if (texture_exists(*(data.lightMap))) {
			if (hasVertexFade) { // LightVertex
				fillShape(&data, getPixels_2x2<false, false, true, true, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
			} else {
				if (colorless) { // Light without normalization
					fillShape(&data, getPixels_2x2<false, false, true, false, true>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				} else { // Light
					fillShape(&data, getPixels_2x2<false, false, true, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
				}
			}
		} else {
			if (hasVertexFade) { // Vertex
				fillShape(&data, getPixels_2x2<false, false, false, true, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
			} else { // Single color
				fillShape(&data, getPixels_2x2<false, false, false, false, false>, colorBuffer, depthBuffer, triangle, projection, shape, filter);
			}
		}
	}
}

}

#endif
