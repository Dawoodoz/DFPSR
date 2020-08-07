
#ifndef DFPSR_SPRITE_ENGINE
#define DFPSR_SPRITE_ENGINE

#include "../../../DFPSR/includeFramework.h"
#include "orthoAPI.h"
#include "lightAPI.h"
#include <assert.h>
#include <limits>

namespace dsr {

// TODO: Make into a constructor for each vector type
inline FVector3D parseFVector3D(const ReadableString& content) {
	List<ReadableString> args = string_split(content, U',');
	if (args.length() != 3) {
		printText("Expected a vector of three decimal values.\n");
		return FVector3D();
	} else {
		return FVector3D(string_toDouble(args[0]), string_toDouble(args[1]), string_toDouble(args[2]));
	}
}

// The sprite instance itself has a game-specific index to the sprite type
// The caller should also have some kind of control over containing and rendering the items
struct Sprite {
public:
	int typeIndex;
	Direction direction;
	IVector3D location; // Displayed at X, Y-Z in world pixel coordinates
	bool shadowCasting;
public:
	Sprite(int typeIndex, Direction direction, const IVector3D& location, bool shadowCasting)
	: typeIndex(typeIndex), direction(direction), location(location), shadowCasting(shadowCasting) {}
};

class SpriteWorldImpl;
using SpriteWorld = std::shared_ptr<SpriteWorldImpl>;

int sprite_loadTypeFromFile(const String& folderPath, const String& spriteName);
int sprite_getTypeCount();

// TODO: Create the ortho system using the content of its configuration file to hide the type itself.
SpriteWorld spriteWorld_create(OrthoSystem ortho, int shadowResolution);
void spriteWorld_addBackgroundSprite(SpriteWorld& world, const Sprite& sprite);
void spriteWorld_addTemporarySprite(SpriteWorld& world, const Sprite& sprite);

// Create a point light that only exists until the next call to spriteWorld_clearTemporary.
//   position is in tile unit world-space.
void spriteWorld_createTemporary_pointLight(SpriteWorld& world, const FVector3D position, float radius, float intensity, ColorRgbI32 color, bool shadowCasting);
void spriteWorld_createTemporary_directedLight(SpriteWorld& world, const FVector3D direction, float intensity, ColorRgbI32 color);

void spriteWorld_clearTemporary(SpriteWorld& world);

void spriteWorld_draw(SpriteWorld& world, AlignedImageRgbaU8& colorTarget);

// The result is an approximation in mini-tile units.
//   The 3D system does not align with screen pixels for less than whole tile units.
//   TODO: See if an exact float position can be returned from pixelToMiniOffset instead of using integers that are less precise.
IVector3D spriteWorld_findGroundAtPixel(SpriteWorld& world, const AlignedImageRgbaU8& colorBuffer, const IVector2D& pixelLocation);

// Approximates a mini-tile offset along the ground from the given pixel offset and moves the camera accordingly
//   If the offset is too small, the camera might not move at all
void spriteWorld_moveCameraInPixels(SpriteWorld& world, const IVector2D& pixelOffset);

// Get internal buffers after rendering.
//   Reading before having drawn the world for the first time will return null because the world does not yet know the target resolution.
//   By not being a part of rendering itself, it cannot go back in time and speed up rendering, so only use for debugging.
//     TODO: Make another feature for actually disabling dynamic light on low-end machines.
AlignedImageRgbaU8 spriteWorld_getDiffuseBuffer(SpriteWorld& world);
OrderedImageRgbaU8 spriteWorld_getNormalBuffer(SpriteWorld& world);
OrderedImageRgbaU8 spriteWorld_getLightBuffer(SpriteWorld& world);
AlignedImageF32 spriteWorld_getHeightBuffer(SpriteWorld& world);

// Access the index of the camera's fixed direction
//   This is not an index selecting the camera itself, only selecting the viewing angle
// TODO: Implement bound checks or a system that's easier to understand.
int spriteWorld_getCameraDirectionIndex(SpriteWorld& world);
void spriteWorld_setCameraDirectionIndex(SpriteWorld& world, int index);

// Pre-conditions:
//   The model should be pre-transformed so that it can be rendered at the world origin
//   Textures must be converted into vertex colors or else they will simply be ignored
//   Enabling debug will save another file using a *Debug.png prefix with additional information
//     Use it to find flaws in generated shadow shapes that are hard to see in raw data
// TODO: Hide OrthoSystem or expose it safely
void sprite_generateFromModel(const Model& visibleModel, const Model& shadowModel, const OrthoSystem& ortho, const String& targetPath, int cameraAngles, bool debug = false);
// A simpler version writing the result to an image and a string instead of saving to files.
void sprite_generateFromModel(ImageRgbaU8& targetAtlas, String& targetConfigText, const Model& visibleModel, const Model& shadowModel, const OrthoSystem& ortho, const String& targetPath, int cameraAngles);

}

#endif

