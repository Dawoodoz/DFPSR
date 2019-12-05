
#include <limits>
#include "../../DFPSR/includeFramework.h"

using namespace dsr;

const String mediaPath = string_combine(U"media", file_separator());
static BasicResourcePool pool(mediaPath);

// Global variables
std::shared_ptr<VisualComponent> mainPanel;
float distance = 4.0f;
bool running = true;
int detailLevel = 2;
bool useOrthogonalCamera = false;
bool useDepthBuffer = true;

// The window handle
Window window;

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

int main(int argn, char **argv) {
	// Create a window
	window = window_create(U"David Piuva's Software Renderer - Cube example", 1600, 900);
	// Load an interface to the window
	window_loadInterfaceFromFile(window, mediaPath + U"interface.lof");

	// Tell the application to terminate when the window is closed
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Get whole window key events
	window_setKeyboardEvent(window, [](const KeyboardEvent& event) {
		if (event.keyboardEventType == KeyboardEventType::KeyDown) {
			DsrKey key = event.dsrKey;
			if (key >= DsrKey_F1 && key <= DsrKey_F3) {
				detailLevel = key - DsrKey_F1;
			} else if (key >= DsrKey_1 && key <= DsrKey_9) {
				window_setPixelScale(window, key - DsrKey_0);
			} else if (key == DsrKey_F11) {
				window_setFullScreen(window, !window_isFullScreen(window));
			} else if (key == DsrKey_Escape) {
				running = false;
			}
		}
	});

	// Get component handles
	Component mainPanel = window_findComponentByName(window, U"mainPanel", true);
	Component buttonA = window_findComponentByName(window, U"buttonA", true);
	Component buttonB = window_findComponentByName(window, U"buttonB", true);

	// Connect components with actions
	component_setMouseMoveEvent(mainPanel, [](const MouseEvent& event) {
		distance = event.position.y / (float)window_getCanvasHeight(window) * 20.0f + 0.01f;
	});
	component_setPressedEvent(buttonA, []() {
		useOrthogonalCamera = !useOrthogonalCamera;
	});
	component_setPressedEvent(buttonB, []() {
		useDepthBuffer = !useDepthBuffer;
	});

	// Create a cube model
	Model cubeModel = createCubeModel(FVector3D(-0.5f), FVector3D(0.5f));
	model_setDiffuseMapByName(cubeModel, 0, pool, "RGB");
	model_setFilter(cubeModel, Filter::Alpha);

	// Import models
	// TODO: Load write protected models from a resource pool
	Model crateModel = importFromContent_DMF1(string_load(mediaPath + U"Model_Crate.dmf"), pool);
	Model barrelModel = importFromContent_DMF1(string_load(mediaPath + U"Model_Barrel.dmf"), pool);
	Model testModel = importFromContent_DMF1(string_load(mediaPath + U"Model_Test.dmf"), pool);

	// Create a renderer for multi-threading
	Renderer worker = renderer_create();

	while(running) {
		double startTime;
		window_executeEvents(window);

		// Request buffers after executing the events, to get newly allocated buffers after resize events
		auto colorBuffer = window_getCanvas(window);
		auto depthBuffer = window_getDepthBuffer(window);

		// Get target size
		int targetWidth = image_getWidth(colorBuffer);
		int targetHeight = image_getHeight(colorBuffer);

		// Paint the background color
		startTime = time_getSeconds();
		// TODO: Make a SIMD vectorized color fill for non-uniform bytes
		//       Round the start location up to 16-bytes and the end location down to 16-bytes
		//       Use regular assignments for the non-padding leftover pixels in sub-images
		image_fill(colorBuffer, ColorRgbaI32(160, 180, 200, 255));
		printText("Fill sky: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");

		// Update the depth buffer
		startTime = time_getSeconds();
		// Clear the buffer
		if (useOrthogonalCamera) {
			image_fill(depthBuffer, std::numeric_limits<float>::infinity()); // Infinite depth
		} else {
			image_fill(depthBuffer, 0.0f); // Infinite reciprocal depth using zero
		}
		printText("Clear depth: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");

		// Create a camera
		const double speed = 0.2f;
		double timer = time_getSeconds() * speed;
		FVector3D cameraPosition = FVector3D(sin(timer) * distance, 2, cos(timer) * distance);
		FMatrix3x3 cameraRotation = FMatrix3x3::makeAxisSystem(-cameraPosition, FVector3D(0.0f, 1.0f, 0.0f));
		Camera camera = useOrthogonalCamera ?
		  Camera::createOrthogonal(Transform3D(cameraPosition, cameraRotation), targetWidth, targetHeight, 8.0f) :
		  Camera::createPerspective(Transform3D(cameraPosition, cameraRotation), targetWidth, targetHeight);

		Transform3D testLocation(FVector3D(0.0f, -3.0f, 0.0f), FMatrix3x3(3.0f));
		Transform3D crateLocation(FVector3D(sin(timer * 0.36) * 0.21, sin(timer * 1.4) * 0.8, sin(timer * 0.43) * 0.17), FMatrix3x3(4.0f));
		Transform3D barrelLocation(FVector3D(sin(timer * 2.36) * 4.6, sin(timer * 3.45) * 4.6, sin(timer * 2.14 + 3.6) * 4.6), FMatrix3x3(4.0f));
		Transform3D cubeLocation(FVector3D(sin(timer * 4.37) * 2.6, sin(timer * 2.64) * 2.6, sin(timer * 3.34 + 2.7) * 2.6), FMatrix3x3());

		startTime = time_getSeconds();
		ImageF32 depth = useDepthBuffer ? depthBuffer : ImageF32();
		// Begin render batch
		renderer_begin(worker, colorBuffer, depth);
		// Solid
		renderer_giveTask(worker, crateModel, crateLocation, camera);
		renderer_giveTask(worker, barrelModel, barrelLocation, camera);
		renderer_giveTask(worker, testModel, testLocation, camera);
		// Filter
		renderer_giveTask(worker, cubeModel, cubeLocation, camera);
		// Complete render batch
		renderer_end(worker);
		printText("Draw world: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");

		startTime = time_getSeconds();
		window_drawComponents(window);
		printText("Draw GUI: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");

		startTime = time_getSeconds();
		window_showCanvas(window);
		printText("Show canvas: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");
	}
}

