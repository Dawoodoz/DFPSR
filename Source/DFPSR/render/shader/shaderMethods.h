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

#ifndef DFPSR_RENDER_SHADER_METHODS
#define DFPSR_RENDER_SHADER_METHODS

#include <cstdint>
#include "../../math/FVector.h"
#include "../../math/scalar.h"
#include "../../base/simd3D.h"
#include "../../api/textureAPI.h"
#include "shaderTypes.h"
#include "../constants.h"

namespace dsr {

namespace shaderMethods {
	// Returns the linear interpolation of the values using corresponding weight ratios for A, B and C in 4 pixels at the same time.
	inline F32x4 interpolate(const FVector3D &vertexData, const F32x4x3 &vertexWeights) {
		F32x4 vMA = vertexData.x * vertexWeights.v1;
		F32x4 vMB = vertexData.y * vertexWeights.v2;
		F32x4 vMC = vertexData.z * vertexWeights.v3;
		return vMA + vMB + vMC;
	}

	inline Rgba_F32x4 interpolateVertexColor(const FVector3D &red, const FVector3D &green, const FVector3D &blue, const FVector3D &alpha, const F32x4x3 &vertexWeights) {
		return Rgba_F32x4(
		  interpolate(red,   vertexWeights),
		  interpolate(green, vertexWeights),
		  interpolate(blue,  vertexWeights),
		  interpolate(alpha, vertexWeights)
		);
	}

	// TODO: Implement sparse computation of floating-point mip levels in a grid, which can increase the density when getting closer to a horizon.
	// TODO: Let RgbaMultipy generate additional template instances, especially for SQUARE and MIP_INSIDE which are common.
	//       If the texture has at least 5 mip levels, MIP_INSIDE can be true.
	//       For the majority of textures that are square, SQUARE can be true.
	template<
	  Interpolation INTERPOLATION,
	  bool SQUARE = false,
	  bool SINGLE_LAYER = false,
	  bool XY_INSIDE = false,
	  bool MIP_INSIDE = false,
	  bool HIGHEST_RESOLUTION = false
	>
	inline U32x4 sample_U32(const TextureRgbaU8 *source, const F32x4 &u, const F32x4 &v) {
		if (INTERPOLATION == Interpolation::NN) {
			if (HIGHEST_RESOLUTION) {
				return texture_sample_nearest<SQUARE, SINGLE_LAYER, MIP_INSIDE, HIGHEST_RESOLUTION, U32x4, F32x4>(*source, u, v, U32x4(0u));
			} else {
				// TODO: Calculate MIP levels using a separate rendering stage with sparse resolution writing results into thread-local memory.
				uint32_t mipLevel = texture_getMipLevelIndex<F32x4>(*source, u, v);
				return texture_sample_nearest<SQUARE, SINGLE_LAYER, MIP_INSIDE, HIGHEST_RESOLUTION, U32x4, F32x4>(*source, u, v, U32x4(mipLevel));
			}
		} else {
			if (HIGHEST_RESOLUTION) {
				return texture_sample_bilinear<SQUARE, SINGLE_LAYER, MIP_INSIDE, HIGHEST_RESOLUTION, U32x4, U16x8, F32x4>(*source, u, v, U32x4(0u));
			} else {
				uint32_t mipLevel = texture_getMipLevelIndex<F32x4>(*source, u, v);
				return texture_sample_bilinear<SQUARE, SINGLE_LAYER, MIP_INSIDE, HIGHEST_RESOLUTION, U32x4, U16x8, F32x4>(*source, u, v, U32x4(mipLevel));
			}
		}
	}

	template<Interpolation INTERPOLATION,
	  bool SQUARE = false,
	  bool SINGLE_LAYER = false,
	  bool XY_INSIDE = false,
	  bool MIP_INSIDE = false,
	  bool HIGHEST_RESOLUTION = false
	>
	inline Rgba_F32<U32x4, F32x4> sample_F32(const TextureRgbaU8 *source, const F32x4 &u, const F32x4 &v) {
		return Rgba_F32<U32x4, F32x4>(sample_U32<INTERPOLATION, SQUARE, SINGLE_LAYER, XY_INSIDE, MIP_INSIDE, HIGHEST_RESOLUTION>(source, u, v));
	}
}

}

#endif
