
#include "spriteAPI.h"
#include "Octree.h"
#include "DirtyRectangles.h"
#include "importer.h"
#include "../../DFPSR/render/ITriangle2D.h"
#include "../../DFPSR/base/endian.h"
#include "../../DFPSR/math/scalar.h"

// Comment out a flag to disable an optimization when debugging
#define DIRTY_RECTANGLE_OPTIMIZATION

namespace dsr {

template <bool HIGH_QUALITY>
static IRect renderModel(const Model& model, OrthoView view, ImageF32 depthBuffer, ImageRgbaU8 diffuseTarget, ImageRgbaU8 normalTarget, const FVector2D& worldOrigin, const Transform3D& modelToWorldSpace);

template <bool HIGH_QUALITY>
static IRect renderDenseModel(const DenseModel& model, OrthoView view, ImageF32 depthBuffer, ImageRgbaU8 diffuseTarget, ImageRgbaU8 normalTarget, const FVector2D& worldOrigin, const Transform3D& modelToWorldSpace);

static Transform3D combineWorldToScreenTransform(const FMatrix3x3& worldSpaceToScreenDepth, const FVector2D& worldOrigin) {
	return Transform3D(FVector3D(worldOrigin.x, worldOrigin.y, 0.0f), worldSpaceToScreenDepth);
}

static Transform3D combineModelToScreenTransform(const Transform3D& modelToWorldSpace, const FMatrix3x3& worldSpaceToScreenDepth, const FVector2D& worldOrigin) {
	return modelToWorldSpace * combineWorldToScreenTransform(worldSpaceToScreenDepth, worldOrigin);
}

static FVector3D IVector3DToFVector3D(const IVector3D& v) {
	return FVector3D(v.x, v.y, v.z);
}

static IVector3D FVector3DToIVector3D(const FVector3D& v) {
	return IVector3D(v.x, v.y, v.z);
}

struct SpriteConfig {
	int centerX, centerY; // The sprite's origin in pixels relative to the upper left corner
	int frameRows; // The atlas has one row for each frame
	int propertyColumns; // The atlas has one column for each type of information
	// The 3D model's bound in tile units
	//   The height image goes from 0 at minimum Y to 255 at maximum Y
	FVector3D minBound, maxBound;
	// Shadow shapes
	List<FVector3D> points; // 3D points for the triangles to refer to by index
	List<int32_t> triangleIndices; // Triangle indices stored in multiples of three integers
	// Construction
	SpriteConfig(int centerX, int centerY, int frameRows, int propertyColumns, FVector3D minBound, FVector3D maxBound)
	: centerX(centerX), centerY(centerY), frameRows(frameRows), propertyColumns(propertyColumns), minBound(minBound), maxBound(maxBound) {}
	explicit SpriteConfig(const ReadableString& content) {
		config_parse_ini(content, [this](const ReadableString& block, const ReadableString& key, const ReadableString& value) {
			if (string_length(block) == 0) {
				if (string_caseInsensitiveMatch(key, U"CenterX")) {
					this->centerX = string_toInteger(value);
				} else if (string_caseInsensitiveMatch(key, U"CenterY")) {
					this->centerY = string_toInteger(value);
				} else if (string_caseInsensitiveMatch(key, U"FrameRows")) {
					this->frameRows = string_toInteger(value);
				} else if (string_caseInsensitiveMatch(key, U"PropertyColumns")) {
					this->propertyColumns = string_toInteger(value);
				} else if (string_caseInsensitiveMatch(key, U"MinBound")) {
					this->minBound = parseFVector3D(value);
				} else if (string_caseInsensitiveMatch(key, U"MaxBound")) {
					this->maxBound = parseFVector3D(value);
				} else if (string_caseInsensitiveMatch(key, U"Points")) {
					List<String> values = string_split(value, U',');
					if (values.length() % 3 != 0) {
						throwError("Points contained ", values.length(), " values, which is not evenly divisible by three!");
					} else {
						this->points.clear();
						this->points.reserve(values.length() / 3);
						for (int v = 0; v < values.length(); v += 3) {
							this->points.push(FVector3D(string_toDouble(values[v]), string_toDouble(values[v+1]), string_toDouble(values[v+2])));
						}
					}
				} else if (string_caseInsensitiveMatch(key, U"TriangleIndices")) {
					List<String> values = string_split(value, U',');
					if (values.length() % 3 != 0) {
						throwError("TriangleIndices contained ", values.length(), " values, which is not evenly divisible by three!");
					} else {
						this->triangleIndices.clear();
						this->triangleIndices.reserve(values.length());
						for (int v = 0; v < values.length(); v++) {
							this->triangleIndices.push(string_toInteger(values[v]));
						}
					}
				} else {
					printText("Unrecognized key \"", key, "\" in sprite configuration file.\n");
				}
			} else {
				printText("Unrecognized block \"", block, "\" in sprite configuration file.\n");
			}
		});
	}
	// Add model as a persistent shadow caster in the sprite configuration
	void appendShadow(const Model& model) {
		points.reserve(this->points.length() + model_getNumberOfPoints(model));
		for (int p = 0; p < model_getNumberOfPoints(model); p++) {
			this->points.push(model_getPoint(model, p));
		}
		for (int part = 0; part < model_getNumberOfParts(model); part++) {
			for (int poly = 0; poly < model_getNumberOfPolygons(model, part); poly++) {
				int vertexCount = model_getPolygonVertexCount(model, part, poly);
				int vertA = 0;
				int indexA = model_getVertexPointIndex(model, part, poly, vertA);
				for (int vertB = 1; vertB < vertexCount - 1; vertB++) {
					int vertC = vertB + 1;
					int indexB = model_getVertexPointIndex(model, part, poly, vertB);
					int indexC = model_getVertexPointIndex(model, part, poly, vertC);
					triangleIndices.push(indexA); triangleIndices.push(indexB); triangleIndices.push(indexC);
				}
			}
		}
	}
	String toIni() {
		// General information
		String result = string_combine(
			U"; Sprite configuration file\n",
			U"CenterX=", this->centerX, "\n",
			U"CenterY=", this->centerY, "\n",
			U"FrameRows=", this->frameRows, "\n",
			U"PropertyColumns=", this->propertyColumns, "\n",
			U"MinBound=", this->minBound, "\n",
			U"MaxBound=", this->maxBound, "\n"
		);
		// Low-resolution 3D shape
		if (this->points.length() > 0) {
			string_append(result, U"Points=");
			for (int p = 0; p < this->points.length(); p++) {
				if (p > 0) {
					string_append(result, U", ");
				}
				string_append(result, this->points[p]);
			}
			string_append(result, U"\n");
			string_append(result, U"TriangleIndices=");
			for (int i = 0; i < this->triangleIndices.length(); i+=3) {
				if (i > 0) {
					string_append(result, U", ");
				}
				string_append(result, this->triangleIndices[i], U",", this->triangleIndices[i+1], U",", this->triangleIndices[i+2]);
			}
			string_append(result, U"\n");
		}
		return result;
	}
};

static ImageF32 scaleHeightImage(const ImageRgbaU8& heightImage, float minHeight, float maxHeight, const ImageRgbaU8& colorImage) {
	float scale = (maxHeight - minHeight) / 255.0f;
	float offset = minHeight;
	int width = image_getWidth(heightImage);
	int height = image_getHeight(heightImage);
	ImageF32 result = image_create_F32(width, height);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float value = image_readPixel_clamp(heightImage, x, y).red;
			if (image_readPixel_clamp(colorImage, x, y).alpha > 127) {
				image_writePixel(result, x, y, (value * scale) + offset);
			} else {
				image_writePixel(result, x, y, -std::numeric_limits<float>::infinity());
			}
		}
	}
	return result;
}

struct SpriteFrame {
	IVector2D centerPoint;
	ImageRgbaU8 colorImage; // (Red, Green, Blue, _)
	ImageRgbaU8 normalImage; // (NormalX, NormalY, NormalZ, _)
	ImageF32 heightImage;
	SpriteFrame(const IVector2D& centerPoint, const ImageRgbaU8& colorImage, const ImageRgbaU8& normalImage, const ImageF32& heightImage)
	: centerPoint(centerPoint), colorImage(colorImage), normalImage(normalImage), heightImage(heightImage) {}
};

struct SpriteType {
public:
	String name;
	IVector3D minBoundMini, maxBoundMini;
	List<SpriteFrame> frames;
	// TODO: Compress the data using a shadow-only model type of only positions and triangle indices in a single part.
	//       The shadow model will have its own rendering method excluding the color target.
	//       Shadow rendering can be a lot simpler by not calculating any vertex weights
	//         just interpolate the depth using addition, compare to the old value and write the new depth value.
	Model shadowModel;
public:
	// folderPath should end with a path separator
	SpriteType(const String& folderPath, const String& name) : name(name) {
		// Load the image atlas
		ImageRgbaU8 loadedAtlas = image_load_RgbaU8(string_combine(folderPath, name, U".png"));
		// Load the settings
		const SpriteConfig configuration = SpriteConfig(string_load(string_combine(folderPath, name, U".ini")));
		this->minBoundMini = IVector3D(
		  floor(configuration.minBound.x * ortho_miniUnitsPerTile),
		  floor(configuration.minBound.y * ortho_miniUnitsPerTile),
		  floor(configuration.minBound.z * ortho_miniUnitsPerTile)
		);
		this->maxBoundMini = IVector3D(
		  ceil(configuration.maxBound.x * ortho_miniUnitsPerTile),
		  ceil(configuration.maxBound.y * ortho_miniUnitsPerTile),
		  ceil(configuration.maxBound.z * ortho_miniUnitsPerTile)
		);
		int width = image_getWidth(loadedAtlas) / configuration.propertyColumns;
		int height = image_getHeight(loadedAtlas) / configuration.frameRows;
		for (int a = 0; a < configuration.frameRows; a++) {
			ImageRgbaU8 colorImage = image_getSubImage(loadedAtlas, IRect(0, a * height, width, height));
			ImageRgbaU8 heightImage = image_getSubImage(loadedAtlas, IRect(width, a * height, width, height));
			ImageRgbaU8 normalImage = image_getSubImage(loadedAtlas, IRect(width * 2, a * height, width, height));
			ImageF32 scaledHeightImage = scaleHeightImage(heightImage, configuration.minBound.y, configuration.maxBound.y, colorImage);
			this->frames.pushConstruct(IVector2D(configuration.centerX, configuration.centerY), colorImage, normalImage, scaledHeightImage);
		}
		// Create a model for rendering shadows
		if (configuration.points.length() > 0) {
			this->shadowModel = model_create();
			for (int p = 0; p < configuration.points.length(); p++) {
				model_addPoint(this->shadowModel, configuration.points[p]);
			}
			model_addEmptyPart(this->shadowModel, U"Shadow");
			for (int t = 0; t < configuration.triangleIndices.length(); t+=3) {
				model_addTriangle(this->shadowModel, 0, configuration.triangleIndices[t], configuration.triangleIndices[t+1], configuration.triangleIndices[t+2]);
			}
		}
	}
public:
	// TODO: Force frame count to a power of two or replace modulo with look-up tables in sprite configurations.
	int getFrameIndex(Direction direction) {
		const int frameFromDir[dir360] = {4, 1, 5, 2, 6, 3, 7, 0};
		return frameFromDir[correctDirection(direction)] % this->frames.length();
	}
};

struct DenseTriangle {
public:
	FVector3D colorA, colorB, colorC, posA, posB, posC, normalA, normalB, normalC;
public:
	DenseTriangle() {}
	DenseTriangle(
	  const FVector3D& colorA, const FVector3D& colorB, const FVector3D& colorC,
	  const FVector3D& posA, const FVector3D& posB, const FVector3D& posC,
	  const FVector3D& normalA, const FVector3D& normalB, const FVector3D& normalC)
	  : colorA(colorA), colorB(colorB), colorC(colorC),
	    posA(posA), posB(posB), posC(posC),
	    normalA(normalA), normalB(normalB), normalC(normalC) {}
};
// The raw format for dense models using vertex colors instead of textures
// Due to the high number of triangles, indexing positions would cause a lot of cache misses
struct DenseModelImpl {
public:
	Array<DenseTriangle> triangles;
	FVector3D minBound, maxBound;
public:
	// Optimize an existing model
	DenseModelImpl(const Model& original);
};

struct ModelType {
public:
	String name;
	DenseModel visibleModel;
	Model shadowModel;
public:
	// folderPath should end with a path separator
	ModelType(const String& folderPath, const String& visibleModelName, const String& shadowModelName)
	: name(visibleModelName) {
		int64_t dotIndex = string_findFirst(visibleModelName, U'.');
		if (dotIndex > -1) {
			name = string_before(visibleModelName, dotIndex);
		} else {
			name = visibleModelName;
		}
		this->visibleModel = DenseModel_create(importer_loadModel(folderPath + visibleModelName, true, Transform3D()));
		this->shadowModel = importer_loadModel(folderPath + shadowModelName, true, Transform3D());
	}
	ModelType(const DenseModel& visibleModel, const Model& shadowModel)
	: visibleModel(visibleModel), shadowModel(shadowModel) {}
};

// Global list of all sprite types ever loaded
List<SpriteType> spriteTypes;
int spriteWorld_loadSpriteTypeFromFile(const String& folderPath, const String& spriteName) {
	spriteTypes.pushConstruct(folderPath, spriteName);
	return spriteTypes.length() - 1;
}
int spriteWorld_getSpriteTypeCount() {
	return spriteTypes.length();
}
String spriteWorld_getSpriteTypeName(int index) {
	return spriteTypes[index].name;
}

// Global list of all model types ever loaded
List<ModelType> modelTypes;
int spriteWorld_loadModelTypeFromFile(const String& folderPath, const String& visibleModelName, const String& shadowModelName) {
	modelTypes.pushConstruct(folderPath, visibleModelName, shadowModelName);
	return modelTypes.length() - 1;
}
int spriteWorld_getModelTypeCount() {
	return modelTypes.length();
}
String spriteWorld_getModelTypeName(int index) {
	return modelTypes[index].name;
}

static int getSpriteFrameIndex(const SpriteInstance& sprite, OrthoView view) {
	return spriteTypes[sprite.typeIndex].getFrameIndex(view.worldDirection + sprite.direction);
}

// Returns a 2D bounding box of affected target pixels
static IRect drawSprite(const SpriteInstance& sprite, const OrthoView& ortho, const IVector2D& worldCenter, ImageF32 targetHeight, ImageRgbaU8 targetColor, ImageRgbaU8 targetNormal) {
	int frameIndex = getSpriteFrameIndex(sprite, ortho);
	const SpriteFrame* frame = &spriteTypes[sprite.typeIndex].frames[frameIndex];
	IVector2D screenSpace = ortho.miniTilePositionToScreenPixel(sprite.location, worldCenter) - frame->centerPoint;
	float heightOffset = sprite.location.y * ortho_tilesPerMiniUnit;
	draw_higher(targetHeight, frame->heightImage, targetColor, frame->colorImage, targetNormal, frame->normalImage, screenSpace.x, screenSpace.y, heightOffset);
	return IRect(screenSpace.x, screenSpace.y, image_getWidth(frame->colorImage), image_getHeight(frame->colorImage));
}

static IRect drawModel(const ModelInstance& instance, const OrthoView& ortho, const IVector2D& worldCenter, ImageF32 targetHeight, ImageRgbaU8 targetColor, ImageRgbaU8 targetNormal) {
	return renderDenseModel<false>(modelTypes[instance.typeIndex].visibleModel, ortho, targetHeight, targetColor, targetNormal, FVector2D(worldCenter.x, worldCenter.y), instance.location);
}

// The camera transform for each direction
FMatrix3x3 ShadowCubeMapSides[6] = {
	FMatrix3x3::makeAxisSystem(FVector3D( 1.0f, 0.0f, 0.0f), FVector3D(0.0f, 1.0f, 0.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D(-1.0f, 0.0f, 0.0f), FVector3D(0.0f, 1.0f, 0.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D( 0.0f, 1.0f, 0.0f), FVector3D(0.0f, 0.0f, 1.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D( 0.0f,-1.0f, 0.0f), FVector3D(0.0f, 0.0f, 1.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D( 0.0f, 0.0f, 1.0f), FVector3D(0.0f, 1.0f, 0.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D( 0.0f, 0.0f,-1.0f), FVector3D(0.0f, 1.0f, 0.0f))
};

// TODO: Move to the ortho API using a safe getter in modulo
FMatrix3x3 spriteDirections[8] = {
	FMatrix3x3::makeAxisSystem(FVector3D( 0.0f, 0.0f, 1.0f), FVector3D(0.0f, 1.0f, 0.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D( 1.0f, 0.0f, 1.0f), FVector3D(0.0f, 1.0f, 0.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D( 1.0f, 0.0f, 0.0f), FVector3D(0.0f, 1.0f, 0.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D( 1.0f, 0.0f,-1.0f), FVector3D(0.0f, 1.0f, 0.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D( 0.0f, 0.0f,-1.0f), FVector3D(0.0f, 1.0f, 0.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D(-1.0f, 0.0f,-1.0f), FVector3D(0.0f, 1.0f, 0.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D(-1.0f, 0.0f, 0.0f), FVector3D(0.0f, 1.0f, 0.0f)),
	FMatrix3x3::makeAxisSystem(FVector3D(-1.0f, 0.0f, 1.0f), FVector3D(0.0f, 1.0f, 0.0f))
};

struct CubeMapF32 {
	int resolution;           // The width and height of each shadow depth image or 0 if no shadows are casted
	AlignedImageF32 cubeMap;  // A vertical sequence of reciprocal depth images for the six sides of the cube
	ImageF32 cubeMapViews[6]; // Sub-images sharing their allocations with cubeMap as sub-images
	explicit CubeMapF32(int resolution) : resolution(resolution) {
		this->cubeMap = image_create_F32(resolution, resolution * 6);
		for (int s = 0; s < 6; s++) {
			this->cubeMapViews[s] = image_getSubImage(this->cubeMap, IRect(0, s * resolution, resolution, resolution));
		}
	}
	void clear() {
		image_fill(this->cubeMap, 0.0f);
	}
};

class PointLight {
public:
	FVector3D position; // The world-space center in tile units
	float radius;       // The light radius in tile units
	float intensity;    // The color's brightness multiplier (using float to allow smooth fading)
	ColorRgbI32 color;  // The color of the light (using integers to detect when the color is uniform)
	bool shadowCasting; // Casting shadows when enabled
public:
	PointLight(FVector3D position, float radius, float intensity, ColorRgbI32 color, bool shadowCasting)
	: position(position), radius(radius), intensity(intensity), color(color), shadowCasting(shadowCasting) {}
public:
	void renderModelShadow(CubeMapF32& shadowTarget, const ModelInstance& modelInstance, const FMatrix3x3& normalToWorld) const {
		Model model = modelTypes[modelInstance.typeIndex].shadowModel;
		if (model_exists(model)) {
			// Place the model relative to the light source's position, to make rendering in light-space easier
			Transform3D modelToWorldTransform = modelInstance.location;
			modelToWorldTransform.position = modelToWorldTransform.position - this->position;
			for (int s = 0; s < 6; s++) {
				Camera camera = Camera::createPerspective(Transform3D(FVector3D(), ShadowCubeMapSides[s] * normalToWorld), shadowTarget.resolution, shadowTarget.resolution);
				model_renderDepth(model, modelToWorldTransform, shadowTarget.cubeMapViews[s], camera);
			}
		}
	}
	void renderSpriteShadow(CubeMapF32& shadowTarget, const SpriteInstance& spriteInstance, const FMatrix3x3& normalToWorld) const {
		if (spriteInstance.shadowCasting) {
			Model model = spriteTypes[spriteInstance.typeIndex].shadowModel;
			if (model_exists(model)) {
				// Place the model relative to the light source's position, to make rendering in light-space easier
				Transform3D modelToWorldTransform = Transform3D(ortho_miniToFloatingTile(spriteInstance.location) - this->position, spriteDirections[spriteInstance.direction]);
				for (int s = 0; s < 6; s++) {
					Camera camera = Camera::createPerspective(Transform3D(FVector3D(), ShadowCubeMapSides[s] * normalToWorld), shadowTarget.resolution, shadowTarget.resolution);
					model_renderDepth(model, modelToWorldTransform, shadowTarget.cubeMapViews[s], camera);
				}
			}
		}
	}
	// Render shadows from passive sprites
	void renderPassiveShadows(CubeMapF32& shadowTarget, Octree<SpriteInstance>& sprites, const FMatrix3x3& normalToWorld) const {
		IVector3D center = ortho_floatingTileToMini(this->position);
		IVector3D minBound = center - ortho_floatingTileToMini(radius);
		IVector3D maxBound = center + ortho_floatingTileToMini(radius);
		sprites.map(minBound, maxBound, [this, shadowTarget, normalToWorld](SpriteInstance& sprite, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound) mutable {
			this->renderSpriteShadow(shadowTarget, sprite, normalToWorld);
			return LeafAction::None;
		});
	}
	// Render shadows from passive models
	void renderPassiveShadows(CubeMapF32& shadowTarget, Octree<ModelInstance>& models, const FMatrix3x3& normalToWorld) const {
		IVector3D center = ortho_floatingTileToMini(this->position);
		IVector3D minBound = center - ortho_floatingTileToMini(radius);
		IVector3D maxBound = center + ortho_floatingTileToMini(radius);
		models.map(minBound, maxBound, [this, shadowTarget, normalToWorld](ModelInstance& model, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound) mutable {
			this->renderModelShadow(shadowTarget, model, normalToWorld);
			return LeafAction::None;
		});
	}
public:
	void illuminate(const OrthoView& camera, const IVector2D& worldCenter, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const AlignedImageF32& heightBuffer, const CubeMapF32& shadowSource) const {
		if (this->shadowCasting) {
			addPointLight(camera, worldCenter, lightBuffer, normalBuffer, heightBuffer, this->position, this->radius, this->intensity, this->color, shadowSource.cubeMap);
		} else {
			addPointLight(camera, worldCenter, lightBuffer, normalBuffer, heightBuffer, this->position, this->radius, this->intensity, this->color);
		}
	}
};

class DirectedLight {
public:
	FVector3D direction;    // The world-space direction
	float intensity;        // The color's brightness multiplier (using float to allow smooth fading)
	ColorRgbI32 color;      // The color of the light (using integers to detect when the color is uniform)
public:
	DirectedLight(FVector3D direction, float intensity, ColorRgbI32 color)
	: direction(direction), intensity(intensity), color(color) {}
public:
	void illuminate(const OrthoView& camera, const IVector2D& worldCenter, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, bool overwrite = false) const {
		if (overwrite) {
			setDirectedLight(camera, lightBuffer, normalBuffer, this->direction, this->intensity, this->color);
		} else {
			addDirectedLight(camera, lightBuffer, normalBuffer, this->direction, this->intensity, this->color);
		}
	}
};

IVector3D getBoxCorner(const IVector3D& minBound, const IVector3D& maxBound, int cornerIndex) {
	assert(cornerIndex >= 0 && cornerIndex < 8);
	return IVector3D(
	  ((uint32_t)cornerIndex & 1u) ? maxBound.x : minBound.x,
	  ((uint32_t)cornerIndex & 2u) ? maxBound.y : minBound.y,
	  ((uint32_t)cornerIndex & 4u) ? maxBound.z : minBound.z
	);
}

static bool orthoCullingTest(const OrthoView& ortho, const IVector3D& minBound, const IVector3D& maxBound, const IRect& seenRegion) {
	IVector2D corners[8];
	for (int c = 0; c < 8; c++) {
		corners[c] = ortho.miniTileOffsetToScreenPixel(getBoxCorner(minBound, maxBound, c));
	}
	if (corners[0].x < seenRegion.left()
	 && corners[1].x < seenRegion.left()
	 && corners[2].x < seenRegion.left()
	 && corners[3].x < seenRegion.left()
	 && corners[4].x < seenRegion.left()
	 && corners[5].x < seenRegion.left()
	 && corners[6].x < seenRegion.left()
	 && corners[7].x < seenRegion.left()) {
		return false;
	}
	if (corners[0].x > seenRegion.right()
	 && corners[1].x > seenRegion.right()
	 && corners[2].x > seenRegion.right()
	 && corners[3].x > seenRegion.right()
	 && corners[4].x > seenRegion.right()
	 && corners[5].x > seenRegion.right()
	 && corners[6].x > seenRegion.right()
	 && corners[7].x > seenRegion.right()) {
		return false;
	}
	if (corners[0].y < seenRegion.top()
	 && corners[1].y < seenRegion.top()
	 && corners[2].y < seenRegion.top()
	 && corners[3].y < seenRegion.top()
	 && corners[4].y < seenRegion.top()
	 && corners[5].y < seenRegion.top()
	 && corners[6].y < seenRegion.top()
	 && corners[7].y < seenRegion.top()) {
		return false;
	}
	if (corners[0].y > seenRegion.bottom()
	 && corners[1].y > seenRegion.bottom()
	 && corners[2].y > seenRegion.bottom()
	 && corners[3].y > seenRegion.bottom()
	 && corners[4].y > seenRegion.bottom()
	 && corners[5].y > seenRegion.bottom()
	 && corners[6].y > seenRegion.bottom()
	 && corners[7].y > seenRegion.bottom()) {
		return false;
	}
	return true;
}

// BlockState keeps track of when the background itself needs to update from static objects being created or destroyed
enum class BlockState {
	Unused,
	Ready,
	Dirty
};
class BackgroundBlock {
public:
	static const int blockSize = 512;
	static const int maxDistance = blockSize * 2;
	IRect worldRegion;
	int cameraId = 0;
	BlockState state = BlockState::Unused;
	OrderedImageRgbaU8 diffuseBuffer;
	OrderedImageRgbaU8 normalBuffer;
	AlignedImageF32 heightBuffer;
private:
	// Pre-condition: diffuseBuffer must be cleared unless sprites cover the whole block
	void draw(Octree<SpriteInstance>& sprites, Octree<ModelInstance>& models, const OrthoView& ortho) {
		image_fill(this->normalBuffer, ColorRgbaI32(128));
		image_fill(this->heightBuffer, -std::numeric_limits<float>::max());
		OcTreeFilter orthoCullingFilter = [ortho,this](const IVector3D& minBound, const IVector3D& maxBound){
			return orthoCullingTest(ortho, minBound, maxBound, this->worldRegion);
		};
		sprites.map(orthoCullingFilter, [this, ortho](SpriteInstance& sprite, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound){
			drawSprite(sprite, ortho, -this->worldRegion.upperLeft(), this->heightBuffer, this->diffuseBuffer, this->normalBuffer);
			return LeafAction::None;
		});
		models.map(orthoCullingFilter, [this, ortho](ModelInstance& model, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound){
			drawModel(model, ortho, -this->worldRegion.upperLeft(), this->heightBuffer, this->diffuseBuffer, this->normalBuffer);
			return LeafAction::None;
		});
	}
public:
	BackgroundBlock(Octree<SpriteInstance>& sprites, Octree<ModelInstance>& models, const IRect& worldRegion, const OrthoView& ortho)
	: worldRegion(worldRegion), cameraId(ortho.id), state(BlockState::Ready),
	  diffuseBuffer(image_create_RgbaU8(blockSize, blockSize)),
	  normalBuffer(image_create_RgbaU8(blockSize, blockSize)),
	  heightBuffer(image_create_F32(blockSize, blockSize)) {
		this->draw(sprites, models, ortho);
	}
	void update(Octree<SpriteInstance>& sprites, Octree<ModelInstance>& models, const IRect& worldRegion, const OrthoView& ortho) {
		this->worldRegion = worldRegion;
		this->cameraId = ortho.id;
		image_fill(this->diffuseBuffer, ColorRgbaI32(0));
		this->draw(sprites, models, ortho);
		this->state = BlockState::Ready;
	}
	void draw(ImageRgbaU8& diffuseTarget, ImageRgbaU8& normalTarget, ImageF32& heightTarget, const IRect& seenRegion) const {
		if (this->state != BlockState::Unused) {
			int left = this->worldRegion.left() - seenRegion.left();
			int top = this->worldRegion.top() - seenRegion.top();
			draw_copy(diffuseTarget, this->diffuseBuffer, left, top);
			draw_copy(normalTarget, this->normalBuffer, left, top);
			draw_copy(heightTarget, this->heightBuffer, left, top);
		}
	}
	void recycle() {
		//printText("Recycle block at ", this->worldRegion, "\n");
		this->state = BlockState::Unused;
		this->worldRegion = IRect();
		this->cameraId = -1;
	}
};

// TODO: A way to delete passive sprites and models using search criterias for bounding box and leaf content using a boolean lambda
class SpriteWorldImpl {
public:
	// World
	OrthoSystem ortho;
	// Having one passive and one active collection per member type allow packing elements tighter to reduce cache misses.
	//   It also allow executing rendering sorted by which code has to be fetched into the instruction cache.
	// Sprites that rarely change and can be stored in a background image.
	Octree<SpriteInstance> passiveSprites;
	// Rarely moved models can be rendered using free rotation and uniform scaling to the background image.
	Octree<ModelInstance> passiveModels;
	// Temporary things are deleted when spriteWorld_clearTemporary is called
	List<SpriteInstance> temporarySprites;
	List<ModelInstance> temporaryModels;
	List<PointLight> temporaryPointLights;
	List<DirectedLight> temporaryDirectedLights;
	// View
	int cameraIndex = 0;
	IVector3D cameraLocation;
	// Deferred rendering
	OrderedImageRgbaU8 diffuseBuffer;
	OrderedImageRgbaU8 normalBuffer;
	AlignedImageF32 heightBuffer;
	OrderedImageRgbaU8 lightBuffer;
	// Passive background
	// TODO: How can split-screen use multiple cameras without duplicate blocks or deleting the other camera's blocks by distance?
	List<BackgroundBlock> backgroundBlocks;
	// These dirty rectangles keep track of when the background has to be redrawn to the screen after having drawn a dynamic sprite, moved the camera or changed static geometry
	DirtyRectangles dirtyBackground;
private:
	// Reused buffers
	int shadowResolution;
	CubeMapF32 temporaryShadowMap;
public:
	SpriteWorldImpl(const OrthoSystem &ortho, int shadowResolution)
	: ortho(ortho), passiveSprites(ortho_miniUnitsPerTile * 64), passiveModels(ortho_miniUnitsPerTile * 64), shadowResolution(shadowResolution), temporaryShadowMap(shadowResolution) {}
public:
	void updateBlockAt(const IRect& blockRegion, const IRect& seenRegion) {
		int unusedBlockIndex = -1;
		// Find an existing block
		for (int b = 0; b < this->backgroundBlocks.length(); b++) {
			BackgroundBlock* currentBlockPtr = &this->backgroundBlocks[b];
			if (currentBlockPtr->state != BlockState::Unused) {
				// Check direction
				if (currentBlockPtr->cameraId == this->ortho.view[this->cameraIndex].id) {
					// Check location
					if (currentBlockPtr->worldRegion.left() == blockRegion.left() && currentBlockPtr->worldRegion.top() == blockRegion.top()) {
						// Update if needed
						if (currentBlockPtr->state == BlockState::Dirty) {
							currentBlockPtr->update(this->passiveSprites, this->passiveModels, blockRegion, this->ortho.view[this->cameraIndex]);
						}
						// Use the block
						return;
					} else {
						// See if the block is too far from the camera
						if (currentBlockPtr->worldRegion.right() < seenRegion.left() - BackgroundBlock::maxDistance
						 || currentBlockPtr->worldRegion.left() > seenRegion.right() + BackgroundBlock::maxDistance
						 || currentBlockPtr->worldRegion.bottom() < seenRegion.top() - BackgroundBlock::maxDistance
					 	 || currentBlockPtr->worldRegion.top() > seenRegion.bottom() + BackgroundBlock::maxDistance) {
							// Recycle because it's too far away
							currentBlockPtr->recycle();
							unusedBlockIndex = b;
						}
					}
				} else{
					// Recycle directly when another camera angle is used
					currentBlockPtr->recycle();
					unusedBlockIndex = b;
				}
			} else {
				unusedBlockIndex = b;
			}
		}
		// If none of them matched, we should've passed by any unused block already
		if (unusedBlockIndex > -1) {
			// We have a block to reuse
			this->backgroundBlocks[unusedBlockIndex].update(this->passiveSprites, this->passiveModels, blockRegion, this->ortho.view[this->cameraIndex]);
		} else {
			// Create a new block
			this->backgroundBlocks.pushConstruct(this->passiveSprites, this->passiveModels, blockRegion, this->ortho.view[this->cameraIndex]);
		}
	}
	void invalidateBlockAt(int left, int top) {
		// Find an existing block
		for (int b = 0; b < this->backgroundBlocks.length(); b++) {
			BackgroundBlock* currentBlockPtr = &this->backgroundBlocks[b];
			// Assuming that alternative camera angles will be removed when drawing next time
			if (currentBlockPtr->state == BlockState::Ready
			 && currentBlockPtr->worldRegion.left() == left
			 && currentBlockPtr->worldRegion.top() == top) {
				// Make dirty to force an update
				currentBlockPtr->state = BlockState::Dirty;
			}
		}
	}
	// Make sure that each pixel in seenRegion is occupied by an updated background block
	void updateBlocks(const IRect& seenRegion) {
		// Round inclusive pixel indices down to containing blocks and iterate over them in strides along x and y
		int64_t roundedLeft = roundDown(seenRegion.left(), BackgroundBlock::blockSize);
		int64_t roundedTop = roundDown(seenRegion.top(), BackgroundBlock::blockSize);
		int64_t roundedRight = roundDown(seenRegion.right() - 1, BackgroundBlock::blockSize);
		int64_t roundedBottom = roundDown(seenRegion.bottom() - 1, BackgroundBlock::blockSize);
		for (int64_t y = roundedTop; y <= roundedBottom; y += BackgroundBlock::blockSize) {
			for (int64_t x = roundedLeft; x <= roundedRight; x += BackgroundBlock::blockSize) {
				// Make sure that a block is allocated and pre-drawn at this location
				this->updateBlockAt(IRect(x, y, BackgroundBlock::blockSize, BackgroundBlock::blockSize), seenRegion);
			}
		}
	}
	void drawDeferred(OrderedImageRgbaU8& diffuseTarget, OrderedImageRgbaU8& normalTarget, AlignedImageF32& heightTarget, const IRect& seenRegion) {
		// Check image dimensions
		assert(image_getWidth(diffuseTarget) == seenRegion.width() && image_getHeight(diffuseTarget) == seenRegion.height());
		assert(image_getWidth(normalTarget) == seenRegion.width() && image_getHeight(normalTarget) == seenRegion.height());
		assert(image_getWidth(heightTarget) == seenRegion.width() && image_getHeight(heightTarget) == seenRegion.height());
		this->dirtyBackground.setTargetResolution(seenRegion.width(), seenRegion.height());
		// Draw passive sprites to blocks
		this->updateBlocks(seenRegion);

		// Draw background blocks to the target images
		for (int b = 0; b < this->backgroundBlocks.length(); b++) {
			#ifdef DIRTY_RECTANGLE_OPTIMIZATION
				// Optimized version
				for (int64_t r = 0; r < this->dirtyBackground.getRectangleCount(); r++) {
					IRect screenClip = this->dirtyBackground.getRectangle(r);
					IRect worldClip = screenClip + seenRegion.upperLeft();
					ImageRgbaU8 clippedDiffuseTarget = image_getSubImage(diffuseTarget, screenClip);
					ImageRgbaU8 clippedNormalTarget = image_getSubImage(normalTarget, screenClip);
					ImageF32 clippedHeightTarget = image_getSubImage(heightTarget, screenClip);
					this->backgroundBlocks[b].draw(clippedDiffuseTarget, clippedNormalTarget, clippedHeightTarget, worldClip);
				}
			#else
				// Reference implementation
				this->backgroundBlocks[b].draw(diffuseTarget, normalTarget, heightTarget, seenRegion);
			#endif
		}

		// Reset dirty rectangles so that active sprites may record changes
		this->dirtyBackground.noneDirty();
		// Draw active sprites to the targets
		for (int s = 0; s < this->temporarySprites.length(); s++) {
			IRect drawnRegion = drawSprite(this->temporarySprites[s], this->ortho.view[this->cameraIndex], -seenRegion.upperLeft(), heightTarget, diffuseTarget, normalTarget);
			this->dirtyBackground.makeRegionDirty(drawnRegion);
		}
		for (int s = 0; s < this->temporaryModels.length(); s++) {
			IRect drawnRegion = drawModel(this->temporaryModels[s], this->ortho.view[this->cameraIndex], -seenRegion.upperLeft(), heightTarget, diffuseTarget, normalTarget);
			this->dirtyBackground.makeRegionDirty(drawnRegion);
		}
	}
public:
	// modifiedRegion is given in pixels relative to the world origin for the current camera angle
	void updatePassiveRegion(const IRect& modifiedRegion) {
		int64_t roundedLeft = roundDown(modifiedRegion.left(), BackgroundBlock::blockSize);
		int64_t roundedTop = roundDown(modifiedRegion.top(), BackgroundBlock::blockSize);
		int64_t roundedRight = roundDown(modifiedRegion.right() - 1, BackgroundBlock::blockSize);
		int64_t roundedBottom = roundDown(modifiedRegion.bottom() - 1, BackgroundBlock::blockSize);
		for (int64_t y = roundedTop; y <= roundedBottom; y += BackgroundBlock::blockSize) {
			for (int64_t x = roundedLeft; x <= roundedRight; x += BackgroundBlock::blockSize) {
				// Make sure that a block is allocated and pre-drawn at this location
				this->invalidateBlockAt(x, y);
			}
		}
		// Redrawing the whole background to the screen is very cheap using memcpy, so no need to optimize this rare event
		this->dirtyBackground.allDirty();
	}
	IVector2D findWorldCenter(const AlignedImageRgbaU8& colorTarget) const {
		return IVector2D(image_getWidth(colorTarget) / 2, image_getHeight(colorTarget) / 2) - this->ortho.miniTileOffsetToScreenPixel(this->cameraLocation, this->cameraIndex);
	}
	void draw(AlignedImageRgbaU8& colorTarget) {
		double startTime;

		IVector2D worldCenter = this->findWorldCenter(colorTarget);

		// Resize when the window has resized or the buffers haven't been allocated before
		int width = image_getWidth(colorTarget);
		int height = image_getHeight(colorTarget);
		if (image_getWidth(this->diffuseBuffer) != width || image_getHeight(this->diffuseBuffer) != height) {
			this->diffuseBuffer = image_create_RgbaU8(width, height);
			this->normalBuffer = image_create_RgbaU8(width, height);
			this->lightBuffer = image_create_RgbaU8(width, height);
			this->heightBuffer = image_create_F32(width, height);
		}

		IRect worldRegion = IRect(-worldCenter.x, -worldCenter.y, width, height);
		startTime = time_getSeconds();
			this->drawDeferred(this->diffuseBuffer, this->normalBuffer, this->heightBuffer, worldRegion);
		debugText("Draw deferred: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");

		// Illuminate using directed lights
		if (this->temporaryDirectedLights.length() > 0) {
			startTime = time_getSeconds();
				// Overwriting any light from the previous frame
				for (int p = 0; p < this->temporaryDirectedLights.length(); p++) {
					this->temporaryDirectedLights[p].illuminate(this->ortho.view[this->cameraIndex], worldCenter, this->lightBuffer, this->normalBuffer, p == 0);
				}
			debugText("Sun light: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");
		} else {
			startTime = time_getSeconds();
				image_fill(this->lightBuffer, ColorRgbaI32(0)); // Set light to black
			debugText("Clear light: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");
		}

		// Illuminate using point lights
		for (int p = 0; p < this->temporaryPointLights.length(); p++) {
			PointLight *currentLight = &this->temporaryPointLights[p];
			if (currentLight->shadowCasting) {
				startTime = time_getSeconds();
				this->temporaryShadowMap.clear();
				// Shadows from background sprites
				currentLight->renderPassiveShadows(this->temporaryShadowMap, this->passiveSprites, ortho.view[this->cameraIndex].normalToWorldSpace);
				currentLight->renderPassiveShadows(this->temporaryShadowMap, this->passiveModels, ortho.view[this->cameraIndex].normalToWorldSpace);
				// Shadows from temporary sprites
				for (int s = 0; s < this->temporarySprites.length(); s++) {
					currentLight->renderSpriteShadow(this->temporaryShadowMap, this->temporarySprites[s], ortho.view[this->cameraIndex].normalToWorldSpace);
				}
				// Shadows from temporary models
				for (int s = 0; s < this->temporaryModels.length(); s++) {
					currentLight->renderModelShadow(this->temporaryShadowMap, this->temporaryModels[s], ortho.view[this->cameraIndex].normalToWorldSpace);
				}
				debugText("Cast point-light shadows: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");
			}
			startTime = time_getSeconds();
			currentLight->illuminate(this->ortho.view[this->cameraIndex], worldCenter, this->lightBuffer, this->normalBuffer, this->heightBuffer, this->temporaryShadowMap);
			debugText("Illuminate from point-light: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");
		}

		// Draw the final image to the target by multiplying diffuse with light
		startTime = time_getSeconds();
			blendLight(colorTarget, this->diffuseBuffer, this->lightBuffer);
		debugText("Blend light: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");
	}
};

SpriteWorld spriteWorld_create(OrthoSystem ortho, int shadowResolution) {
	return std::make_shared<SpriteWorldImpl>(ortho, shadowResolution);
}

#define MUST_EXIST(OBJECT, METHOD) if (OBJECT.get() == nullptr) { throwError("The " #OBJECT " handle was null in " #METHOD "\n"); }

// Get the eight corners of an axis-aligned bounding box
static void getCorners(const FVector3D& minBound, const FVector3D& maxBound, FVector3D* resultCorners) {
	resultCorners[0] = FVector3D(minBound.x, minBound.y, minBound.z);
	resultCorners[1] = FVector3D(maxBound.x, minBound.y, minBound.z);
	resultCorners[2] = FVector3D(minBound.x, maxBound.y, minBound.z);
	resultCorners[3] = FVector3D(maxBound.x, maxBound.y, minBound.z);
	resultCorners[4] = FVector3D(minBound.x, minBound.y, maxBound.z);
	resultCorners[5] = FVector3D(maxBound.x, minBound.y, maxBound.z);
	resultCorners[6] = FVector3D(minBound.x, maxBound.y, maxBound.z);
	resultCorners[7] = FVector3D(maxBound.x, maxBound.y, maxBound.z);
}

// Transform the eight corners of an axis-aligned bounding box
static void transformCorners(const FVector3D& minBound, const FVector3D& maxBound, const Transform3D& transform, FVector3D* resultCorners) {
	resultCorners[0] = transform.transformPoint(FVector3D(minBound.x, minBound.y, minBound.z));
	resultCorners[1] = transform.transformPoint(FVector3D(maxBound.x, minBound.y, minBound.z));
	resultCorners[2] = transform.transformPoint(FVector3D(minBound.x, maxBound.y, minBound.z));
	resultCorners[3] = transform.transformPoint(FVector3D(maxBound.x, maxBound.y, minBound.z));
	resultCorners[4] = transform.transformPoint(FVector3D(minBound.x, minBound.y, maxBound.z));
	resultCorners[5] = transform.transformPoint(FVector3D(maxBound.x, minBound.y, maxBound.z));
	resultCorners[6] = transform.transformPoint(FVector3D(minBound.x, maxBound.y, maxBound.z));
	resultCorners[7] = transform.transformPoint(FVector3D(maxBound.x, maxBound.y, maxBound.z));
}

// References:
//   world contains the camera system for convenience
// Input:
//   transform tells how the local bounds are transformed into mini-tile world-space
//   localMinBound and localMaxBound is the local mini-tile bound relative to the given origin
// Output:
//   worldMinBound and worldMaxBound is the bound in mini-tile coordinates relative to world origin
static void get3DBounds(
  SpriteWorld& world, const Transform3D& transform, const FVector3D& localMinBound, const FVector3D& localMaxBound, IVector3D& worldMinBound, IVector3D& worldMaxBound) {
	// Transform from local to global coordinates
	FVector3D transformedCorners[8];
	transformCorners(localMinBound, localMaxBound, transform, transformedCorners);
	// Initialize 3D bound to the center point so that tree branches expand bounds to include the origins of every leaf
	//   This make searches a lot easier for off-centered sprites and models by belonging to a coordinate independent of the design
	worldMinBound = FVector3DToIVector3D(transform.position);
	worldMaxBound = FVector3DToIVector3D(transform.position);
	for (int c = 0; c < 8; c++) {
		FVector3D miniSpaceCorner = transformedCorners[c];
		replaceWithSmaller(worldMinBound.x, (int32_t)floor(miniSpaceCorner.x));
		replaceWithSmaller(worldMinBound.y, (int32_t)floor(miniSpaceCorner.y));
		replaceWithSmaller(worldMinBound.z, (int32_t)floor(miniSpaceCorner.z));
		replaceWithLarger(worldMaxBound.x, (int32_t)ceil(miniSpaceCorner.x));
		replaceWithLarger(worldMaxBound.y, (int32_t)ceil(miniSpaceCorner.y));
		replaceWithLarger(worldMaxBound.z, (int32_t)ceil(miniSpaceCorner.z));
	}
}

// References:
//   world contains the camera system for convenience
// Input:
//   worldMinBound and worldMaxBound is the bound in mini-tile coordinates relative to world origin
// Output:
//   globalPixelMinBound and globalPixelMaxBound is the bound in pixel coordinates relative to world origin
static void getScreenBounds(SpriteWorld& world, const IVector3D& worldMinBound, const IVector3D& worldMaxBound, IVector2D& globalPixelMinBound, IVector2D& globalPixelMaxBound) {
	// Create a transform for global pixels
	Transform3D worldToGlobalPixels = combineWorldToScreenTransform(world->ortho.view[world->cameraIndex].worldSpaceToScreenDepth, FVector2D());
	FVector3D corners[8];
	getCorners(IVector3DToFVector3D(worldMinBound) * ortho_tilesPerMiniUnit, IVector3DToFVector3D(worldMaxBound) * ortho_tilesPerMiniUnit, corners);
	// Screen bound
	FVector3D firstGlobalPixelSpaceCorner = worldToGlobalPixels.transformPoint(corners[0]);
	globalPixelMinBound = IVector2D((int32_t)floor(firstGlobalPixelSpaceCorner.x), (int32_t)floor(firstGlobalPixelSpaceCorner.y));
	globalPixelMaxBound = IVector2D((int32_t)ceil(firstGlobalPixelSpaceCorner.x), (int32_t)ceil(firstGlobalPixelSpaceCorner.y));
	for (int c = 0; c < 8; c++) {
		FVector3D globalPixelSpaceCorner = worldToGlobalPixels.transformPoint(corners[c]);
		replaceWithSmaller(globalPixelMinBound.x, (int32_t)floor(globalPixelSpaceCorner.x));
		replaceWithSmaller(globalPixelMinBound.y, (int32_t)floor(globalPixelSpaceCorner.y));
		replaceWithLarger(globalPixelMaxBound.x, (int32_t)ceil(globalPixelSpaceCorner.x));
		replaceWithLarger(globalPixelMaxBound.y, (int32_t)ceil(globalPixelSpaceCorner.y));
	}
}

void spriteWorld_addBackgroundSprite(SpriteWorld& world, const SpriteInstance& sprite) {
	MUST_EXIST(world, spriteWorld_addBackgroundSprite);
	if (sprite.typeIndex < 0 || sprite.typeIndex >= spriteTypes.length()) { throwError(U"Sprite type index ", sprite.typeIndex, " is out of bound!\n"); }
	// Get world aligned 3D bounds based on the local bounding box
	IVector3D worldMinBound = sprite.location, worldMaxBound = sprite.location;
	get3DBounds(world, Transform3D(IVector3DToFVector3D(sprite.location), spriteDirections[sprite.direction]), IVector3DToFVector3D(spriteTypes[sprite.typeIndex].minBoundMini), IVector3DToFVector3D(spriteTypes[sprite.typeIndex].maxBoundMini), worldMinBound, worldMaxBound);
	// No need for getScreenBounds when the sprite has known image bounds that are more precise
	// Add the passive sprite to the octree
	world->passiveSprites.insert(sprite, sprite.location, worldMinBound, worldMaxBound);
	// Find the affected passive region and make it dirty
	int frameIndex = getSpriteFrameIndex(sprite, world->ortho.view[world->cameraIndex]);
	const SpriteFrame* frame = &spriteTypes[sprite.typeIndex].frames[frameIndex];
	IVector2D upperLeft = world->ortho.miniTilePositionToScreenPixel(sprite.location, world->cameraIndex, IVector2D()) - frame->centerPoint;
	IRect region = IRect(upperLeft.x, upperLeft.y, image_getWidth(frame->colorImage), image_getHeight(frame->colorImage));
	world->updatePassiveRegion(region);
}

void spriteWorld_addBackgroundModel(SpriteWorld& world, const ModelInstance& instance) {
	MUST_EXIST(world, spriteWorld_addBackgroundModel);
	if (instance.typeIndex < 0 || instance.typeIndex >= modelTypes.length()) { throwError(U"Model type index ", instance.typeIndex, " is out of bound!\n"); }
	// Get the origin and outer bounds
	ModelType *type = &(modelTypes[instance.typeIndex]);
	// Transform the bounds
	IVector3D origin = ortho_floatingTileToMini(instance.location.position);
	// Get world aligned 3D bounds based on the local bounding box
	IVector3D worldMinBound = origin, worldMaxBound = origin;
	IVector2D globalPixelMinBound, globalPixelMaxBound;
	Transform3D transform = Transform3D(instance.location.position * (float)ortho_miniUnitsPerTile, instance.location.transform);
	get3DBounds(world, transform, type->visibleModel->minBound * (float)ortho_miniUnitsPerTile, type->visibleModel->maxBound * (float)ortho_miniUnitsPerTile, worldMinBound, worldMaxBound);
	// Getting screen bounds from world aligned bounds will grow even more when transformed to the screen, but this won't affect already dirty regions when adding many models at the same time
	getScreenBounds(world, worldMinBound, worldMaxBound, globalPixelMinBound, globalPixelMaxBound);
	// Add the passive model to the octree
	world->passiveModels.insert(instance, origin, worldMinBound, worldMaxBound);
	// Make the affected region dirty
	world->updatePassiveRegion(IRect(globalPixelMinBound.x, globalPixelMinBound.y, globalPixelMaxBound.x - globalPixelMinBound.x, globalPixelMaxBound.y - globalPixelMinBound.y));
}

//using SpriteSelection = std::function<bool(SpriteInstance&, const IVector3D, const IVector3D, const IVector3D)>;
void spriteWorld_removeBackgroundSprites(SpriteWorld& world, const IVector3D& searchMinBound, const IVector3D& searchMaxBound, const SpriteSelection& filter) {
	world->passiveSprites.map(searchMinBound, searchMaxBound, [world, filter](SpriteInstance& sprite, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound) mutable {
		if (filter(sprite, origin, minBound, maxBound)) {
			IVector2D globalPixelMinBound, globalPixelMaxBound;
			getScreenBounds(world, minBound, maxBound, globalPixelMinBound, globalPixelMaxBound);
			world->updatePassiveRegion(IRect(globalPixelMinBound.x, globalPixelMinBound.y, globalPixelMaxBound.x - globalPixelMinBound.x, globalPixelMaxBound.y - globalPixelMinBound.y));
			return LeafAction::Erase;
		} else {
			return LeafAction::None;
		}
	});
}

void spriteWorld_removeBackgroundSprites(SpriteWorld& world, const IVector3D& searchMinBound, const IVector3D& searchMaxBound) {
	spriteWorld_removeBackgroundSprites(world, searchMinBound, searchMaxBound, [](SpriteInstance& sprite, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound) {
		return true; // Erase everything in the bound
	});
}

//using ModelSelection = std::function<bool(ModelInstance&, const IVector3D, const IVector3D, const IVector3D)>;
void spriteWorld_removeBackgroundModels(SpriteWorld& world, const IVector3D& searchMinBound, const IVector3D& searchMaxBound, const ModelSelection& filter) {
	world->passiveModels.map(searchMinBound, searchMaxBound, [world, filter](ModelInstance& model, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound) mutable {
		if (filter(model, origin, minBound, maxBound)) {
			IVector2D globalPixelMinBound, globalPixelMaxBound;
			getScreenBounds(world, minBound, maxBound, globalPixelMinBound, globalPixelMaxBound);
			world->updatePassiveRegion(IRect(globalPixelMinBound.x, globalPixelMinBound.y, globalPixelMaxBound.x - globalPixelMinBound.x, globalPixelMaxBound.y - globalPixelMinBound.y));
			return LeafAction::Erase;
		} else {
			return LeafAction::None;
		}
	});
}

void spriteWorld_removeBackgroundModels(SpriteWorld& world, const IVector3D& searchMinBound, const IVector3D& searchMaxBound) {
	spriteWorld_removeBackgroundModels(world, searchMinBound, searchMaxBound, [](ModelInstance& model, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound) {
		return true; // Erase everything in the bound
	});
}

void spriteWorld_addTemporarySprite(SpriteWorld& world, const SpriteInstance& sprite) {
	MUST_EXIST(world, spriteWorld_addTemporarySprite);
	if (sprite.typeIndex < 0 || sprite.typeIndex >= spriteTypes.length()) { throwError(U"Sprite type index ", sprite.typeIndex, " is out of bound!\n"); }
	// Add the temporary sprite
	world->temporarySprites.push(sprite);
}

void spriteWorld_addTemporaryModel(SpriteWorld& world, const ModelInstance& instance) {
	MUST_EXIST(world, spriteWorld_addTemporaryModel);
	// Add the temporary model
	world->temporaryModels.push(instance);
}

void spriteWorld_createTemporary_pointLight(SpriteWorld& world, const FVector3D position, float radius, float intensity, ColorRgbI32 color, bool shadowCasting) {
	MUST_EXIST(world, spriteWorld_createTemporary_pointLight);
	world->temporaryPointLights.pushConstruct(position, radius, intensity, color, shadowCasting);
}

void spriteWorld_createTemporary_directedLight(SpriteWorld& world, const FVector3D direction, float intensity, ColorRgbI32 color) {
	MUST_EXIST(world, spriteWorld_createTemporary_pointLight);
	world->temporaryDirectedLights.pushConstruct(direction, intensity, color);
}

void spriteWorld_clearTemporary(SpriteWorld& world) {
	MUST_EXIST(world, spriteWorld_clearTemporary);
	world->temporarySprites.clear();
	world->temporaryModels.clear();
	world->temporaryPointLights.clear();
	world->temporaryDirectedLights.clear();
}

void spriteWorld_draw(SpriteWorld& world, AlignedImageRgbaU8& colorTarget) {
	MUST_EXIST(world, spriteWorld_draw);
	world->draw(colorTarget);
}

#define BOX_LINE(INDEX_A, INDEX_B) draw_line(target, corners[INDEX_A].x, corners[INDEX_A].y, corners[INDEX_B].x, corners[INDEX_B].y, color);
void debugDrawBound(SpriteWorld& world, const IVector2D& worldCenter, AlignedImageRgbaU8& target, const ColorRgbaI32& color, const IVector3D& minBound, const IVector3D& maxBound) {
	IVector2D corners[8];
	for (int c = 0; c < 8; c++) {
		// TODO: Convert to real screen pixels using the camera offset.
		corners[c] = world->ortho.view[world->cameraIndex].miniTilePositionToScreenPixel(getBoxCorner(minBound, maxBound, c), worldCenter);
	}
	BOX_LINE(0, 1);
	BOX_LINE(2, 3);
	BOX_LINE(4, 5);
	BOX_LINE(6, 7);
	BOX_LINE(0, 2);
	BOX_LINE(1, 3);
	BOX_LINE(4, 6);
	BOX_LINE(5, 7);
	BOX_LINE(0, 4);
	BOX_LINE(1, 5);
	BOX_LINE(2, 6);
	BOX_LINE(3, 7);
}

void spriteWorld_debug_octrees(SpriteWorld& world, AlignedImageRgbaU8& colorTarget) {
	MUST_EXIST(world, spriteWorld_debug_octrees);
	IVector2D worldCenter = world->findWorldCenter(colorTarget);
	IRect seenRegion = IRect(-worldCenter.x, -worldCenter.y, image_getWidth(colorTarget), image_getHeight(colorTarget));
	OcTreeFilter orthoCullingFilter = [&world, &worldCenter, &seenRegion, &colorTarget](const IVector3D& minBound, const IVector3D& maxBound){
		debugDrawBound(world, worldCenter, colorTarget, ColorRgbaI32(100, 100, 100, 255), minBound, maxBound);
		return orthoCullingTest(world->ortho.view[world->cameraIndex], minBound, maxBound, seenRegion);
	};
	world->passiveSprites.map(orthoCullingFilter, [&world, &worldCenter, &colorTarget](SpriteInstance& sprite, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound){
		debugDrawBound(world, worldCenter, colorTarget, ColorRgbaI32(0, 255, 0, 255), minBound, maxBound);
		return LeafAction::None;
	});
	world->passiveModels.map(orthoCullingFilter, [&world, &worldCenter, &colorTarget](ModelInstance& model, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound){
		debugDrawBound(world, worldCenter, colorTarget, ColorRgbaI32(0, 0, 255, 255), minBound, maxBound);
		return LeafAction::None;
	});
}

IVector3D spriteWorld_findGroundAtPixel(SpriteWorld& world, const AlignedImageRgbaU8& colorBuffer, const IVector2D& pixelLocation) {
	MUST_EXIST(world, spriteWorld_findGroundAtPixel);
	return world->ortho.pixelToMiniPosition(pixelLocation, world->cameraIndex, world->findWorldCenter(colorBuffer));
}

void spriteWorld_setCameraLocation(SpriteWorld& world, const IVector3D miniTileLocation) {
	MUST_EXIST(world, spriteWorld_setCameraLocation);
	if (world->cameraLocation != miniTileLocation) {
		world->cameraLocation = miniTileLocation;
		world->dirtyBackground.allDirty();
	}
}

void spriteWorld_moveCameraInPixels(SpriteWorld& world, const IVector2D& pixelOffset) {
	MUST_EXIST(world, spriteWorld_moveCameraInPixels);
	if (pixelOffset.x != 0 || pixelOffset.y != 0) {
		world->cameraLocation = world->cameraLocation + world->ortho.pixelToMiniOffset(pixelOffset, world->cameraIndex);
		world->dirtyBackground.allDirty();
	}
}

AlignedImageRgbaU8 spriteWorld_getDiffuseBuffer(SpriteWorld& world) {
	MUST_EXIST(world, spriteWorld_getDiffuseBuffer);
	return world->diffuseBuffer;
}

OrderedImageRgbaU8 spriteWorld_getNormalBuffer(SpriteWorld& world) {
	MUST_EXIST(world, spriteWorld_getNormalBuffer);
	return world->normalBuffer;
}

OrderedImageRgbaU8 spriteWorld_getLightBuffer(SpriteWorld& world) {
	MUST_EXIST(world, spriteWorld_getLightBuffer);
	return world->lightBuffer;
}

AlignedImageF32 spriteWorld_getHeightBuffer(SpriteWorld& world) {
	MUST_EXIST(world, spriteWorld_getHeightBuffer);
	return world->heightBuffer;
}

int spriteWorld_getCameraDirectionIndex(SpriteWorld& world) {
	MUST_EXIST(world, spriteWorld_getCameraDirectionIndex);
	return world->cameraIndex;
}

void spriteWorld_setCameraDirectionIndex(SpriteWorld& world, int index) {
	MUST_EXIST(world, spriteWorld_setCameraDirectionIndex);
	if (index != world->cameraIndex) {
		world->cameraIndex = index;
		world->dirtyBackground.allDirty();
	}
}

static FVector3D FVector4Dto3D(FVector4D v) {
	return FVector3D(v.x, v.y, v.z);
}

static FVector2D FVector3Dto2D(FVector3D v) {
	return FVector2D(v.x, v.y);
}

// Get the pixel bound from a projected vertex point in floating pixel coordinates
static IRect boundFromVertex(const FVector3D& screenProjection) {
	return IRect((int)(screenProjection.x), (int)(screenProjection.y), 1, 1);
}

// Returns true iff the box might be seen using a pessimistic test
static IRect boundingBoxToRectangle(const FVector3D& minBound, const FVector3D& maxBound, const Transform3D& objectToScreenSpace) {
	FVector3D points[8];
	transformCorners(minBound, maxBound, objectToScreenSpace, points);
	IRect result = boundFromVertex(points[0]);
	for (int p = 1; p < 8; p++) {
		result = IRect::merge(result, boundFromVertex(points[p]));
	}
	return result;
}

static IRect getBackCulledTriangleBound(const FVector3D& a, const FVector3D& b, const FVector3D& c) {
	if (((c.x - a.x) * (b.y - a.y)) + ((c.y - a.y) * (a.x - b.x)) >= 0.0f) {
		// Back facing
		return IRect();
	} else {
		// Front facing
		int leftBound = (int)std::min(std::min(a.x, b.x), c.x);
		int topBound = (int)std::min(std::min(a.y, b.y), c.y);
		int rightBound = (int)(std::max(std::max(a.x, b.x), c.x)) + 1;
		int bottomBound = (int)(std::max(std::max(a.y, b.y), c.y)) + 1;
		return IRect(leftBound, topBound, rightBound - leftBound, bottomBound - topBound);
	}
}

static FVector3D normalFromPoints(const FVector3D& A, const FVector3D& B, const FVector3D& C) {
    return normalize(crossProduct(B - A, C - A));
}

static FVector3D getAverageNormal(const Model& model, int part, int poly) {
	int vertexCount = model_getPolygonVertexCount(model, part, poly);
	FVector3D normalSum;
	for (int t = 0; t < vertexCount - 2; t++) {
		normalSum = normalSum + normalFromPoints(
		  model_getVertexPosition(model, part, poly, 0),
		  model_getVertexPosition(model, part, poly, t + 1),
		  model_getVertexPosition(model, part, poly, t + 2)
		);
	}
	return normalize(normalSum);
}

DenseModel DenseModel_create(const Model& original) {
	return std::make_shared<DenseModelImpl>(original);
}

static int getTriangleCount(const Model& original) {
	int triangleCount = 0;
	for (int part = 0; part < model_getNumberOfParts(original); part++) {
		for (int poly = 0; poly < model_getNumberOfPolygons(original, part); poly++) {
			int vertexCount = model_getPolygonVertexCount(original, part, poly);
			triangleCount += vertexCount - 2;
		}
	}
	return triangleCount;
}

DenseModelImpl::DenseModelImpl(const Model& original)
: triangles(getTriangleCount(original), DenseTriangle()) {
	// Get the bounding box
	model_getBoundingBox(original, this->minBound, this->maxBound);
	// Generate normals
	int pointCount = model_getNumberOfPoints(original);
	Array<FVector3D> normalPoints(pointCount, FVector3D());
	// Calculate smooth normals in object-space, by adding each polygon's normal to each child vertex
	for (int part = 0; part < model_getNumberOfParts(original); part++) {
		for (int poly = 0; poly < model_getNumberOfPolygons(original, part); poly++) {
			FVector3D polygonNormal = getAverageNormal(original, part, poly);
			for (int vert = 0; vert < model_getPolygonVertexCount(original, part, poly); vert++) {
				int point = model_getVertexPointIndex(original, part, poly, vert);
				normalPoints[point] = normalPoints[point] + polygonNormal;
			}
		}
	}
	// Normalize the result per vertex, to avoid having unbalanced weights when normalizing per pixel
	for (int point = 0; point < pointCount; point++) {
		normalPoints[point] = normalize(normalPoints[point]);
	}
	// Generate a simpler triangle structure
	int triangleIndex = 0;
	for (int part = 0; part < model_getNumberOfParts(original); part++) {
		for (int poly = 0; poly < model_getNumberOfPolygons(original, part); poly++) {
			int vertexCount = model_getPolygonVertexCount(original, part, poly);
			int vertA = 0;
			int indexA = model_getVertexPointIndex(original, part, poly, vertA);
			for (int vertB = 1; vertB < vertexCount - 1; vertB++) {
				int vertC = vertB + 1;
				int indexB = model_getVertexPointIndex(original, part, poly, vertB);
				int indexC = model_getVertexPointIndex(original, part, poly, vertC);
				triangles[triangleIndex] =
				  DenseTriangle(
					FVector4Dto3D(model_getVertexColor(original, part, poly, vertA)) * 255.0f,
					FVector4Dto3D(model_getVertexColor(original, part, poly, vertB)) * 255.0f,
					FVector4Dto3D(model_getVertexColor(original, part, poly, vertC)) * 255.0f,
					model_getPoint(original, indexA), model_getPoint(original, indexB), model_getPoint(original, indexC),
					normalPoints[indexA], normalPoints[indexB], normalPoints[indexC]
				  );
				triangleIndex++;
			}
		}
	}
}

// Pre-conditions:
//   * All images must exist and have the same dimensions
//   * diffuseTarget and normalTarget must have RGBA pack order
//   * All triangles in model must be contained within the image bounds after being projected using view
// Post-condition:
//   Returns the dirty pixel bound based on projected positions
// worldOrigin is the perceived world's origin in target pixel coordinates
// modelToWorldSpace is used to place the model freely in the world
template <bool HIGH_QUALITY>
static IRect renderDenseModel(const DenseModel& model, OrthoView view, ImageF32 depthBuffer, ImageRgbaU8 diffuseTarget, ImageRgbaU8 normalTarget, const FVector2D& worldOrigin, const Transform3D& modelToWorldSpace) {
	// Combine position transforms
	Transform3D objectToScreenSpace = combineModelToScreenTransform(modelToWorldSpace, view.worldSpaceToScreenDepth, worldOrigin);
	// Create a pessimistic 2D bound from the 3D bounding box
	IRect pessimisticBound = boundingBoxToRectangle(model->minBound, model->maxBound, objectToScreenSpace);
	// Get the target image bound
	IRect clipBound = image_getBound(depthBuffer);
	// Fast culling test
	if (!IRect::overlaps(pessimisticBound, clipBound)) {
		// Nothing drawn, no dirty rectangle
		return IRect();
	}
	// Combine normal transforms
	FMatrix3x3 modelToNormalSpace = modelToWorldSpace.transform * transpose(view.normalToWorldSpace);
	// Get image properties
	int diffuseStride = image_getStride(diffuseTarget);
	int normalStride = image_getStride(normalTarget);
	int heightStride = image_getStride(depthBuffer);
	// Call getters in advance to avoid call overhead in the loops
	SafePointer<uint32_t> diffuseData = image_getSafePointer(diffuseTarget);
	SafePointer<uint32_t> normalData = image_getSafePointer(normalTarget);
	SafePointer<float> heightData = image_getSafePointer(depthBuffer);
	// Render triangles
	for (int tri = 0; tri < model->triangles.length(); tri++) {
		DenseTriangle triangle =  model->triangles[tri];
		// Transform positions
		FVector3D projectedA = objectToScreenSpace.transformPoint(triangle.posA);
		FVector3D projectedB = objectToScreenSpace.transformPoint(triangle.posB);
		FVector3D projectedC = objectToScreenSpace.transformPoint(triangle.posC);
		IRect triangleBound = IRect::cut(clipBound, getBackCulledTriangleBound(projectedA, projectedB, projectedC));
		if (triangleBound.hasArea()) {
			// Find the first row
			SafePointer<uint32_t> diffuseRow = diffuseData;
			diffuseRow.increaseBytes(diffuseStride * triangleBound.top());
			SafePointer<uint32_t> normalRow = normalData;
			normalRow.increaseBytes(normalStride * triangleBound.top());
			SafePointer<float> heightRow = heightData;
			heightRow.increaseBytes(heightStride * triangleBound.top());
			// Pre-compute matrix inverse for vertex weights
			FVector2D cornerA = FVector3Dto2D(projectedA);
			FVector2D cornerB = FVector3Dto2D(projectedB);
			FVector2D cornerC = FVector3Dto2D(projectedC);
			FMatrix2x2 offsetToWeight = inverse(FMatrix2x2(cornerB - cornerA, cornerC - cornerA));
			// Transform normals
			FVector3D normalA = modelToNormalSpace.transform(triangle.normalA);
			FVector3D normalB = modelToNormalSpace.transform(triangle.normalB);
			FVector3D normalC = modelToNormalSpace.transform(triangle.normalC);
			// Iterate over the triangle's bounding box
			for (int y = triangleBound.top(); y < triangleBound.bottom(); y++) {
				SafePointer<uint32_t> diffusePixel = diffuseRow + triangleBound.left();
				SafePointer<uint32_t> normalPixel = normalRow + triangleBound.left();
				SafePointer<float> heightPixel = heightRow + triangleBound.left();
				for (int x = triangleBound.left(); x < triangleBound.right(); x++) {
					FVector2D weightBC = offsetToWeight.transform(FVector2D(x + 0.5f, y + 0.5f) - cornerA);
					FVector3D weight = FVector3D(1.0f - (weightBC.x + weightBC.y), weightBC.x, weightBC.y);
					// Check if the pixel is inside the triangle
					if (weight.x >= -0.00001f && weight.y >= -0.00001f && weight.z >= -0.00001f ) {
						float height = interpolateUsingAffineWeight(projectedA.z, projectedB.z, projectedC.z, weight);
						if (height > *heightPixel) {
							FVector3D vertexColor = interpolateUsingAffineWeight(triangle.colorA, triangle.colorB, triangle.colorC, weight);
							*heightPixel = height;
							// Write data directly without saturation (Do not use colors outside of the visible range!)
							*diffusePixel = ((uint32_t)vertexColor.x) | ENDIAN_POS_ADDR(((uint32_t)vertexColor.y), 8) | ENDIAN_POS_ADDR(((uint32_t)vertexColor.z), 16) | ENDIAN_POS_ADDR(255, 24);
							if (HIGH_QUALITY) {
								FVector3D normal = (normalize(interpolateUsingAffineWeight(normalA, normalB, normalC, weight)) + 1.0f) * 127.5f;
								*normalPixel = ((uint32_t)normal.x) | ENDIAN_POS_ADDR(((uint32_t)normal.y), 8) | ENDIAN_POS_ADDR(((uint32_t)normal.z), 16) | ENDIAN_POS_ADDR(255, 24);
							} else {
								FVector3D normal = (interpolateUsingAffineWeight(normalA, normalB, normalC, weight) + 1.0f) * 127.5f;
								*normalPixel = ((uint32_t)normal.x) | ENDIAN_POS_ADDR(((uint32_t)normal.y), 8) | ENDIAN_POS_ADDR(((uint32_t)normal.z), 16) | ENDIAN_POS_ADDR(255, 24);
							}
						}
					}
					diffusePixel += 1;
					normalPixel += 1;
					heightPixel += 1;
				}
				diffuseRow.increaseBytes(diffuseStride);
				normalRow.increaseBytes(normalStride);
				heightRow.increaseBytes(heightStride);
			}
		}
	}
	return pessimisticBound;
}

void sprite_generateFromModel(ImageRgbaU8& targetAtlas, String& targetConfigText, const Model& visibleModel, const Model& shadowModel, const OrthoSystem& ortho, const String& targetPath, int cameraAngles) {
	// Validate input
	if (cameraAngles < 1) {
		printText("  Need at least one camera angle to generate a sprite!\n");
		return;
	} else if (!model_exists(visibleModel)) {
		printText("  There's nothing to render, because visible model does not exist!\n");
		return;
	} else if (model_getNumberOfParts(visibleModel) == 0) {
		printText("  There's nothing to render in the visible model, because there are no parts in the visible model!\n");
		return;
	} else {
		// Measure the bounding cylinder for determining the uncropped image size
		FVector3D minBound, maxBound;
		model_getBoundingBox(visibleModel, minBound, maxBound);
		// Check if generating a bound failed
		if (minBound.x > maxBound.x) {
			printText("  There's nothing visible in the model, because the 3D bounding box had no points to be created from!\n");
			return;
		}

		printText("  Representing height from ", minBound.y, " to ", maxBound.y, " encoded using 8-bits\n");

		// Calculate initial image size
		float worstCaseDiameter = (std::max(maxBound.x, -minBound.x) + std::max(maxBound.y, -minBound.y) + std::max(maxBound.z, -minBound.z)) * 2;
		int maxRes = roundUp(worstCaseDiameter * ortho.pixelsPerTile, 2) + 4; // Round up to even pixels and add 4 padding pixels

		// Allocate square images from the pessimistic size estimation
		int width = maxRes;
		int height = maxRes;
		ImageF32 depthBuffer = image_create_F32(width, height);
		ImageRgbaU8 colorImage[cameraAngles];
		ImageRgbaU8 heightImage[cameraAngles];
		ImageRgbaU8 normalImage[cameraAngles];
		for (int a = 0; a < cameraAngles; a++) {
			colorImage[a] = image_create_RgbaU8(width, height);
			heightImage[a] = image_create_RgbaU8(width, height);
			normalImage[a] = image_create_RgbaU8(width, height);
		}
		// Generate the optimized model structure with normals
		DenseModel denseModel = DenseModel_create(visibleModel);
		// Render the model to multiple render targets at once
		float heightScale = 255.0f / (maxBound.y - minBound.y);
		for (int a = 0; a < cameraAngles; a++) {
			image_fill(depthBuffer, -1000000000.0f);
			image_fill(colorImage[a], ColorRgbaI32(0, 0, 0, 0));
			FVector2D origin = FVector2D((float)width * 0.5f, (float)height * 0.5f);
			renderDenseModel<true>(denseModel, ortho.view[a], depthBuffer, colorImage[a], normalImage[a], origin, Transform3D());
			// Convert height into an 8 bit channel for saving
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					int32_t opacityPixel = image_readPixel_clamp(colorImage[a], x, y).alpha;
					int32_t heightPixel = (image_readPixel_clamp(depthBuffer, x, y) - minBound.y) * heightScale;
					image_writePixel(heightImage[a], x, y, ColorRgbaI32(heightPixel, 0, 0, opacityPixel));
				}
			}
		}

		// Crop all images uniformly for easy atlas packing
		int32_t minX = width;
		int32_t minY = height;
		int32_t maxX = 0;
		int32_t maxY = 0;
		for (int a = 0; a < cameraAngles; a++) {
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					if (image_readPixel_border(colorImage[a], x, y).alpha) {
						if (x < minX) minX = x;
						if (x > maxX) maxX = x;
						if (y < minY) minY = y;
						if (y > maxY) maxY = y;
					}
				}
			}
		}
		// Check if cropping failed
		if (minX > maxX) {
			printText("  There's nothing visible in the model, because cropping the final images returned nothing!\n");
			return;
		}

		IRect cropRegion = IRect(minX, minY, (maxX + 1) - minX, (maxY + 1) - minY);
		if (cropRegion.width() < 1 || cropRegion.height() < 1) {
			printText("  Cropping failed to find any drawn pixels!\n");
			return;
		}
		for (int a = 0; a < cameraAngles; a++) {
			colorImage[a] = image_getSubImage(colorImage[a], cropRegion);
			heightImage[a] = image_getSubImage(heightImage[a], cropRegion);
			normalImage[a] = image_getSubImage(normalImage[a], cropRegion);
		}
		int croppedWidth = cropRegion.width();
		int croppedHeight = cropRegion.height();
		int centerX = width / 2 - cropRegion.left();
		int centerY = height / 2 - cropRegion.top();
		printText("  Cropped images of ", croppedWidth, "x", croppedHeight, " pixels with centers at (", centerX, ", ", centerY, ")\n");

		// Pack everything into an image atlas
		targetAtlas = image_create_RgbaU8(croppedWidth * 3, croppedHeight * cameraAngles);
		for (int a = 0; a < cameraAngles; a++) {
			draw_copy(targetAtlas, colorImage[a], 0, a * croppedHeight);
			draw_copy(targetAtlas, heightImage[a], croppedWidth, a * croppedHeight);
			draw_copy(targetAtlas, normalImage[a], croppedWidth * 2, a * croppedHeight);
		}

		SpriteConfig config = SpriteConfig(centerX, centerY, cameraAngles, 3, minBound, maxBound);
		if (model_exists(shadowModel) && model_getNumberOfPoints(shadowModel) > 0) {
			config.appendShadow(shadowModel);
		}
		targetConfigText = config.toIni();
	}
}

// Allowing the last decimals to deviate a bit because floating-point operations are rounded differently between computers
static bool approximateTextMatch(const ReadableString &a, const ReadableString &b, double tolerance = 0.00002) {
	int readerA = 0, readerB = 0;
	while (readerA < string_length(a) && readerB < string_length(b)) {
		DsrChar charA = a[readerA];
		DsrChar charB = b[readerB];
		if (character_isValueCharacter(charA) && character_isValueCharacter(charB)) {
			// Scan forward on both sides while consuming content and comparing the actual value
			int startA = readerA;
			int startB = readerB;
			// Only move forward on valid characters
			if (a[readerA] == U'-') { readerA++; }
			if (b[readerB] == U'-') { readerB++; }
			while (character_isDigit(a[readerA])) { readerA++; }
			while (character_isDigit(b[readerB])) { readerB++; }
			if (a[readerA] == U'.') { readerA++; }
			if (b[readerB] == U'.') { readerB++; }
			while (character_isDigit(a[readerA])) { readerA++; }
			while (character_isDigit(b[readerB])) { readerB++; }
			// Approximate values
			double valueA = string_toDouble(string_exclusiveRange(a, startA, readerA));
			double valueB = string_toDouble(string_exclusiveRange(b, startB, readerB));
			// Check the difference
			double diff = valueB - valueA;
			if (diff > tolerance || diff < -tolerance) {
				// Too big difference, this is probably not a rounding error
				return false;
			}
		} else if (charA != charB) {
			// Difference with a non-value involved
			return false;
		}
		readerA++;
		readerB++;
	}
	if (readerA < string_length(a) - 1 || readerB < string_length(b) - 1) {
		// One text had unmatched remains after the other reached its end
		return false;
	} else {
		return true;
	}
}

void sprite_generateFromModel(const Model& visibleModel, const Model& shadowModel, const OrthoSystem& ortho, const String& targetPath, int cameraAngles, bool debug) {
	// Generate an image and a configuration file from the visible model
	ImageRgbaU8 atlasImage; String configText;
	sprite_generateFromModel(atlasImage, configText, visibleModel, shadowModel, ortho, targetPath, cameraAngles);
	// Save the result on success
	if (string_length(configText) > 0) {
		// Save the atlas
		String atlasPath = targetPath + U".png";
		// Try loading any existing image
		ImageRgbaU8 existingAtlasImage = image_load_RgbaU8(atlasPath, false);
		if (image_exists(existingAtlasImage)) {
			int difference = image_maxDifference(atlasImage, existingAtlasImage);
			if (difference <= 2) {
				printText("  No significant changes against ", targetPath, ".\n");
			} else {
				image_save(atlasImage, atlasPath);
				printText("  Updated ", targetPath, " with a deviation of ", difference, ".\n");
			}
		} else {
			// Only save if there was no existing image or it differed significantly from the new result
			// This comparison is made to avoid flooding version history with changes from invisible differences in color rounding
			image_save(atlasImage, atlasPath);
			printText("  Saved atlas to ", targetPath, ".\n");
		}

		// Save the configuration
		String configPath = targetPath + U".ini";
		String oldConfixText = string_load(configPath, false);
		if (approximateTextMatch(configText, oldConfixText)) {
			printText("  No significant changes against ", targetPath, ".\n\n");
		} else {
			string_save(targetPath + U".ini", configText);
			printText("  Saved sprite config to ", targetPath, ".\n\n");
		}

		if (debug) {
			ImageRgbaU8 debugImage; String garbageText;
			// TODO: Show overlap between visible and shadow so that shadow outside of visible is displayed as bright red on a dark model.
			//       The number of visible shadow pixels should be reported automatically
			//       in an error message at the end of the total execution together with file names.
			sprite_generateFromModel(debugImage, garbageText, shadowModel, Model(), ortho, targetPath + U"Debug", 8);
			image_save(debugImage, targetPath + U"Debug.png");
		}
	}
}

}

