
#include <limits>
#include "../../DFPSR/includeFramework.h"

using namespace dsr;

// Global variables
bool running = true;

// The window handle
Window window;

// Textures for 3D models must use power-of-two dimensions
AlignedImageU8 darkEdge = image_fromAscii(
	"< .-x>"
	"<xxxxxxxxxxxxxxxx>"
	"<x--------------x>"
	"<x-............-x>"
	"<x-.          .-x>"
	"<x-.          .-x>"
	"<x-.          .-x>"
	"<x-.          .-x>"
	"<x-.          .-x>"
	"<x-.          .-x>"
	"<x-.          .-x>"
	"<x-.          .-x>"
	"<x-.          .-x>"
	"<x-.          .-x>"
	"<x-............-x>"
	"<x--------------x>"
	"<xxxxxxxxxxxxxxxx>"
);
OrderedImageRgbaU8 myTexture = image_pack(darkEdge, darkEdge, 0, 255);

int createCubePart(Model model, const FVector3D &min, const FVector3D &max) {
	// Add positions
	model_addPoint(model, FVector3D(min.x, min.y, min.z)); // 0: Left-down-near
	model_addPoint(model, FVector3D(min.x, min.y, max.z)); // 1: Left-down-far
	model_addPoint(model, FVector3D(min.x, max.y, min.z)); // 2: Left-up-near
	model_addPoint(model, FVector3D(min.x, max.y, max.z)); // 3: Left-up-far
	model_addPoint(model, FVector3D(max.x, min.y, min.z)); // 4: Right-down-near
	model_addPoint(model, FVector3D(max.x, min.y, max.z)); // 5: Right-down-far
	model_addPoint(model, FVector3D(max.x, max.y, min.z)); // 6: Right-up-near
	model_addPoint(model, FVector3D(max.x, max.y, max.z)); // 7: Right-up-far
	// Create a part for the polygons
	int part = model_addEmptyPart(model, U"cube");
	// Polygons using default texture coordinates on the 4 corners of the texture
	model_addQuad(model, part, 3, 2, 0, 1); // Left quad
	model_addQuad(model, part, 6, 7, 5, 4); // Right quad
	model_addQuad(model, part, 2, 6, 4, 0); // Front quad
	model_addQuad(model, part, 7, 3, 1, 5); // Back quad
	model_addQuad(model, part, 3, 7, 6, 2); // Top quad
	model_addQuad(model, part, 0, 4, 5, 1); // Bottom quad
	return part;
}

Model createCubeModel(const FVector3D &min, const FVector3D &max) {
	Model result = model_create();
	createCubePart(result, min, max);
	return result;
}

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Create a window
	window = window_create(U"Basic 3D template", 1600, 900);

	// Tell the application to terminate when the window is closed
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Get whole window key events
	window_setKeyboardEvent(window, [](const KeyboardEvent& event) {
		if (event.keyboardEventType == KeyboardEventType::KeyDown) {
			DsrKey key = event.dsrKey;
			if (key >= DsrKey_1 && key <= DsrKey_9) {
				window_setPixelScale(window, key - DsrKey_0);
			} else if (key == DsrKey_F11) {
				window_setFullScreen(window, !window_isFullScreen(window));
			} else if (key == DsrKey_Escape) {
				running = false;
			}
		}
	});

	// Genrate mip-maps for the texture
	image_generatePyramid(myTexture);
	// Create a cube model
	Model cubeModel = createCubeModel(FVector3D(-0.5f), FVector3D(0.5f));
	// Assign the texture to part 0
	model_setDiffuseMap(cubeModel, 0, myTexture);

	// Create a renderer for multi-threading
	Renderer worker = renderer_create();

	while(running) {
		window_executeEvents(window);
		auto colorBuffer = window_getCanvas(window);
		auto depthBuffer = window_getDepthBuffer(window);
		int targetWidth = image_getWidth(colorBuffer);
		int targetHeight = image_getHeight(colorBuffer);

		// Paint the background color
		image_fill(colorBuffer, ColorRgbaI32(0, 0, 0, 0));
		image_fill(depthBuffer, 0.0f); // Infinite reciprocal depth using zero

		// Create a camera
		const float distance = 1.3f;
		const float height = 1.0f;
		const double speed = 0.2f;
		double timer = time_getSeconds() * speed;
		FVector3D cameraPosition = FVector3D(sin(timer) * distance, height, cos(timer) * distance);
		FMatrix3x3 cameraRotation = FMatrix3x3::makeAxisSystem(-cameraPosition, FVector3D(0.0f, 1.0f, 0.0f));
		Camera camera = Camera::createPerspective(Transform3D(cameraPosition, cameraRotation), targetWidth, targetHeight);

		// Render
		renderer_begin(worker, colorBuffer, depthBuffer);
		renderer_giveTask(worker, cubeModel, Transform3D(), camera);
		renderer_end(worker);

		window_showCanvas(window);
	}
}
