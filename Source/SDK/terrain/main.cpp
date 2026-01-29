
#include <limits>
#include <cassert>
#include "../../DFPSR/includeFramework.h"

/*
	To do:
		* Make a class for the heightmap.
			This will make it reusable and simplify lazy orthogonal rendering.
				2D rendering allow very high resolutions to be used.
				When the camera moves, background patches are drawn over the screen.
					The camera may only translate in whole pixels along a world center camera location.
				When the camera is still, dirty rectangles clears the background before drawing new dynamic items.
					Items that are currently not changing can be kept if their dirty rectangles aren't colliding with anything animated.
					A parked car or barrel doesn't have to be redrawn until something passes nearby and cuts into it by clearing itself.
				Static buildings are drawn together with the heightmap as the background.
					* A depth buffer based on the world Y coordinate is used for all rendering so that drawing 2D sprites is easy.
			When requesting an orthogonal rendering to be done, the request is done in a 2D coordinates system.
				The coordinates can be calculated from the tile or world space using the orthogonal camera.
				* How can larger maps be made if the whole bump map cannot be stored in memory at once?
					Mostly relevant on the Raspberry PI.
			Changing camera angle or switching to another zoom level will clear all temporary buffers.
			A set of 512² patches are rendered when made visible and deleted when running out of buffers.
				A minimum set of buffers will be calculated from the window size.
			Static light sources can cast shadows on the bump map.
				A double intensity RGBA lightmap might be able to express color light without taking too much cache.
				Can deferred light be used in screen space?
		* A template layer class that updates one image from a set of other images.
			The function being applied as a filter should be virtual.
				The input arguments should hold shared pointers to the inputs.
				Updating is done on demand when the output is requested to be updated for a used region of the image.
				The update will recursively order updates of inputs on the relevant sections.
				Any extra size needed for sampling around the center is added to the bounds.
			Dirty rectangles are sent to order updates.
				The renderer should provide a collection type for dirty rectangles with built in merging of overlapping regions.
			Extend image pyramid generation to allow updating after partial changes to the image.
				This might require refactoring with buffer views so that pyramid layers are actually images.
				Alternatively, just allow giving separate images to the pyramid and give full control.
		* Start to controll the camera and terraform the ground in realtime.
			* Respond to mouse move event at screen borders to move around.
			* Hold a key or something to rotate the camera.
			* Line intersection with triangles in object space transformed using the instance's inverse matrix.
			* Partial update of diffuse map and the final color map.
				The outmost edges of the height map must be at height zero to avoid seeing the end of we world as a seam.
			* Place the camera around a non-zero location so that the map's upper left corner is at zero in world space.
				One tile should be one length unit in 3D.
			* Draw a 3D model where the tool is placed in 3D.
		* Allow spraying decals on the map.
			Both bump and diffuse maps should be affected by decal layers.
			Decals should be possible to serialize for efficiently compressed level design.
		* Make some kind of vehicle that a camera can follow when playing in third person mode.
			Maybe a helicopter that can shoot rockets on turrets and enemy cars.
		* Try to divide the ground model into multiple sections so that model culling can be implemented for a known 0..255 height bound.
			When actually generating models, the framework can generate the bounds automatically.
		* Make it fast and simple to convert between world space coordinates (x, y, z) and map indices (x, -z) for multiple resolutions.
			One unit in world space should be one square in the grid.
			Buildings are placed in a tile system where the height map has to be made flat before they can be placed adjacent to each other.
			Some buildings like defensive walls does not require a completely flat ground.
		* Try to make a reusable class from the terrain that knows its bounding box for culling from the minimum and maximum height.
			If good enough, the terrain class can be in an game engine around the core renderer.
			The core has the generic stuff that everyone needs.
			The game engine has lots of features that are useful for games.
				* Image loading
				* Advanced model format
				* Terrain model
				* Encapsulation of internal complexity with a procedural API
	Notes:
		* If sunlight is aligned with an axis then an algorithm for ray-tracing the mountain can be made very efficient.
			Remember at what height the light reached in the previous pixel and lower it by the pixelwise light slope.
			If it goes below the ground's height, set it to the ground's height.
			If the light is above the ground level, the sun is occluded.
			The thresholding can be made soft by linearly fading when light is a bit above the ground as if grass absorbed the light partially.
			Having a static lightmap might however not need this optimizing restriction.
*/

using namespace dsr;

const String applicationFolder = file_getApplicationFolder();
const String mediaFolder = file_combinePaths(applicationFolder, U"media");

// A real point (x, y, z) may touch a certain tile at integer indices (tileU, tileV) when:
//   tileU <= x <= tileU + 1.0
//   0.0f <= y <= highestGround
//   tileV <= -z <= tileV + 1.0

// Heightmaps use integers in the range 0..255 to express heights from 0.0 to highestGround
static const float highestGround = 5.0f;
	static const float heightPerUnit = highestGround / 255.0f; // One unit of height converted to world space
// One tile in a height map is 1x1 xz units in world space
static const uint32_t colorDensityShift = 4; // 2 ^ colorDensityShift = tileColorDensity
	static const uint32_t tileColorDensity = 1 << colorDensityShift; // tileColorDensity² color pixels per tile. Last tileColorDensity-1 rows and columns are unused in the high-resolution maps.
	static const uint32_t colorDensityRemainderMask = tileColorDensity - 1; // Only the colorDensityShift last bits
	static const uint32_t colorDensityWholeMask = ~colorDensityRemainderMask; // Masking out the colorDensityShift last bits
	static const float reciprocalDensity = 1.0f / (float)tileColorDensity;
	static const float squareReciprocalDensity = reciprocalDensity * reciprocalDensity;
	// Returns floor(x / tileColorDensity)
	static inline uint32_t wholeTile(uint32_t x) {
		return (x & colorDensityWholeMask) >> colorDensityShift;
	}
	// Returns x % tileColorDensity
	static inline uint32_t remTile(uint32_t x) {
		return x & colorDensityRemainderMask;
	}
	// Returns tileColorDensity - x
	static inline uint32_t invRem(uint32_t x) {
		return tileColorDensity - x;
	}

FVector3D gridToWorld(float tileU, float tileV, float height) {
	return FVector3D(tileU, height, -tileV);
}

FVector3D worldToGrid(FVector3D worldSpace) {
	return FVector3D(worldSpace.x, -worldSpace.z, worldSpace.y);
}

float getHeight(const ImageU8& heightMap, int32_t u, int32_t v) {
	return image_readPixel_border(heightMap, u, v) * heightPerUnit;
}

int32_t createGridPart(Model& targetModel, const ImageU8& heightMap) {
	int32_t mapWidth = image_getWidth(heightMap);
	int32_t mapHeight = image_getHeight(heightMap);
	float scaleU = 1.0f / (mapWidth - 1.0f);
	float scaleV = 1.0f / (mapHeight - 1.0f);
	// Create a part for the polygons
	int32_t part = model_addEmptyPart(targetModel, U"grid");
	for (int32_t z = 0; z < mapHeight; z++) {
		for (int32_t x = 0; x < mapWidth; x++) {
			// Sample the height map and convert to world space
			float height = getHeight(heightMap, x, z);
			// Create a position from the 3D index
			FVector3D position = gridToWorld(x, z, height);
			// Add the point to the model
			model_addPoint(targetModel, position);
			if (x > 0 && z > 0) {
				// Create vertex data
				//   A-B
				//     |
				//   D-C
				int32_t px = x - 1;
				int32_t pz = z - 1;
				int32_t indexA = px + pz * mapWidth;
				int32_t indexB = x  + pz * mapWidth;
				int32_t indexC = x  + z  * mapWidth;
				int32_t indexD = px + z  * mapWidth;
				FVector4D texA = FVector4D(px * scaleU, pz * scaleV, 0.0f, 0.0f);
				FVector4D texB = FVector4D(x  * scaleU, pz * scaleV, 0.0f, 0.0f);
				FVector4D texC = FVector4D(x  * scaleU, z  * scaleV, 0.0f, 0.0f);
				FVector4D texD = FVector4D(px * scaleU, z  * scaleV, 0.0f, 0.0f);
				// Create a polygon unless it's at the bottom
				if (image_readPixel_border(heightMap, x-1, z-1) > 0 || image_readPixel_border(heightMap, x, z-1) > 0 || image_readPixel_border(heightMap, x-1, z) > 0 || image_readPixel_border(heightMap, x, z) > 0) {
					if ((x + z) % 2 == 0) {
						int32_t poly = model_addQuad(targetModel, part, indexA, indexB, indexC, indexD);
						model_setTexCoord(targetModel, part, poly, 0, texA);
						model_setTexCoord(targetModel, part, poly, 1, texB);
						model_setTexCoord(targetModel, part, poly, 2, texC);
						model_setTexCoord(targetModel, part, poly, 3, texD);
					} else {
						int32_t poly = model_addQuad(targetModel, part, indexB, indexC, indexD, indexA);
						model_setTexCoord(targetModel, part, poly, 0, texB);
						model_setTexCoord(targetModel, part, poly, 1, texC);
						model_setTexCoord(targetModel, part, poly, 2, texD);
						model_setTexCoord(targetModel, part, poly, 3, texA);
					}
				}
			}
		}
	}
	return part;
}

static Model createGrid(const ImageU8& heightMap, const TextureRgbaU8& colorMap) {
	Model model = model_create();
	int32_t part = createGridPart(model, heightMap);
	model_setDiffuseMap(model, part, colorMap);
	return model;
}

static inline int32_t saturateFloat(float value) {
	if (!(value >= 0.0f)) {
		// NaN or negative
		return 0;
	} else if (value > 255.0f) {
		// Too large
		return 255;
	} else {
		// Round to closest
		return (int32_t)(value + 0.5f);
	}
}

float sampleFixedBilinear(const ImageU8& heightMap, uint32_t x, uint32_t y) {
	// Get whole coordinates
	uint32_t lowX = wholeTile(x);
	uint32_t lowY = wholeTile(y);
	// Sample neighbors
	uint32_t upperLeft = image_readPixel_clamp(heightMap, lowX, lowY);
	uint32_t upperRight = image_readPixel_clamp(heightMap, lowX + 1, lowY);
	uint32_t lowerLeft = image_readPixel_clamp(heightMap, lowX, lowY + 1);
	uint32_t lowerRight = image_readPixel_clamp(heightMap, lowX + 1, lowY + 1);
	// Get weights
	uint32_t wX = remTile(x);
	uint32_t wY = remTile(y);
	uint32_t iwX = invRem(wX);
	uint32_t iwY = invRem(wY);
	// Combine
	uint32_t upper = upperLeft * iwX + upperRight * wX;
	uint32_t lower = lowerLeft * iwX + lowerRight * wX;
	uint32_t center = upper * iwY + lower * wY;
	// Normalize as float
	return (float)center * squareReciprocalDensity;
}

ColorRgbaI32 sampleColorRampLinear(const ImageRgbaU8& colorRamp, float x) {
	assert(image_getWidth(colorRamp) == 256 && image_getHeight(colorRamp) == 1);
	if (x < 0.0f) {
		return image_readPixel_clamp(colorRamp, 0, 0);
	} else if (x > 255.0f) {
		return image_readPixel_clamp(colorRamp, 255, 0);
	} else {
		int32_t low = (int32_t)x;
		int32_t high = low + 1;
		float weight = x - low;
		ColorRgbaI32 lowColor = image_readPixel_clamp(colorRamp, low, 0);
		ColorRgbaI32 highColor = image_readPixel_clamp(colorRamp, high, 0);
		return ColorRgbaI32::mix(lowColor, highColor, weight);
	}
}

// Represents the height in a finer pixel density for material effects
void generateBumpMap(ImageF32& targetBumpMap, const ImageU8& heightMap, const ImageU8& bumpPattern) {
	for (int32_t y = 0; y < image_getHeight(targetBumpMap); y++) {
		for (int32_t x = 0; x < image_getWidth(targetBumpMap); x++) {
			// TODO: Apply gaussian blur after bilinear interpolation to hide seams.
			float height = (
			    sampleFixedBilinear(heightMap, x, y)
			  + sampleFixedBilinear(heightMap, x + 8, y - 10)
			  + sampleFixedBilinear(heightMap, x - 10, y - 8)
			  + sampleFixedBilinear(heightMap, x - 8, y + 10)
			  + sampleFixedBilinear(heightMap, x + 10, y + 8)
			  + sampleFixedBilinear(heightMap, x - 4, y - 6)
			  + sampleFixedBilinear(heightMap, x + 6, y - 4)
			  + sampleFixedBilinear(heightMap, x + 4, y + 6)
			  + sampleFixedBilinear(heightMap, x - 6, y + 4)
			  + sampleFixedBilinear(heightMap, x + 3, y - 5)
			  + sampleFixedBilinear(heightMap, x - 5, y - 3)
			  + sampleFixedBilinear(heightMap, x - 3, y + 5)
			  + sampleFixedBilinear(heightMap, x + 5, y + 3)
			) / 13.0f;
			float bump = image_readPixel_tile(bumpPattern, x, y) - 127.5f;
			image_writePixel(targetBumpMap, x, y, std::max(0.0f, height + bump * 0.1f));
		}
	}
}

FVector3D getNormal(const ImageF32& bumpMap, int32_t x, int32_t y) {
	float bumpLeft = image_readPixel_clamp(bumpMap, x - 1, y);
	float bumpRight = image_readPixel_clamp(bumpMap, x + 1, y);
	float bumpUp = image_readPixel_clamp(bumpMap, x, y - 1);
	float bumpDown = image_readPixel_clamp(bumpMap, x, y + 1);
	static const float DistancePerTwoPixels = 2.0 / tileColorDensity; // From -1 to +1 in pixels converted to world space distance
	static const float scale = heightPerUnit / DistancePerTwoPixels;
	return normalize(FVector3D((bumpUp - bumpDown) * scale, 1.0f, (bumpRight - bumpLeft) * scale));
}

void generateLightMap(ImageF32& targetLightMap, const ImageF32& bumpMap, const FVector3D& sunDirection, float ambient) {
	for (int32_t y = 0; y < image_getHeight(targetLightMap); y++) {
		for (int32_t x = 0; x < image_getWidth(targetLightMap); x++) {
			FVector3D surfaceNormal = getNormal(bumpMap, x, y);
			float angularIntensity = std::max(0.0f, dotProduct(surfaceNormal, -sunDirection));
			image_writePixel(targetLightMap, x, y, angularIntensity + ambient);
		}
	}
}

void generateDiffuseMap(ImageRgbaU8& targetDiffuseMap, const ImageF32& bumpMap, const ImageRgbaU8& heightColorRamp) {
	for (int32_t y = 0; y < image_getHeight(targetDiffuseMap); y++) {
		for (int32_t x = 0; x < image_getWidth(targetDiffuseMap); x++) {
			float height = image_readPixel_clamp(bumpMap, x, y);
			ColorRgbaI32 rampColor = sampleColorRampLinear(heightColorRamp, height);
			/*int32_t rx = remTile(x);
			int32_t ry = remTile(y);
			bool gridLine = rx == 0 || ry == 0 || rx == tileColorDensity - 1 || ry == tileColorDensity - 1;
			if (gridLine) {
				rampColor = ColorRgbaI32(
				  rampColor.red / 2,
				  rampColor.green / 2,
				  rampColor.blue / 2,
				  255
				);
			}*/
			image_writePixel(targetDiffuseMap, x, y, rampColor);
		}
	}
}

// Full update of the ground
void updateColorMap(ImageRgbaU8& targetColorMap, const ImageRgbaU8& diffuseMap, const ImageF32& lightMap) {
	for (int32_t y = 0; y < image_getHeight(targetColorMap); y++) {
		for (int32_t x = 0; x < image_getWidth(targetColorMap); x++) {
			ColorRgbaI32 diffuse = image_readPixel_clamp(diffuseMap, x, y);
			float light = image_readPixel_clamp(lightMap, x, y);
			image_writePixel(targetColorMap, x, y, diffuse * light);
		}
	}
}

// Variables
IVector2D mousePos;
bool running = true;
bool showBuffers = false;

// The window handle
Window window;

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Create a window
	window = window_create(U"David Piuva's Software Renderer - Terrain example", 1600, 900);

	// Bind methods to events
	window_setKeyboardEvent(window, [](const KeyboardEvent& event) {
		if (event.keyboardEventType == KeyboardEventType::KeyDown) {
			DsrKey key = event.dsrKey;
			if (key == DsrKey_B) {
				showBuffers = !showBuffers;
			} else if (key == DsrKey_F11) {
				window_setFullScreen(window, !window_isFullScreen(window));
			} else if (key == DsrKey_Escape) {
				running = false;
			}
		}
	});
	window_setMouseEvent(window, [](const MouseEvent& event) {
		mousePos = event.position;
	});
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Load height map
	ImageU8 heightMap = image_get_red(image_load_RgbaU8(file_combinePaths(mediaFolder, U"HeightMap.png")));
	// Load generic cloud pattern
	ImageU8 genericCloudPattern = image_get_red(image_load_RgbaU8(file_combinePaths(mediaFolder, U"Cloud.png")));
	// Load height ramp
	ImageRgbaU8 heightRamp = image_load_RgbaU8(file_combinePaths(mediaFolder, U"RampIsland.png"));

	// Get dimensions
	const int32_t heighMapWidth = image_getWidth(heightMap);
	const int32_t heighMapHeight = image_getHeight(heightMap);
	const int32_t colorMapWidth = heighMapWidth * tileColorDensity;
	const int32_t colorMapHeight = heighMapHeight * tileColorDensity;

	// Create a bump map in the same 0..255 range as the height map, but using floats
	ImageF32 bumpMap = image_create_F32(colorMapWidth, colorMapHeight);
	generateBumpMap(bumpMap, heightMap, genericCloudPattern);

	// Create a light map
	ImageF32 lightMap = image_create_F32(colorMapWidth, colorMapHeight);
	FVector3D sunDirection = normalize(FVector3D(0.3f, -1.0f, 1.0f));
	float ambient = 0.2f;
	generateLightMap(lightMap, bumpMap, sunDirection, ambient);

	// Create a diffuse image
	ImageRgbaU8 diffuseMap = image_create_RgbaU8(colorMapWidth, colorMapHeight);
	generateDiffuseMap(diffuseMap, bumpMap, heightRamp);

	// Create a color texture with 5 resolutions.
	TextureRgbaU8 colorTexture = texture_create_RgbaU8(colorMapWidth, colorMapHeight, 5);
	// Get the highest texture resolution as an image for easy manipulation.
	ImageRgbaU8 colorMap = texture_getMipLevelImage(colorTexture, 0);
	// Update the color map and texture.
	updateColorMap(colorMap, diffuseMap, lightMap);
	texture_generatePyramid(colorTexture);

	// Create a ground model
	Model ground = createGrid(heightMap, colorTexture);

	// Create a renderer for multi-threading
	Renderer worker = renderer_create();

	while(running) {
		double startTime;
		window_executeEvents(window);

		// Request buffers after executing the events, to get newly allocated buffers after resize events
		ImageRgbaU8 colorBuffer = window_getCanvas(window);
		ImageF32 depthBuffer = window_getDepthBuffer(window);

		// Get target size
		int32_t targetWidth = image_getWidth(colorBuffer);
		int32_t targetHeight = image_getHeight(colorBuffer);

		// Paint the background color
		startTime = time_getSeconds();
		image_fill(colorBuffer, ColorRgbaI32(0, 0, 0, 0)); // Setting each channel to the same value can use memset for faster filling
		printText(U"Fill sky: ", (time_getSeconds() - startTime) * 1000.0, U" ms\n");

		// Clear the depth buffer
		startTime = time_getSeconds();
		image_fill(depthBuffer, 0.0f); // Infinite reciprocal depth using default zero
		printText(U"Clear depth: ", (time_getSeconds() - startTime) * 1000.0, U" ms\n");

		// Create a camera
		const double speed = 0.2f;
		double timer = time_getSeconds() * speed;
		float distance = mousePos.y * 0.03f + 10.0f;
		FVector3D worldCenter = FVector3D(heighMapWidth * 0.5f, 0.0f, heighMapHeight * -0.5f);
		FVector3D cameraOffset = FVector3D(sin(timer) * distance, mousePos.x * 0.03f + 10.0f, cos(timer) * distance);
		FVector3D cameraPosition = worldCenter + cameraOffset;
		FMatrix3x3 cameraRotation = FMatrix3x3::makeAxisSystem(-cameraOffset, FVector3D(0.0f, 1.0f, 0.0f));
		Camera camera = Camera::createPerspective(Transform3D(cameraPosition, cameraRotation), targetWidth, targetHeight);

		// Render the ground using multi-threading
		renderer_begin(worker, colorBuffer, depthBuffer);
		startTime = time_getSeconds();
		renderer_giveTask(worker, ground, Transform3D(), camera);
		printText(U"Project triangles: ", (time_getSeconds() - startTime) * 1000.0, U" ms\n");
		startTime = time_getSeconds();
		renderer_end(worker);
		printText(U"Rasterize triangles: ", (time_getSeconds() - startTime) * 1000.0, U" ms\n");

		if (showBuffers) {
			startTime = time_getSeconds();
			//draw_copy(colorBuffer, colorMap, mousePos.x, mousePos.y);
			//draw_copy(colorBuffer, heightMap, mousePos.x, mousePos.y);
			draw_copy(colorBuffer, bumpMap, mousePos.x, mousePos.y);
			printText(U"Show buffers: ", (time_getSeconds() - startTime) * 1000.0, U" ms\n");
		}

		window_showCanvas(window);
	}

	printText(U"\nTerminating the application.\n");
}
