
#include "orthoAPI.h"

namespace dsr {

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

