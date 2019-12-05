#include "../../DFPSR/includeFramework.h"
#include <raspicam/raspicam.h>

using namespace dsr;

// Global
const String mediaPath = string_combine(U"media", file_separator());
bool running = true;

// The window handle
Window window;

int main(int argn, char **argv) {
	// Create a window
	int cameraWidth = 320 * 2;
	int cameraHeight = 240 * 2;
	window = window_create(U"Raspberry Pi camera application", cameraWidth, cameraHeight);
	// Load an interface to the window
	//window_loadInterfaceFromFile(window, mediaPath + U"?.lof");

	// Bind methods to events
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Start the camera
	raspicam::RaspiCam piCamera;
	piCamera.setWidth(cameraWidth); piCamera.setHeight(cameraHeight);
	piCamera.setFormat(raspicam::RASPICAM_FORMAT_GRAY);
	if (!piCamera.open()) {
		throwErrorMessage("Couldn't find any Raspberry Pi camera!\n");
		return -1;
	}
	time_sleepSeconds(0.1);

	// Create an image for the camera input
	AlignedImageU8 cameraImage = image_create_U8(piCamera.getWidth(), piCamera.getHeight());

	// Execute
	while(running) {
		window_executeEvents(window);
		//window_drawComponents(window);

		// Set shutter time in microseconds
		piCamera.setShutterSpeed(10000); // 10000 fast, 20000 normal
		piCamera.setISO(800); // 100 darkest, 800 brightest
		// Get an image from the camera
		piCamera.grab();
		// Uncomment for a 10 ms cooldown time after each frame
		//time_sleepSeconds(0.1);

		piCamera.retrieve(image_dangerous_getData(cameraImage));

		// Display the image
		auto canvas = window_getCanvas(window);
		draw_copy(canvas, cameraImage);

		// Show the final state of the canvas without flickering
		window_showCanvas(window);
	}
}

