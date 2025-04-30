
// TODO:
// Test with multiple displays and high DPI mode.

#include "../../DFPSR/includeFramework.h"

using namespace dsr;

// Get the application folder when possible, falling back on current directory on systems not offering the feature.
static const String applicationFolder = file_getApplicationFolder();
static const String mediaFolder = file_combinePaths(applicationFolder, U"media");
static BasicResourcePool pool(mediaFolder);

// Global variables
static bool running = true;
static double cameraYaw = 0.0f;
static double cameraPitch = 0.0f;
static FVector3D cameraPosition = FVector3D(0.0f, 0.0f, 0.0f);
static bool showCursor = false;

// The window handle
static Window window;

static int createRoomPart(Model model, const FVector3D &min, const FVector3D &max) {
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
	model_addQuad(model, part, 1, 0, 2, 3); // Left quad
	model_addQuad(model, part, 4, 5, 7, 6); // Right quad
	model_addQuad(model, part, 0, 4, 6, 2); // Front quad
	model_addQuad(model, part, 5, 1, 3, 7); // Back quad
	model_addQuad(model, part, 2, 6, 7, 3); // Top quad
	model_addQuad(model, part, 1, 5, 4, 0); // Bottom quad
	return part;
}

static Model createRoomModel(const FVector3D &min, const FVector3D &max) {
	Model result = model_create();
	createRoomPart(result, min, max);
	return result;
}

static IVector2D cursorOrigin;
static int32_t cursorLimitX = 10;
static int32_t cursorLimitY = 10;
static IVector2D previousCursorPosition;
static bool cursorWasReset = false;
static bool firstMouseEvent = true;

static bool moveForward = false;
static bool moveBackward = false;
static bool moveUp = false;
static bool moveDown = false;
static bool moveLeft = false;
static bool moveRight = false;
static bool moveFaster = false;

// The maximum camera pitch in both positive and negative direction.
//   Goes a bit outside of 90 degrees (1.57079633 radians) just to show that it is possible when calculating the up vector instead of hardcoding it to a constant.
static const double maxPitch = 1.8;

// Room coordinates.
static const FVector3D roomMinimum = FVector3D(-10.0f);
static const FVector3D roomMaximum = FVector3D(10.0f);
static const float cameraCollisionRadius = 1.0f;
static const FVector3D cameraMinimum = roomMinimum + cameraCollisionRadius;
static const FVector3D cameraMaximum = roomMaximum - cameraCollisionRadius;

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Create a full-screen window
	window = window_create_fullscreen(U"David Piuva's Software Renderer - Camera example");
	// Hide the cursor
	window_setCursorVisibility(window, false);

	// Tell the application to terminate when the window is closed
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Get whole window key events
	window_setKeyboardEvent(window, [](const KeyboardEvent& event) {
		DsrKey key = event.dsrKey;
		if (event.keyboardEventType == KeyboardEventType::KeyDown) {
			if (key >= DsrKey_1 && key <= DsrKey_9) {
				// TODO: Create a smooth transition between resolutions.
				window_setPixelScale(window, key - DsrKey_0);
			} else if (key == DsrKey_F11) {
				window_setFullScreen(window, !window_isFullScreen(window));
				window_setCursorVisibility(window, !window_isFullScreen(window));
			} else if (key == DsrKey_Escape) {
				running = false;
			} else if (key == DsrKey_C) {
				// Press C to toggle visibility the cursor and debug drawing.
				showCursor = !showCursor;
				window_setCursorVisibility(window, showCursor);
			} else if (key == DsrKey_W) {
				moveForward = true;
			} else if (key == DsrKey_S) {
				moveBackward = true;
			} else if (key == DsrKey_E) {
				moveUp = true;
			} else if (key == DsrKey_Q) {
				moveDown = true;
			} else if (key == DsrKey_A) {
				moveLeft = true;
			} else if (key == DsrKey_D) {
				moveRight = true;
			} else if (key == DsrKey_Shift) {
				moveFaster = true;
			}
		} else if (event.keyboardEventType == KeyboardEventType::KeyUp) {
			if (key == DsrKey_W) {
				moveForward = false;
			} else if (key == DsrKey_S) {
				moveBackward = false;
			} else if (key == DsrKey_E) {
				moveUp = false;
			} else if (key == DsrKey_Q) {
				moveDown = false;
			} else if (key == DsrKey_A) {
				moveLeft = false;
			} else if (key == DsrKey_D) {
				moveRight = false;
			} else if (key == DsrKey_Shift) {
				moveFaster = false;
			}
		}
	});

	// Get whole window mouse events
	window_setMouseEvent(window, [](const MouseEvent& event) {
		if (firstMouseEvent) {
			// Ignore motion from the first mouse event, because it has no previous cursor position to compare against.
			firstMouseEvent = false;
		} else {
			IVector2D movement = event.position - previousCursorPosition;
			IVector2D offset = event.position - cursorOrigin;
			if (cursorWasReset && offset.x == 0 && offset.y == 0) {
				// The first cursor at the image center after a reset is ignored.
				cursorWasReset = false;
			} else {
				// TODO: Adjust mouse sensitivity somehow.
				double radiansPerCanvasPixel = 0.005 * double(window_getPixelScale(window));
				cameraYaw   += double(movement.x) * radiansPerCanvasPixel;
				cameraPitch -= double(movement.y) * radiansPerCanvasPixel;
				if (cameraPitch > maxPitch) cameraPitch = maxPitch;
				if (cameraPitch < -maxPitch) cameraPitch = -maxPitch;
				if (offset.x < -cursorLimitX || offset.y < -cursorLimitY || offset.x > cursorLimitX || offset.y > cursorLimitY) {
					// The cursor traveled outside of the box, so it is moved to the center.
					if (window_setCursorPosition(window, cursorOrigin.x, cursorOrigin.y)) {
						// If successful, remember that the cursor was reset, so that the next mouse move event going to the center can be ignored.
						cursorWasReset = true;
					}
				}
			}
		}
		previousCursorPosition = event.position;
	});

	// Create a room model
	Model roomModel = createRoomModel(roomMinimum, roomMaximum);
	model_setDiffuseMapByName(roomModel, 0, pool, "Grid");

	// Create a renderer for multi-threading
	Renderer worker = renderer_create();

	double lastTime = 0.0;
	while(running) {
		// Measure time
		double time = time_getSeconds();
		double timePerFrame = time - lastTime;

		// Fetch mouse and keyboard events from the window.
		window_executeEvents(window);

		// Request buffers after executing the events, to get newly allocated buffers after resize events
		ImageRgbaU8 colorBuffer = window_getCanvas(window);
		// For a sky box, the depth buffer can be replaced with ImageF32() as an empty image handle.
		//ImageF32 depthBuffer;
		// But for demonstration purposes, we render the room using a depth buffer.
		ImageF32 depthBuffer = window_getDepthBuffer(window);

		// Get target size
		int targetWidth = image_getWidth(colorBuffer);
		int targetHeight = image_getHeight(colorBuffer);

		// Reset the mouse to the center of the canvas when getting too far out.
		cursorOrigin = IVector2D(targetWidth / 2, targetHeight / 2);
		cursorLimitX = targetWidth / 4;
		cursorLimitY = targetHeight / 4;

		// No need to paint the background for indoor scenes, because everything will be covered by geometry.
		//   Any gap will be undefined behavior because some window backends use double buffering while others do not.
		//   One way to make sure that this does not happen, is to let the debug version clear the color with a flashing color while looking for holes.
		//image_fill(colorBuffer, ColorRgbaI32(0, 0, 0, 0));

		// Clear the depth buffer
		image_fill(depthBuffer, 0.0f); // Infinite reciprocal depth using zero


		// Calculate camera orientation from pitch and yaw in radians.
		FVector3D cameraForwardDirection = FVector3D(sin(cameraYaw) * cos(cameraPitch), sin(cameraPitch), cos(cameraYaw) * cos(cameraPitch));
		FVector3D cameraUpDirection = FVector3D(-sin(cameraYaw) * sin(cameraPitch), cos(cameraPitch), -cos(cameraYaw) * sin(cameraPitch));
		FMatrix3x3 cameraRotation = FMatrix3x3::makeAxisSystem(cameraForwardDirection, cameraUpDirection);
		Camera camera = Camera::createPerspective(Transform3D(cameraPosition, cameraRotation), targetWidth, targetHeight);

		// Move the camera.
		double speed = moveFaster ? 40.0 : 10.0;
		double moveOffset = speed * timePerFrame;
		if (moveForward) cameraPosition = cameraPosition + cameraForwardDirection * moveOffset;
		if (moveBackward) cameraPosition = cameraPosition - cameraForwardDirection * moveOffset;
		if (moveUp) cameraPosition = cameraPosition + cameraUpDirection * moveOffset;
		if (moveDown) cameraPosition = cameraPosition - cameraUpDirection * moveOffset;
		if (moveLeft) cameraPosition = cameraPosition - cameraRotation.xAxis * moveOffset;
		if (moveRight) cameraPosition = cameraPosition + cameraRotation.xAxis * moveOffset;

		// Collide against walls.
		if (cameraPosition.x < cameraMinimum.x) cameraPosition.x = cameraMinimum.x;
		if (cameraPosition.y < cameraMinimum.y) cameraPosition.y = cameraMinimum.y;
		if (cameraPosition.z < cameraMinimum.z) cameraPosition.z = cameraMinimum.z;
		if (cameraPosition.x > cameraMaximum.x) cameraPosition.x = cameraMaximum.x;
		if (cameraPosition.y > cameraMaximum.y) cameraPosition.y = cameraMaximum.y;
		if (cameraPosition.z > cameraMaximum.z) cameraPosition.z = cameraMaximum.z;

		// Begin render batch
		renderer_begin(worker, colorBuffer, depthBuffer);
		// Projected triangles from the room's model
		renderer_giveTask(worker, roomModel, Transform3D(), camera);
		// Render the projected triangles
		renderer_end(worker);

		// Debug draw the camera rotation system, which is toggled using the C button.
		if (showCursor) {
			IVector2D writer = IVector2D(10, 10);
			font_printLine(colorBuffer, font_getDefault(), string_combine(U"cameraYaw = ", cameraYaw, U" radians"), writer, ColorRgbaI32(255, 255, 255, 255)); writer.y += 20;
			font_printLine(colorBuffer, font_getDefault(), string_combine(U"cameraPitch = ", cameraPitch, U" radians"), writer, ColorRgbaI32(255, 255, 255, 255)); writer.y += 20;
			font_printLine(colorBuffer, font_getDefault(), string_combine(U"forward = ", cameraForwardDirection), writer, ColorRgbaI32(255, 255, 255, 255)); writer.y += 20;
			font_printLine(colorBuffer, font_getDefault(), string_combine(U"up = ", cameraUpDirection), writer, ColorRgbaI32(255, 255, 255, 255)); writer.y += 20;
			// Draw the region that the cursor can move within without jumping to the center.
			int32_t left = cursorOrigin.x - cursorLimitX;
			int32_t right = cursorOrigin.x + cursorLimitX;
			int32_t top = cursorOrigin.y - cursorLimitY;
			int32_t bottom = cursorOrigin.y + cursorLimitY;
			draw_line(colorBuffer, left, top, right, top, ColorRgbaI32(255, 255, 255, 255));
			draw_line(colorBuffer, left, bottom, right, bottom, ColorRgbaI32(255, 255, 255, 255));
			draw_line(colorBuffer, left, top, left, bottom, ColorRgbaI32(255, 255, 255, 255));
			draw_line(colorBuffer, right, top, right, bottom, ColorRgbaI32(255, 255, 255, 255));
		} else {
			int32_t crosshairRadius = cursorLimitY / 16;
			int32_t left = cursorOrigin.x - crosshairRadius;
			int32_t right = cursorOrigin.x + crosshairRadius;
			int32_t top = cursorOrigin.y - crosshairRadius;
			int32_t bottom = cursorOrigin.y + crosshairRadius;
			draw_line(colorBuffer, left, cursorOrigin.y, right, cursorOrigin.y, ColorRgbaI32(255, 255, 255, 255));
			draw_line(colorBuffer, cursorOrigin.x, top, cursorOrigin.x, bottom, ColorRgbaI32(255, 255, 255, 255));
		}

		// Upload canvas to window.
		window_showCanvas(window);

		lastTime = time;
	}
}
