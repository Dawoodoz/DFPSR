
// TODO:
// * Make this project into an application that starts automatically to test GUI and sound after building the Builder build system.
// * Let the user browse a file system and select a location for a new or existing project.
// * Explain how everything works when starting for the first time, using a command line argument.
// * A catalogue of SDK examples with images and descriptions loaded automatically from their folder.
//     * Offer one-click build and execution of SDK examples on multiple platforms, while explaining how the building works.

#include "../../DFPSR/includeFramework.h"
#include "sound.h"

using namespace dsr;

// Global
bool running = true;
Window window;
Component mainPanel;
Component toolPanel;

String interfaceContent =
UR"QUOTE(
Begin : Panel
	Name = "mainPanel"
	Solid = 0
	Begin : Panel
		Name = "toolPanel"
		Color = 180,180,180
		Solid = 1
		bottom = 50
	End
End
)QUOTE";

static const double pi = 3.1415926535897932384626433832795;
static const double cyclesToRadians = pi * 2.0;
static const int toneCount = 9;
int basicTone, boomSound;
int playing[toneCount];
void createTestProject() {
	for (int t = 0; t < toneCount; t++) {
		playing[t] = -1;
	}
	// Pure tone
	basicTone = generateMonoSoundBuffer(U"sine", 441, 44100, soundFormat_F32, [](double time) -> double {
		return sin(time * (cyclesToRadians * 100));
	});
	// Loaded from file
	boomSound = loadSoundFromFile(file_combinePaths(file_getApplicationFolder(), U"Boom.wav"));
}

static EnvelopeSettings envelope = EnvelopeSettings(0.1, 0.2, 0.8, 0.4, 0.1, -0.02, 0.04, 0.5);
static double previewPressTime = 1.0;
static double previewViewTime = 4.0;

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	printText(U"Input arguments:\n");
	for (int a = 0; a < args.length(); a++) {
		printText(U"  args[", a, "] = ", args[a], U"\n");
	}

	// Start sound thread
	printText(U"Initializing sound\n");
	sound_initialize();

	// Create something to test
	printText(U"Creating test project\n");
	createTestProject();

	// Create a window
	window = window_create(U"Sound generator", 800, 600);

	// Load an interface to the window
	window_loadInterfaceFromString(window, interfaceContent);

	// Find components
	mainPanel = window_findComponentByName(window, U"mainPanel");
	toolPanel = window_findComponentByName(window, U"toolPanel");

	// Bind methods to events
	window_setKeyboardEvent(window, [](const KeyboardEvent& event) {
		DsrKey key = event.dsrKey;
		if (event.keyboardEventType == KeyboardEventType::KeyDown) {
			if (key == DsrKey_Escape) {
				running = false;
			} else if (key >= DsrKey_1 && key <= DsrKey_9) {
				int toneIndex = key - DsrKey_1;
				printText(U"Start tone ", toneIndex, U"\n");
				playing[toneIndex] = playSound(basicTone, true, 0.25, 0.25, 3.0 + toneIndex * 0.25, envelope);
			} else if (key == DsrKey_0) {
				playSound(boomSound, false, 0.25, 1.0, 1.0);
			}
		} else if (event.keyboardEventType == KeyboardEventType::KeyUp) {
			if (key >= DsrKey_1 && key <= DsrKey_9) {
				int toneIndex = key - DsrKey_1;
				printText(U"End tone ", toneIndex, U"\n");
				releaseSound(playing[toneIndex]); // Soft stop with following release
			} else if (key == DsrKey_Space) {
				stopAllSounds();
			}
		} else if (event.keyboardEventType == KeyboardEventType::KeyType) {
			String message;
			string_append(message, U"Typed ");
			string_appendChar(message, event.character);
			string_append(message, " of code ", event.character, "\n");
			printText(message);
		}
	});
	/*
	component_setMouseDownEvent(mainPanel, [](const MouseEvent& event) {
		
	});
	component_setMouseMoveEvent(mainPanel, [](const MouseEvent& event) {
		
	});
	component_setMouseUpEvent(mainPanel, [](const MouseEvent& event) {
		
	});
	*/
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Execute
	while(running) {
		// Wait for actions so that we don't render until an action has been recieved
		// This will save battery on laptops for applications that don't require animation
		while (!window_executeEvents(window)) {
			time_sleepSeconds(0.01);
		}
		// Fill the background
		AlignedImageRgbaU8 canvas = window_getCanvas(window);
		image_fill(canvas, ColorRgbaI32(64, 64, 64, 255));
		// Draw things
		drawEnvelope(canvas, IRect(0, 50, 550, 100), envelope, previewPressTime, previewViewTime);
		drawSound(canvas, IRect(0, 150, 550, 100), boomSound);
		drawSound(canvas, IRect(0, 250, 550, 100), basicTone);
		// Draw interface
		window_drawComponents(window);
		// Show the final image
		window_showCanvas(window);
	}
	// Close sound thread
	printText(U"Terminating sound\n");
	sound_terminate();
}
