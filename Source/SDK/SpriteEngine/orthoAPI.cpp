
#include "orthoAPI.h"

namespace dsr {

OrthoView::OrthoView(int id, const IVector2D roundedXAxis, const IVector2D roundedZAxis, int yPixelsPerTile, const FMatrix3x3 &normalToWorldSpace, Direction worldDirection)
: id(id), worldDirection(worldDirection), normalToWorldSpace(normalToWorldSpace),
  pixelOffsetPerTileX(roundedXAxis), pixelOffsetPerTileZ(roundedZAxis), yPixelsPerTile(yPixelsPerTile) {
	// Pixel aligned 3D transformation matrix from tile (x, y, z) to screen (x, y, h)
	FMatrix3x3 tileToScreen = FMatrix3x3(
		FVector3D(roundedXAxis.x, roundedXAxis.y, 0),
		FVector3D(0, -this->yPixelsPerTile, 1.0f),
		FVector3D(roundedZAxis.x, roundedZAxis.y, 0)
	);
	// Back from deep screen pixels to world tile coordinates
	FMatrix3x3 screenToTile = inverse(tileToScreen);

	// TODO: Obsolete
	this->roundedScreenPixelsToWorldTiles = inverse(FMatrix2x2(FVector2D(roundedXAxis.x, roundedXAxis.y), FVector2D(roundedZAxis.x, roundedZAxis.y)));

	// Save the conversion from screen-space to world-space in tile units
	this->screenDepthToWorldSpace = screenToTile;
	this->worldSpaceToScreenDepth = tileToScreen;

	// Save the conversion from screen-space to light-space in tile units
	this->screenDepthToLightSpace = FMatrix3x3(
	  this->normalToWorldSpace.transformTransposed(screenToTile.xAxis),
	  this->normalToWorldSpace.transformTransposed(screenToTile.yAxis),
	  this->normalToWorldSpace.transformTransposed(screenToTile.zAxis)
	);
	this->lightSpaceToScreenDepth = inverse(this->screenDepthToLightSpace);
}

IVector2D OrthoView::miniTileOffsetToScreenPixel(const IVector3D& miniTileOffset) const {
	IVector2D centeredPixelLocation = this->pixelOffsetPerTileX * miniTileOffset.x + this->pixelOffsetPerTileZ * miniTileOffset.z;
	centeredPixelLocation.y -= miniTileOffset.y * this->yPixelsPerTile;
	return centeredPixelLocation / ortho_miniUnitsPerTile;
}

IVector2D OrthoView::miniTilePositionToScreenPixel(const IVector3D& position, const IVector2D& worldCenter) const {
	return this->miniTileOffsetToScreenPixel(position) + worldCenter;
}

FVector3D OrthoView::pixelToTileOffset(const IVector2D& pixelOffset) const {
	FVector2D xzTiles = this->roundedScreenPixelsToWorldTiles.transform(FVector2D(pixelOffset.x, pixelOffset.y));
	return FVector3D(xzTiles.x, 0.0f, xzTiles.y);
}

IVector3D OrthoView::pixelToMiniOffset(const IVector2D& pixelOffset) const {
	FVector3D tiles = this->pixelToTileOffset(pixelOffset);
	return IVector3D(ortho_floatingTileToMini(tiles.x), 0, ortho_floatingTileToMini(tiles.z));
}

IVector3D OrthoView::pixelToMiniPosition(const IVector2D& pixelLocation, const IVector2D& worldCenter) const {
	return this->pixelToMiniOffset(pixelLocation - worldCenter);
}

OrthoSystem::OrthoSystem() : cameraTilt(0), pixelsPerTile(0) {}

OrthoSystem::OrthoSystem(float cameraTilt, int pixelsPerTile) : cameraTilt(cameraTilt), pixelsPerTile(pixelsPerTile) {
	this->update();
}

OrthoSystem::OrthoSystem(const ReadableString& content) {
	config_parse_ini(content, [this](const ReadableString& block, const ReadableString& key, const ReadableString& value) {
		if (string_length(block) == 0) {
			if (string_caseInsensitiveMatch(key, U"DownTiltPerThousand")) {
				this->cameraTilt = (float)string_toInteger(value) * -0.001f;
			} else if (string_caseInsensitiveMatch(key, U"PixelsPerTile")) {
				this->pixelsPerTile = string_toInteger(value);
			} else {
				printText("Unrecognized key \"", key, "\" in orthogonal camera configuration file.\n");
			}
		} else {
			printText("Unrecognized block \"", block, "\" in orthogonal camera configuration file.\n");
		}
	});
	this->update();
}

void OrthoSystem::update() {
	// Calculate y offset rounded to whole tiles to prevent random gaps in grids
	int yPixelsPerTile = (float)this->pixelsPerTile / sqrt(this->cameraTilt * this->cameraTilt + 1);

	// Define sprite directions
	FVector3D upAxis = FVector3D(0.0f, 1.0f, 0.0f);
	Direction worldDirections[8] = {dir315, dir45, dir135, dir225, dir0, dir90, dir180, dir270};
	// Define approximate camera systems just to get something axis aligned
	FMatrix3x3 cameraSystems[8];
	cameraSystems[0] = FMatrix3x3::makeAxisSystem(FVector3D(diag, this->cameraTilt, diag), upAxis);
	cameraSystems[1] = FMatrix3x3::makeAxisSystem(FVector3D(-diag, this->cameraTilt, diag), upAxis);
	cameraSystems[2] = FMatrix3x3::makeAxisSystem(FVector3D(-diag, this->cameraTilt, -diag), upAxis);
	cameraSystems[3] = FMatrix3x3::makeAxisSystem(FVector3D(diag, this->cameraTilt, -diag), upAxis);
	cameraSystems[4] = FMatrix3x3::makeAxisSystem(FVector3D( 0, this->cameraTilt, 1), upAxis);
	cameraSystems[5] = FMatrix3x3::makeAxisSystem(FVector3D(-1, this->cameraTilt, 0), upAxis);
	cameraSystems[6] = FMatrix3x3::makeAxisSystem(FVector3D( 0, this->cameraTilt,-1), upAxis);
	cameraSystems[7] = FMatrix3x3::makeAxisSystem(FVector3D( 1, this->cameraTilt, 0), upAxis);

	for (int a = 0; a < maxCameraAngles; a++) {
		// Define the coordinate system for normals
		FVector3D normalSystemDirection = cameraSystems[a].zAxis;
		normalSystemDirection.y = 0.0f;
		FMatrix3x3 normalToWorldSpace = FMatrix3x3::makeAxisSystem(normalSystemDirection, FVector3D(0.0f, 1.0f, 0.0f));
		// Create an axis system truncated inwards to whole pixels to prevent creating empty seams between tile aligned sprites
		Camera approximateCamera = Camera::createOrthogonal(Transform3D(FVector3D(), cameraSystems[a]), this->pixelsPerTile, this->pixelsPerTile, 0.5f);
		float halfTile = (float)this->pixelsPerTile * 0.5f;
		FVector2D XAxis = approximateCamera.worldToScreen(FVector3D(1.0f, 0.0f, 0.0f)).is - halfTile;
		FVector2D ZAxis = approximateCamera.worldToScreen(FVector3D(0.0f, 0.0f, 1.0f)).is - halfTile;
		this->view[a] = OrthoView(
		  a,
		  IVector2D((int)XAxis.x, (int)XAxis.y),
		  IVector2D((int)ZAxis.x, (int)ZAxis.y),
		  yPixelsPerTile,
		  normalToWorldSpace,
		  worldDirections[a]
		);
	}
}

int ortho_roundToTile(int miniCoordinate) {
	return roundDown(miniCoordinate + (ortho_miniUnitsPerTile / 2), ortho_miniUnitsPerTile);
}

IVector3D ortho_roundToTile(const IVector3D& miniPosition) {
	return IVector3D(ortho_roundToTile(miniPosition.x), miniPosition.y, ortho_roundToTile(miniPosition.z));
}

float ortho_miniToFloatingTile(int miniCoordinate) {
	return (float)miniCoordinate * ortho_tilesPerMiniUnit;
}

FVector3D ortho_miniToFloatingTile(const IVector3D& miniPosition) {
	return FVector3D(
	  ortho_miniToFloatingTile(miniPosition.x),
	  ortho_miniToFloatingTile(miniPosition.y),
	  ortho_miniToFloatingTile(miniPosition.z)
	);
}

int ortho_floatingTileToMini(float tileCoordinate) {
	return (int)round((double)tileCoordinate * (double)ortho_miniUnitsPerTile);
}

IVector3D ortho_floatingTileToMini(const FVector3D& tilePosition) {
	return IVector3D(
	  ortho_floatingTileToMini(tilePosition.x),
	  ortho_floatingTileToMini(tilePosition.y),
	  ortho_floatingTileToMini(tilePosition.z)
	);
}

}

