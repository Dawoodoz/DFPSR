
/*
TODO:
	* Create a visual graph with instruments, filters, speakers and file recorders to test a modular sound engine.
	* Allow recording the output of a session into a hi-fi stereo sound buffer, which can later be exported as a file.
	* Create a basic compressed music format for looping sounds in different speed and volume from compressed interpolated curves.
	* Make a list of named instruments containing a list of voices.
		Each voice refers to a sound buffer by index (using names in files) and an envelope for how to play the sound.
		Each voice will be played as its own instrument but from the same input for a richer sound without having to duplicate notes.
		The sounds can be either embedded into the project (editable for tiny instrument patterns) or refer to external files (for whole music tracks).
*/

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
int basicTone, testSound;
int playing[toneCount];
void createTestProject() {
	for (int t = 0; t < toneCount; t++) {
		playing[t] = -1;
	}
	// Pure tone
	basicTone = generateMonoSoundBuffer(U"sine", 441, 44100, [](double time) -> float {
		return sin(time * (cyclesToRadians * 100));
	});
	// Loaded from file
	testSound = loadSoundFromFile(U"Water.wav");
}

static EnvelopeSettings envelope = EnvelopeSettings(0.1, 0.2, 0.8, 0.4, 0.1, -0.02, 0.04, 0.5);
static double previewPressTime = 1.0;
static double previewViewTime = 4.0;

static int selectedBuffer = 0;
static void limitSelection() {
	int maxIndex = getSoundBufferCount() - 1;
	if (selectedBuffer < 0) selectedBuffer = 0;
	if (selectedBuffer > maxIndex) selectedBuffer = maxIndex;
}

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Start sound thread
	printText("Initializing sound\n");
	sound_initialize();

	// Create something to test
	printText("Creating test project\n");
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
				playing[toneIndex] = playSound(basicTone, true, 0.25, 0.25, 3.0 + toneIndex * 0.25, envelope);
			} else if (key == DsrKey_A) {
				playSound(testSound, false, 1.0, 0.0, 1.0);
			} else if (key == DsrKey_S) {
				playSound(testSound, false, 1.0, 1.0, 1.0);
			} else if (key == DsrKey_D) {
				playSound(testSound, false, 0.0, 1.0, 1.0);
			} else if (key == DsrKey_UpArrow) {
				selectedBuffer--;
				limitSelection();
			} else if (key == DsrKey_DownArrow) {
				selectedBuffer++;
				limitSelection();
			}
		} else if (event.keyboardEventType == KeyboardEventType::KeyUp) {
			if (key >= DsrKey_1 && key <= DsrKey_9) {
				int toneIndex = key - DsrKey_1;
				releaseSound(playing[toneIndex]); // Soft stop with following release
			} else if (key == DsrKey_Space) {
				stopAllSounds();
			}
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
		int width = image_getWidth(canvas);
		// Draw things
		drawEnvelope(canvas, IRect(0, 50, width, 100), envelope, previewPressTime, previewViewTime);
		// TODO: Group into a visual component for viewing sound buffers.
		int top = 150;
		for (int s = 0; s < getSoundBufferCount(); s++) {
			int height = 100;
			drawSound(canvas, IRect(0, top, width, height), s, s == selectedBuffer);
			top += height;
		}
		// Draw interface
		window_drawComponents(window);
		// Show the final image
		window_showCanvas(window);
	}
	// Close sound thread
	printText("Terminating sound\n");
	sound_terminate();
}
