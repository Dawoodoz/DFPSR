
#include <assert.h>
#include <limits>
#include <functional>
#include "../../DFPSR/includeFramework.h"
#include "sprite/spriteAPI.h"
#include "sprite/importer.h"

using namespace dsr;

static constexpr float colorScale = 1.0f / 255.0f;
static FVector4D pixelToVertexColor(const ColorRgbaI32& color) {
	return FVector4D(color.red * colorScale, color.green * colorScale, color.blue * colorScale, 1.0f);
}

static int createTriangle(Model& model, int part, int indexA, int indexB, int indexC, FVector4D colorA, FVector4D colorB, FVector4D colorC, bool flip = false) {
	if (flip) {
		int poly = model_addTriangle(model, part, indexB, indexA, indexC);
		model_setVertexColor(model, part, poly, 0, colorB);
		model_setVertexColor(model, part, poly, 1, colorA);
		model_setVertexColor(model, part, poly, 2, colorC);
		return poly;
	} else {
		int poly = model_addTriangle(model, part, indexA, indexB, indexC);
		model_setVertexColor(model, part, poly, 0, colorA);
		model_setVertexColor(model, part, poly, 1, colorB);
		model_setVertexColor(model, part, poly, 2, colorC);
		return poly;
	}
}

using TransformFunction = std::function<FVector3D(int pixelX, int pixelY, int displacement)>;

// Returns the start point index for another side to weld against
int createGridSide(Model& model, int part, const ImageU8& heightMap, const ImageRgbaU8& colorMap,
  const TransformFunction& transform, bool clipZero, bool mergeSides, bool flipDepth = false, bool flipFaces = false, int otherStartPointIndex = -1) {
	int startPointIndex = model_getNumberOfPoints(model);
	int mapWidth = image_getWidth(heightMap);
	int mapHeight = image_getHeight(heightMap);
	int flipScale = flipDepth ? -1 : 1;
	int columns = mergeSides ? mapWidth + 1 : mapWidth;
	// Create a part for the polygons
	for (int z = 0; z < mapHeight; z++) {
		for (int x = 0; x < columns; x++) {
			// Sample the height map and convert to world space
			int cx = x % mapWidth;
			int heightC = image_readPixel_border(heightMap, cx, z);
			// Add the point to the model
			if (x < mapWidth) {
				// Create a position from the 3D index
				model_addPoint(model, transform(x, z, heightC * flipScale));
			}
			if (x > 0 && z > 0) {
				// Create vertex data
				//   A-B
				//     |
				//   D-C
				int px = x - 1;
				int cz = z;
				int pz = z - 1;
				// Sample previous heights
				int heightA = image_readPixel_border(heightMap, px, pz);
				int heightB = image_readPixel_border(heightMap, cx, pz);
				int heightD = image_readPixel_border(heightMap, px, cz);
				// Tell where to weld with another side's points
				bool weldA = otherStartPointIndex > -1 && heightA == 0;
				bool weldB = otherStartPointIndex > -1 && heightB == 0;
				bool weldC = otherStartPointIndex > -1 && heightC == 0;
				bool weldD = otherStartPointIndex > -1 && heightD == 0;
				// Get indices to points
				int indexA = (weldA ? otherStartPointIndex : startPointIndex) + px + pz * mapWidth;
				int indexB = (weldB ? otherStartPointIndex : startPointIndex) + cx + pz * mapWidth;
				int indexC = (weldC ? otherStartPointIndex : startPointIndex) + cx + cz  * mapWidth;
				int indexD = (weldD ? otherStartPointIndex : startPointIndex) + px + cz  * mapWidth;
				// Sample colors
				FVector4D colorA = pixelToVertexColor(image_readPixel_tile(colorMap, px, pz));
				FVector4D colorB = pixelToVertexColor(image_readPixel_tile(colorMap, cx, pz));
				FVector4D colorC = pixelToVertexColor(image_readPixel_tile(colorMap, cx, cz));
				FVector4D colorD = pixelToVertexColor(image_readPixel_tile(colorMap, px, cz));
				// Decide how to split triangles and which ones to display
				bool acSplit = false;
				bool skipFirst = false;
				bool skipSecond = false;
				if (heightA == 0 && heightC == 0) {
					// ABCD fan of ABC and ACD
					acSplit = true;
					if (heightB == 0) { skipFirst = true; }
					if (heightD == 0) { skipSecond = true; }
				} else if (heightB == 0 && heightD == 0) {
					// BCDA fan of ACD and BDA
					acSplit = false;
					if (heightC == 0) { skipFirst = true; }
					if (heightA == 0) { skipSecond = true; }
				} else {
					int cA = image_readPixel_tile(heightMap, cx - 2, cz - 2);
					int cB = image_readPixel_tile(heightMap, cx + 1, cz - 2);
					int cC = image_readPixel_tile(heightMap, cx + 1, cz + 1);
					int cD = image_readPixel_tile(heightMap, cx - 2, cz + 1);
					int diffAC = abs((cA + cC) - (heightA + heightC));
					int diffBD = abs((cB + cD) - (heightB + heightD));
					acSplit = diffBD > diffAC;
				}
				if (!clipZero) {
					skipFirst = false;
					skipSecond = false;
				}
				// Create a polygon
				if (!(skipFirst && skipSecond)) {
					if (acSplit) {
						if (!skipFirst) {
							createTriangle(model, part,
							  indexA, indexB, indexC,
							  colorA, colorB, colorC, flipFaces
							);
						}
						if (!skipSecond) {
							createTriangle(model, part,
							  indexA, indexC, indexD,
							  colorA, colorC, colorD, flipFaces
							);
						}
					} else {
						if (!skipFirst) {
							createTriangle(model, part,
							  indexB, indexC, indexD,
							  colorB, colorC, colorD, flipFaces
							);
						}
						if (!skipSecond) {
							createTriangle(model, part,
							  indexB, indexD, indexA,
							  colorB, colorD, colorA, flipFaces
							);
						}
					}
				}
			}
		}
	}
	return startPointIndex;
}

// clipZero:
//   Removing triangles from pixels with displacement zero.
//   Used for carving out non-square shapes using black height as the background.
// mergeSides:
//   Connect vertices from the left side of the image with the right side using additional polygons.
//   Used for cylinder shapes to remove the seam where the sides meet.
// mirror:
//   Create another instance of the height field with surfaces and displacement turned in the other direction.
// weldNormals:
//   Merges normals between mirrored sides to let normals at displacement zero merge with the other side.
//   mirror must be active for this to have an effect, because there's no mirrored side to weld against otherwise.
//   clipZero must be active to hide polygons without a normal. (What is the average direction of two opposing planes?)
void createGrid(Model& model, int part, const ImageU8& heightMap, const ImageRgbaU8& colorMap,
  const TransformFunction& transform, bool clipZero, bool mergeSides, bool mirror, bool weldNormals) {
	if (weldNormals && !mirror) {
		printText("\n  Warning! Cannot weld normals without a mirrored side. The \"weldNormals\" will be ignored because \"mirror\" was not active.\n\n");
		weldNormals = false;
	}
	if (weldNormals && !clipZero) {
		printText("\n  Warning! Cannot weld normals without clipping zero displacement. The \"weldNormals\" will be ignored because \"clipZero\" was not active.\n\n");
		weldNormals = false;
	}
	// Generate primary side
	int otherStartPointIndex = createGridSide(model, part, heightMap, colorMap, transform, clipZero, mergeSides);
	// Generate additional mirrored side
	if (mirror) {
		createGridSide(model, part, heightMap, colorMap, transform, clipZero, mergeSides, true, true, weldNormals ? otherStartPointIndex : -1);
	}
}

// The part of ParserState that resets when creating a new part but is kept after generating geometry
struct PartSettings {
	Transform3D location;
	float displacement = 1.0f, patchWidth = 1.0f, patchHeight = 1.0f, radius = 0.0f;
	int clipZero = 0; // 1 will cut away displacements from height zero, 0 will try to display all polygons
	int mirror = 0; // 1 will let height fields generate polygons on both sides to create solid shapes
	PartSettings() {}
};

struct ParserState {
	String sourcePath;
	int angles = 4;
	Model model, shadow;
	int part = -1; // Current part index for model (No index used for shadows)
	PartSettings partSettings;
	explicit ParserState(const String& sourcePath) : sourcePath(sourcePath), model(model_create()), shadow(model_create()) {
		model_addEmptyPart(this->shadow, U"shadow");
	}
};

static void parse_scope(ParserState& state, const ReadableString& key) {
	// End the previous scope
	state.partSettings = PartSettings();
	state.part = -1;
	if (string_caseInsensitiveMatch(key, U"PART")) {
		// Enter a new part's scope
		printText("  New part begins\n");
		state.part = model_addEmptyPart(state.model, U"part");
	} else {
		printText("  Unrecognized scope ", key, " within <>.\n");
	}
}

#define MATCH_ASSIGN_GLOBAL(NAME,ACCESS,PARSER,DESCRIPTION) \
if (string_caseInsensitiveMatch(key, NAME)) { \
	ACCESS = PARSER(value); \
	printText("  ", #DESCRIPTION, " = ", ACCESS, "\n"); \
}
#define MATCH_ASSIGN(BLOCK,NAME,ACCESS,PARSER,DESCRIPTION) \
if (string_caseInsensitiveMatch(key, NAME)) { \
	if (state.BLOCK == -1) { \
		printText("    Cannot assign ", DESCRIPTION, " without a ", #BLOCK, ".\n"); \
	} else { \
		ACCESS = PARSER(value); \
		printText("    ", #DESCRIPTION, " = ", ACCESS, "\n"); \
	} \
}
static void parse_assignment(ParserState& state, const ReadableString& key, const ReadableString& value) {
	MATCH_ASSIGN_GLOBAL(U"Angles", state.angles, string_toInteger, "camera angle count")
	else MATCH_ASSIGN(part, U"Origin", state.partSettings.location.position, parseFVector3D, "origin")
	else MATCH_ASSIGN(part, U"XAxis", state.partSettings.location.transform.xAxis, parseFVector3D, "X-Axis")
	else MATCH_ASSIGN(part, U"YAxis", state.partSettings.location.transform.yAxis, parseFVector3D, "Y-Axis")
	else MATCH_ASSIGN(part, U"ZAxis", state.partSettings.location.transform.zAxis, parseFVector3D, "Z-Axis")
	else MATCH_ASSIGN(part, U"Displacement", state.partSettings.displacement, string_toDouble, "displacement")
	else MATCH_ASSIGN(part, U"ClipZero", state.partSettings.clipZero, string_toInteger, "zero clipping")
	else MATCH_ASSIGN(part, U"Mirror", state.partSettings.mirror, string_toInteger, "mirror flag")
	else MATCH_ASSIGN(part, U"PatchWidth", state.partSettings.patchWidth, string_toDouble, "patch width")
	else MATCH_ASSIGN(part, U"PatchHeight", state.partSettings.patchHeight, string_toDouble, "patch height")
	else MATCH_ASSIGN(part, U"Radius", state.partSettings.radius, string_toDouble, "radius")
	else {
		printText("    Tried to assign ", value, " to unrecognized key ", key, ".\n");
	}
}

enum class Shape {
	None, Plane, Box, Cylinder, LeftHandedModel, RightHandedModel
};
static Shape ShapeFromName(const ReadableString& name) {
	if (string_caseInsensitiveMatch(name, U"PLANE")) {
		return Shape::Plane;
	} else if (string_caseInsensitiveMatch(name, U"BOX")) {
		return Shape::Box;
	} else if (string_caseInsensitiveMatch(name, U"CYLINDER")) {
		return Shape::Cylinder;
	} else if (string_caseInsensitiveMatch(name, U"LEFTHANDEDMODEL")) {
		return Shape::LeftHandedModel;
	} else if (string_caseInsensitiveMatch(name, U"RIGHTHANDEDMODEL")) {
		return Shape::RightHandedModel;
	} else {
		throwError("Unhandled shape \"", name, "\"!\n");
		return Shape::None;
	}
}
static String nameOfShape(Shape shape) {
	if (shape == Shape::None) {
		return U"None";
	} else if (shape == Shape::Plane) {
		return U"Plane";
	} else if (shape == Shape::Box) {
		return U"Box";
	} else if (shape == Shape::Cylinder) {
		return U"Cylinder";
	} else if (shape == Shape::LeftHandedModel) {
		return U"LeftHandedModel";
	} else if (shape == Shape::RightHandedModel) {
		return U"RightHandedModel";
	} else {
		return U"?";
	}
}

// TODO: Arguments for repeating the input images so that pillars can reuse textures for multiple sides when only one camera angle will be saved
static void generateField(ParserState& state, Shape shape, const ImageU8& heightMap, const ImageRgbaU8& colorMap, bool shadow) {
	Transform3D system = state.partSettings.location;
	bool clipZero = state.partSettings.clipZero;
	float offsetPerUnit = state.partSettings.displacement / 255.0f;
	bool mirror = state.partSettings.mirror != 0;
	bool mergeSides = shape == Shape::Cylinder;
	bool weldNormals = mirror && clipZero;
	// Create a transform function based on the shape
	TransformFunction transform;
	if (shape == Shape::Plane) {
		// PatchWidth along local X
		// PatchHeight along local Z
		// Displacement along local Y
		float widthScale = state.partSettings.patchWidth / (image_getWidth(heightMap) - 1);
		float heightScale = state.partSettings.patchHeight / -(image_getHeight(heightMap) - 1);
		FVector3D localScaling = FVector3D(widthScale, offsetPerUnit, heightScale);
		FVector3D localOrigin = FVector3D(state.partSettings.patchWidth * -0.5f, 0.0f, state.partSettings.patchHeight * 0.5f);
		transform = [system, localOrigin, localScaling](int pixelX, int pixelY, int displacement){
			return system.transformPoint(localOrigin + (FVector3D(pixelX, displacement, pixelY) * localScaling));
		};
	} else if (shape == Shape::Cylinder) {
		// Radius + Displacement along local X, Z
		// PatchHeight along local Y
		float radius = state.partSettings.radius;
		float angleScale = 6.283185307f / image_getWidth(heightMap);
		float angleOffset = angleScale * 0.5f; // Start and end half a pixel from the seam
		float heightScale = state.partSettings.patchHeight / -(image_getHeight(heightMap) - 1);
		float heightOffset = state.partSettings.patchHeight * 0.5f;
		int lastRow = image_getHeight(heightMap) - 1;
		bool fillHoles = !mirror && !clipZero; // Automatically fill the holes to close the shape when not mirroring nor clipping the sides
		transform = [system, angleOffset, angleScale, heightOffset, heightScale, radius, offsetPerUnit, fillHoles, lastRow](int pixelX, int pixelY, int displacement){
			float angle = ((float)pixelX * angleScale) + angleOffset;
			float offset = ((float)displacement * offsetPerUnit) + radius;
			float height = ((float)pixelY * heightScale) + heightOffset;
			if (fillHoles && (pixelY == 0 || pixelY == lastRow)) {
				offset = 0.0f;
			}
			return system.transformPoint(FVector3D(-sin(angle) * offset, height, cos(angle) * offset));
		};
	} else {
		printText("Field generation is not implemented for ", nameOfShape(shape), "!\n");
		return;
	}
	if (shadow) {
		createGrid(state.shadow, 0, heightMap, colorMap, transform, clipZero, mergeSides, mirror, weldNormals);
	} else {
		createGrid(state.model, state.part, heightMap, colorMap, transform, clipZero, mergeSides, mirror, weldNormals);
	}
}

static void generateBasicShape(ParserState& state, Shape shape, const ReadableString& arg1, const ReadableString& arg2, const ReadableString& arg3, bool shadow) {
	Transform3D system = state.partSettings.location;
	Model model = shadow ? state.shadow : state.model;
	int part = shadow ? 0 : state.part;
	// All shapes are centered around the axis system's origin from -0.5 to +0.5 of any given size
	if (shape == Shape::Box) {
		// Parse arguments
		float width = string_toDouble(arg1);
		float height = string_toDouble(arg2);
		float depth = string_toDouble(arg3);
		// Create a bound
		FVector3D upper = FVector3D(width, height, depth) * 0.5f;
		FVector3D lower = -upper;
		// Positions
		int first = model_getNumberOfPoints(model);
		model_addPoint(model, system.transformPoint(FVector3D(lower.x, lower.y, lower.z))); // first + 0: Left-down-near
		model_addPoint(model, system.transformPoint(FVector3D(lower.x, lower.y, upper.z))); // first + 1: Left-down-far
		model_addPoint(model, system.transformPoint(FVector3D(lower.x, upper.y, lower.z))); // first + 2: Left-up-near
		model_addPoint(model, system.transformPoint(FVector3D(lower.x, upper.y, upper.z))); // first + 3: Left-up-far
		model_addPoint(model, system.transformPoint(FVector3D(upper.x, lower.y, lower.z))); // first + 4: Right-down-near
		model_addPoint(model, system.transformPoint(FVector3D(upper.x, lower.y, upper.z))); // first + 5: Right-down-far
		model_addPoint(model, system.transformPoint(FVector3D(upper.x, upper.y, lower.z))); // first + 6: Right-up-near
		model_addPoint(model, system.transformPoint(FVector3D(upper.x, upper.y, upper.z))); // first + 7: Right-up-far
		// Polygons
		model_addQuad(model, part, first + 3, first + 2, first + 0, first + 1); // Left quad
		model_addQuad(model, part, first + 6, first + 7, first + 5, first + 4); // Right quad
		model_addQuad(model, part, first + 2, first + 6, first + 4, first + 0); // Front quad
		model_addQuad(model, part, first + 7, first + 3, first + 1, first + 5); // Back quad
		model_addQuad(model, part, first + 3, first + 7, first + 6, first + 2); // Top quad
		model_addQuad(model, part, first + 0, first + 4, first + 5, first + 1); // Bottom quad
	} else if (shape == Shape::Cylinder) {
		// Parse arguments
		float radius = string_toDouble(arg1);
		float height = string_toDouble(arg2);
		int sideCount = string_toDouble(arg3);
		// Create a bound
		float topHeight = height * 0.5f;
		float bottomHeight = height * -0.5f;
		// Positions
		float angleScale = 6.283185307 / (float)sideCount;
		int centerTop = model_addPoint(model, system.transformPoint(FVector3D(0.0f, topHeight, 0.0f)));
		int firstTopSide = model_getNumberOfPoints(model);
		for (int p = 0; p < sideCount; p++) {
			float radians = p * angleScale;
			model_addPoint(model, system.transformPoint(FVector3D(sin(radians) * radius, topHeight, cos(radians) * radius)));
		}
		int centerBottom = model_addPoint(model, system.transformPoint(FVector3D(0.0f, bottomHeight, 0.0f)));
		int firstBottomSide = model_getNumberOfPoints(model);
		for (int p = 0; p < sideCount; p++) {
			float radians = p * angleScale;
			model_addPoint(model, system.transformPoint(FVector3D(sin(radians) * radius, bottomHeight, cos(radians) * radius)));	
		}
		for (int p = 0; p < sideCount; p++) {
			int q = (p + 1) % sideCount;
			// Top fan
			model_addTriangle(model, part, centerTop, firstTopSide + p, firstTopSide + q);
			// Bottom fan
			model_addTriangle(model, part, centerBottom, firstBottomSide + q, firstBottomSide + p);
			// Side
			model_addQuad(model, part, firstTopSide + q, firstTopSide + p, firstBottomSide + p, firstBottomSide + q);
		}
	} else {
		printText("Basic shape generation is not implemented for ", nameOfShape(shape), "!\n");
		return;
	}
}

// Used when displaying shadow models for debugging
static ImageRgbaU8 createDebugTexture() {
	ImageRgbaU8 result = image_create_RgbaU8(2, 2);
	image_writePixel(result, 0, 0, ColorRgbaI32(255, 0, 0, 255));
	image_writePixel(result, 1, 0, ColorRgbaI32(0, 255, 0, 255));
	image_writePixel(result, 0, 1, ColorRgbaI32(0, 0, 255, 255));
	image_writePixel(result, 1, 1, ColorRgbaI32(255, 255, 0, 255));
	return result;
}
ImageRgbaU8 debugTexture = createDebugTexture();

static void parse_shape(ParserState& state, List<String>& args, bool shadow) {
	if (state.part == -1) {
		printText("    Cannot generate a ", args[0], " without a part.\n");
	}
	Shape shape = ShapeFromName(args[0]);
	if (shape == Shape::LeftHandedModel || shape == Shape::RightHandedModel) {
		if (args.length() > 2) {
			printText("    Too many arguments when trying to load a model. Just give one file name without spaces.\n");
		} else if (args.length() < 2) {
			printText("    Loading a model requires a filename.\n");
		} else {
			bool flipX = (shape == Shape::RightHandedModel);
			Model targetModel = shadow ? state.shadow : state.model;
			int targetPart = shadow ? 0 : state.part;
			importer_loadModel(targetModel, targetPart, string_combine(state.sourcePath, args[1]), flipX, state.partSettings.location);
		}
	} else if (args.length() == 2) {
		// Shape, HeightMap
		ImageU8 heightMap = image_get_red(image_load_RgbaU8(state.sourcePath + args[1]));
		generateField(state, shape, heightMap, debugTexture, shadow);
	} else if (args.length() == 3) {
		// Shape, HeightMap, ColorMap
		ImageU8 heightMap = image_get_red(image_load_RgbaU8(state.sourcePath + args[1]));
		ImageRgbaU8 colorMap = image_load_RgbaU8(state.sourcePath + args[2]);
		generateField(state, shape, heightMap, colorMap, shadow);
	} else if (args.length() == 4) {
		// Shape, Width, Height, Depth
		generateBasicShape(state, shape, args[1], args[2], args[3], shadow);
	} else {
		printText("    The ", args[0], " shape needs at least a height map to know the number of vertices to generate. A color map can also be given.\n");
	}
}

static void parse_dsm(ParserState& state, const ReadableString& content) {
	List<String> lines = string_split(content, U'\n');
	for (int l = 0; l < lines.length(); l++) {
		// Get the current line
		ReadableString line = lines[l];
		// Skip comments
		int commentIndex = string_findFirst(line, U';');
		if (commentIndex > -1) {
			line = string_removeOuterWhiteSpace(string_before(line, commentIndex));
		}
		if (string_length(line) > 0) {
			// Find assignments
			int assignmentIndex = string_findFirst(line, U'=');
			int colonIndex = string_findFirst(line, U':');
			int blockStartIndex = string_findFirst(line, U'<');
			int blockEndIndex = string_findFirst(line, U'>');
			if (assignmentIndex > -1) {
				ReadableString key = string_removeOuterWhiteSpace(string_before(line, assignmentIndex));
				ReadableString value = string_removeOuterWhiteSpace(string_after(line, assignmentIndex));
				parse_assignment(state, key, value);
			} else if (colonIndex > -1) {
				ReadableString command = string_removeOuterWhiteSpace(string_before(line, colonIndex));
				ReadableString argContent = string_after(line, colonIndex);
				List<String> args = string_split(argContent, U',');
				for (int a = 0; a < args.length(); a++) {
					args[a] = string_removeOuterWhiteSpace(args[a]);
				}
				if (string_caseInsensitiveMatch(command, U"Visible")) {
					parse_shape(state, args, false);
				} else if (string_caseInsensitiveMatch(command, U"Shadow")) {
					parse_shape(state, args, true);
				} else {
					printText("    Unrecognized command ", command, ".\n");
				}
			} else if (blockStartIndex > -1 && blockEndIndex > -1) {
				String block = string_removeOuterWhiteSpace(string_inclusiveRange(line, blockStartIndex + 1, blockEndIndex - 1));
				parse_scope(state, block);
			} else {
				printText("Unrecognized content \"", line, "\" on line ", l + 1, ".\n");
			}
		}
	}
}

void processScript(const String& sourcePath, const String& targetPath, OrthoSystem ortho, const String& scriptName) {
	// Initialize a parser state containing an empty model
	ParserState state = ParserState(sourcePath);
	// Parse the script to fill the state with a model and additional render settings
	String scriptPath = string_combine(state.sourcePath, scriptName, U".dsm");
	printText("Generating ", scriptPath, "\n");
	parse_dsm(state, string_load(scriptPath));
	// Render the model
	sprite_generateFromModel(state.model, state.shadow, ortho, targetPath + scriptName, state.angles, false);
}

// The first argument is the source folder in which the model scripts are stored.
// The second argument is the target folder in which the results are saved.
// The third argument is the ortho configuration file path.
// The following arguments are plain names of the scripts to process without any path nor extension.
void tool_main(int argn, char **argv) {
	if (argn < 5) {
		printText("Nothing to process. Terminating sprite generation tool.\n");
	} else {
		String sourcePath = string_combine(argv[1], file_separator());
		String targetPath = string_combine(argv[2], file_separator());
		OrthoSystem ortho = OrthoSystem(string_load(String(argv[3])));
		for (int a = 4; a < argn; a++) {
			processScript(sourcePath, targetPath, ortho, String(argv[a]));
		}
	}
}

