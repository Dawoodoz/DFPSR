
#ifndef DFPSR_SPRITE_ENGINE
#define DFPSR_SPRITE_ENGINE

#include "../../DFPSR/includeFramework.h"
#include "orthoAPI.h"
#include "lightAPI.h"
#include <cassert>
#include <limits>

namespace dsr {

// TODO: Make into a constructor for each vector type
inline FVector3D parseFVector3D(const ReadableString& content) {
	List<String> args = string_split(content, U',');
	if (args.length() != 3) {
		printText(U"Expected a vector of three decimal values.\n");
		return FVector3D();
	} else {
		return FVector3D(string_toDouble(args[0]), string_toDouble(args[1]), string_toDouble(args[2]));
	}
}

// A 2D image with depth and normal images for deferred light.
//   To be rendered into images in advance for maximum detail level.
struct SpriteInstance {
public:
	int32_t typeIndex;
	Direction direction;
	IVector3D location; // Mini-tile coordinates.
	bool shadowCasting;
	uint64_t userData; // Can be used to store additional information needed for specific games.
public:
	SpriteInstance(int32_t typeIndex, Direction direction, const IVector3D& location, bool shadowCasting, uint64_t userData = 0)
	: typeIndex(typeIndex), direction(direction), location(location), shadowCasting(shadowCasting), userData(userData) {}
};

struct DenseModelImpl;
using DenseModel = Handle<struct DenseModelImpl>;
DenseModel DenseModel_create(const Model& original);

// A 3D model that can be rotated freely.
//   To be rendered during game-play to allow free rotation.
struct ModelInstance {
public:
	int32_t typeIndex;
	Transform3D location; // 3D tile coordinates with translation and 3-axis rotation allowed.
	uint64_t userData; // Can be used to store additional information needed for specific games.
public:
	ModelInstance(int32_t typeIndex, const Transform3D& location, uint64_t userData = 0)
	: typeIndex(typeIndex), location(location), userData(userData) {}
};

class SpriteWorldImpl;
using SpriteWorld = Handle<SpriteWorldImpl>;

// Sprite types
int32_t spriteWorld_loadSpriteTypeFromFile(const String& folderPath, const String& spriteName);
int32_t spriteWorld_getSpriteTypeCount();
String spriteWorld_getSpriteTypeName(int32_t index);

// Model types
int32_t spriteWorld_loadModelTypeFromFile(const String& folderPath, const String& visibleModelName, const String& shadowModelName);
int32_t spriteWorld_getModelTypeCount();
String spriteWorld_getModelTypeName(int32_t index);

SpriteWorld spriteWorld_create(OrthoSystem ortho, int32_t shadowResolution);
void spriteWorld_addBackgroundSprite(SpriteWorld& world, const SpriteInstance& sprite);
void spriteWorld_addBackgroundModel(SpriteWorld& world, const ModelInstance& instance);
void spriteWorld_addTemporarySprite(SpriteWorld& world, const SpriteInstance& sprite);
void spriteWorld_addTemporaryModel(SpriteWorld& world, const ModelInstance& instance);

// SpriteInstance& sprite, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound -> bool selected
using SpriteSelection = StorableCallback<bool(SpriteInstance&, const IVector3D, const IVector3D, const IVector3D)>;
// Remove sprites using an axis aligned serach box in mini-tile coordinates and a lambda filter.
//   Use userData in the lambda's first argument to get ownership information.
//   Return true for each sprite to remove from the background.
void spriteWorld_removeBackgroundSprites(SpriteWorld& world, const IVector3D& searchMinBound, const IVector3D& searchMaxBound, const SpriteSelection& filter);
// Erasing every sprite within the bound.
void spriteWorld_removeBackgroundSprites(SpriteWorld& world, const IVector3D& searchMinBound, const IVector3D& searchMaxBound);

// ModelInstance& model, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound -> bool selected.
using ModelSelection = StorableCallback<bool(ModelInstance&, const IVector3D, const IVector3D, const IVector3D)>;
// Remove models using an axis aligned serach box in mini-tile coordinates and a lambda filter.
//   Use userData in the lambda's first argument to get ownership information.
//   Return true for each model to remove from the background.
void spriteWorld_removeBackgroundModels(SpriteWorld& world, const IVector3D& searchMinBound, const IVector3D& searchMaxBound, const ModelSelection& filter);
// Erasing every model within the bound.
void spriteWorld_removeBackgroundModels(SpriteWorld& world, const IVector3D& searchMinBound, const IVector3D& searchMaxBound);

// Create a point light that only exists until the next call to spriteWorld_clearTemporary.
//   position is in tile unit world-space.
void spriteWorld_createTemporary_pointLight(SpriteWorld& world, const FVector3D position, float radius, float intensity, ColorRgbI32 color, bool shadowCasting);
void spriteWorld_createTemporary_directedLight(SpriteWorld& world, const FVector3D direction, float intensity, ColorRgbI32 color);

void spriteWorld_clearTemporary(SpriteWorld& world);

// Draw the world using the current camera at the center of colorTarget.
void spriteWorld_draw(SpriteWorld& world, AlignedImageRgbaU8& colorTarget);

// Draw debug information.
void spriteWorld_debug_octrees(SpriteWorld& world, AlignedImageRgbaU8& colorTarget);

// The result is an approximation in mini-tile units.
//   The 3D system does not align with screen pixels for less than whole tile units.
IVector3D spriteWorld_findGroundAtPixel(SpriteWorld& world, const AlignedImageRgbaU8& colorBuffer, const IVector2D& pixelLocation);

// Approximates a mini-tile offset along the ground from the given pixel offset and moves the camera accordingly.
//   If the offset is too small, the camera might not move at all.
void spriteWorld_moveCameraInPixels(SpriteWorld& world, const IVector2D& pixelOffset);

// Get internal buffers after rendering.
//   Reading before having drawn the world for the first time will return null because the world does not yet know the target resolution.
//   By not being a part of rendering itself, it cannot go back in time and speed up rendering, so only use for debugging.
//     TODO: Make another feature for actually disabling dynamic light on low-end machines.
AlignedImageRgbaU8 spriteWorld_getDiffuseBuffer(SpriteWorld& world);
OrderedImageRgbaU8 spriteWorld_getNormalBuffer(SpriteWorld& world);
OrderedImageRgbaU8 spriteWorld_getLightBuffer(SpriteWorld& world);
AlignedImageF32 spriteWorld_getHeightBuffer(SpriteWorld& world);

// Camera's location as a 3D coordinate in world mini-tile coordinates.
IVector3D spriteWorld_getCameraLocation(const SpriteWorld& world);
void spriteWorld_setCameraLocation(SpriteWorld& world, const IVector3D miniTileLocation);

// Access the index of the camera's fixed direction.
//   This is not an index selecting the camera itself, only selecting the viewing angle.
// TODO: Implement bound checks or a system that's easier to understand.
int32_t spriteWorld_getCameraDirectionIndex(const SpriteWorld& world);
void spriteWorld_setCameraDirectionIndex(SpriteWorld& world, int32_t index);

// Get the current direction's orthogonal axis system.
//   This can be used to convert between world and screen coordinates using the camera location.
OrthoView& spriteWorld_getCurrentOrthoView(SpriteWorld& world);
// Get the whole game's orthogonal system for all camera angles.
OrthoSystem& spriteWorld_getOrthoSystem(SpriteWorld& world);

// Pre-conditions:
//   The model should be pre-transformed so that it can be rendered at the world origin.
//   Textures must be converted into vertex colors or else they will simply be ignored.
//   Enabling debug will save another file using a *Debug.png prefix with additional information.
//     Use it to find flaws in generated shadow shapes that are hard to see in raw data.
// TODO: Hide OrthoSystem or expose it safely.
void sprite_generateFromModel(const Model& visibleModel, const Model& shadowModel, const OrthoSystem& ortho, const String& targetPath, int32_t cameraAngles, bool debug = false);
// A simpler version writing the result to an image and a string instead of saving to files.
void sprite_generateFromModel(ImageRgbaU8& targetAtlas, String& targetConfigText, const Model& visibleModel, const Model& shadowModel, const OrthoSystem& ortho, const String& targetPath, int32_t cameraAngles);

}

#endif

