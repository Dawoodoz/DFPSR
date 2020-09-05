
#ifndef DFPSR_DEFERRED_LIGHT_ENGINE
#define DFPSR_DEFERRED_LIGHT_ENGINE

#include <assert.h>
#include "../../../DFPSR/includeFramework.h"
#include "orthoAPI.h"

namespace dsr {

// Light-space is like camera-space, but with the Y-axis straight up in world-space instead of leaning forward.
//   Light space has its origin in the center of camera space.
//   The X axis goes to the right side in image-space.
//   The Z axis goes away from the camera and is aligned with Y = 0.
// Light sources are given in relative coordinates, but calculated in light-space.
//   Send positions as (world-space - cameraPosition).
//   Send directions in world-space.
//   Send normalToWorldSpace to define the rotation from light-space to world-space.
//     The filters will rotate the light sources from world-space to light-space using the inverse of normalToWorldSpace.
// screenDepthProjection defines the coordinate system being used for placing light.
//   OrthoSystem in the sprite API can automatically create a light projection system for camera space in whole tile units.
//   It's X axis defines how much the Y=0 position moves along an X pixel.
//   It's Y axis defines how much the Y=0 position moves along a Y pixel.
//   It's Z axis defines how much the final 3D position moves along a tile unit of depth based on the depth-buffers

// Deferred light operations
void setDirectedLight(const OrthoView& camera, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const FVector3D& lightDirection, float lightIntensity, const ColorRgbI32& lightColor);
void addDirectedLight(const OrthoView& camera, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const FVector3D& lightDirection, float lightIntensity, const ColorRgbI32& lightColor);
void addPointLight(const OrthoView& camera, const IVector2D& worldCenter, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const AlignedImageF32& heightBuffer, const FVector3D& lightPosition, float lightRadius, float lightIntensity, const ColorRgbI32& lightColor, const AlignedImageF32& shadowCubeMap);
void addPointLight(const OrthoView& camera, const IVector2D& worldCenter, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const AlignedImageF32& heightBuffer, const FVector3D& lightPosition, float lightRadius, float lightIntensity, const ColorRgbI32& lightColor);
void blendLight(AlignedImageRgbaU8& targetColor, const OrderedImageRgbaU8& diffuseBuffer, const OrderedImageRgbaU8& lightBuffer);

}

#endif

