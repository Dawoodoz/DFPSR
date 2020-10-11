
#ifndef DFPSR_ORTHO
#define DFPSR_ORTHO

#include <assert.h>
#include "../../DFPSR/includeFramework.h"

namespace dsr {

// TODO: Give an ortho_ prefix
using Direction = int32_t;
static const Direction dir360 = 8;
static const Direction dir315 = 7;
static const Direction dir270 = 6;
static const Direction dir225 = 5;
static const Direction dir180 = 4;
static const Direction dir135 = 3;
static const Direction dir90 = 2;
static const Direction dir45 = 1;
static const Direction dir0 = 0;
inline int correctDirection(Direction direction) {
	return (int32_t)((uint32_t)((int32_t)direction + (dir360 * 1024)) % dir360);
}

// World 3D units
//   Tile = Diameter from one side to another along a standard tile
//     Used for expressing exact tile indices in games so that information can be stored efficiently
//   Mini-Tile = Tile / miniUnitsPerTile
//     Used to express locations in 3D without relying too much on non-deterministic floats
static constexpr int ortho_miniUnitsPerTile = 1024;
static constexpr float ortho_tilesPerMiniUnit = 1.0f / (float)ortho_miniUnitsPerTile;

int ortho_roundToTile(int miniCoordinate);
IVector3D ortho_roundToTile(const IVector3D& miniPosition);
float ortho_miniToFloatingTile(int miniCoordinate);
FVector3D ortho_miniToFloatingTile(const IVector3D& miniPosition);
int ortho_floatingTileToMini(float tileCoordinate);
IVector3D ortho_floatingTileToMini(const FVector3D& tilePosition);

// TODO: Make sure that every conversion is derived from a single pixel-rounded world-to-screen transform
//       Do this by letting it be the only argument for construction using integers
//       Everything else will simply be derived from it on construction
struct OrthoView {
public:
	// Unique integer for identifying the view
	int id = -1;

	// Direction for rotating sprites
	Direction worldDirection = dir0; // How are sprites in the world rotated relative to the camera's point of view

	// The rotating transform from normal-space to world-space.
	//   Light-space is a superset of normal-space with the origin around the camera. (Almost like camera-space but with Y straight up)
	FMatrix3x3 normalToWorldSpace;

	// Pixel aligned space (To ensure that moving one tile has the same number of pixels each time)
	IVector2D pixelOffsetPerTileX; // How many pixels does a sprite move per tile in X.
	IVector2D pixelOffsetPerTileZ; // How many pixels does a sprite move per tile in Z.
	int yPixelsPerTile = 0;

	// How pixels in the depth buffer maps to world-space coordinates in whole floating tiles.
	FMatrix3x3 screenDepthToWorldSpace;
	FMatrix3x3 worldSpaceToScreenDepth;
	// How pixels in the depth buffer maps to light-space coordinates in whole floating tiles.
	//   The origin is at the center of the image.
	//   The X and Y axis gives tile offsets in light space along the screen without depth information.
	//   The Z axis gives tile offset per mini-tile unit of height in the depth buffer.
	FMatrix3x3 screenDepthToLightSpace;
	FMatrix3x3 lightSpaceToScreenDepth;

	// Conversion systems between rounded pixels and XZ tiles along Y = 0
	FMatrix2x2 roundedScreenPixelsToWorldTiles; // TODO: Replace with a screenToTile sub-set
public:
	OrthoView() {}
	OrthoView(int id, const IVector2D roundedXAxis, const IVector2D roundedZAxis, int yPixelsPerTile, const FMatrix3x3 &normalToWorldSpace, Direction worldDirection);
public:
	IVector2D miniTileOffsetToScreenPixel(const IVector3D& miniTileOffset) const;
	// Position is expressed in world space using mini units
	IVector2D miniTilePositionToScreenPixel(const IVector3D& position, const IVector2D& worldCenter) const;
	// Returns the 3D mini-tile units moved along the ground for the pixel offset
	//   Only rotation and scaling for pixel offsets
	FVector3D pixelToTileOffset(const IVector2D& pixelOffset) const;
	IVector3D pixelToMiniOffset(const IVector2D& pixelOffset) const;
	// Returns the 3D mini-tile location for a certain pixel on the screen intersecting with the ground
	//   Full transform for pixel locations
	IVector3D pixelToMiniPosition(const IVector2D& pixelLocation, const IVector2D& worldCenter) const;
};

// How to use the orthogonal system
//   * Place tiles in whole tile integer units
//     Multiply directly with pixelOffsetPerTileX and pixelOffsetPerTileZ to get deterministic pixel offsets
//   * Define sprites in mini units (1 tile = ortho_miniUnitsPerTile mini units)
//     First multiply mini units with yPixelsPerTile, pixelOffsetPerTileX and pixelOffsetPerTileZ for each 3D coordinate
//     Then divide by ortho_miniUnitsPerTile, which most processors should have custom instructions for handling quickly
//     With enough bits in the integers, the result should be steady and not shake around randomly
struct OrthoSystem {
public:
	static constexpr int maxCameraAngles = 8;
	static constexpr float diag = 0.707106781f; // cos(45 degrees) = Sqrt(2) / 2
	// Persistent settings
	float cameraTilt; // Camera coefficient. (-inf is straight down, -1 is diagonal down, 0 is horizontal)
	int pixelsPerTile; // The sideway length of a tile in pixels when seen from straight ahead.
	// Generated views
	OrthoView view[maxCameraAngles];
private:
	// Update generated settings from persistent settings
	// Enforces a valid orthogonal camera system
	void update();
public:
	OrthoSystem();
	OrthoSystem(float cameraTilt, int pixelsPerTile);
	explicit OrthoSystem(const ReadableString& content);
public:
	IVector2D miniTileOffsetToScreenPixel(const IVector3D& miniTileOffset, int cameraIndex) const {
		return this->view[cameraIndex].miniTileOffsetToScreenPixel(miniTileOffset);
	}
	// Position is expressed in world space using mini units
	IVector2D miniTilePositionToScreenPixel(const IVector3D& position, int cameraIndex, const IVector2D& worldCenter) const {
		return this->view[cameraIndex].miniTilePositionToScreenPixel(position, worldCenter);
	}
public:
	// Returns the 3D mini-tile units moved along the ground for the pixel offset
	//   Only rotation and scaling for pixel offsets
	FVector3D pixelToTileOffset(const IVector2D& pixelOffset, int cameraIndex) const {
		return this->view[cameraIndex].pixelToTileOffset(pixelOffset);
	}
	IVector3D pixelToMiniOffset(const IVector2D& pixelOffset, int cameraIndex) const {
		return this->view[cameraIndex].pixelToMiniOffset(pixelOffset);
	}
	// Returns the 3D mini-tile location for a certain pixel on the screen intersecting with the ground
	//   Full transform for pixel locations
	IVector3D pixelToMiniPosition(const IVector2D& pixelLocation, int cameraIndex, const IVector2D& worldCenter) const {
		return this->view[cameraIndex].pixelToMiniPosition(pixelLocation, worldCenter);
	}
};

}

#endif

