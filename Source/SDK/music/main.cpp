
/*
TODO:
	* Create a visual graph with instruments, filters, speakers and file recorders to test a modular sound engine.
	* Allow recording the output of a session into a hi-fi stereo sound buffer, which can later be exported as a file.
	* Create a basic compressed music format for looping sounds in different speed and volume from compressed interpolated curves.
	* Make a list of named instruments containing a list of voices.
		Each voice refers to a sound buffer by index (using names in files) and an envelope for how to play the sound.
		Each voice will be played as its own instrument but from the same input for a richer sound without having to duplicate notes.
		The sounds can be either embedded into the project (editable for tiny instrument patterns) or refer to external files (for whole music tracks).
	* Store, modify, import and export MIDI tracks.
*/

#include "../../DFPSR/includeFramework.h"
#include "../SoundEngine/soundEngine.h"

using namespace dsr;

// Global
bool running = true;
Window window;

static const double pi = 3.1415926535897932384626433832795;
static const double cyclesToRadians = pi * 2.0;
static const int toneCount = 10;
Array<int> basicTone = Array<int>(toneCount, -1);
int testSound;
Array<int> playing = Array<int>(toneCount, -1);

int createSine(int frequency, const ReadableString &name) {
	return soundEngine_insertSoundBuffer(sound_generate_function(44100 / frequency, 1, 44100, [frequency](double time, uint32_t channelIndex) {
		return sin(time * (cyclesToRadians * double(frequency))) * 0.25f;
	}), name, false);
}

void createTestProject() {
	// Loaded from file
	testSound = soundEngine_loadSoundFromFile(U"Water.wav");
	// Pure tones
	for (int t = 0; t < toneCount; t++) {
		playing[t] = -1;
	}
	basicTone[0] = createSine(261, U"C 4"); // C 4
	basicTone[1] = createSine(293, U"D 4"); // D 4
	basicTone[2] = createSine(329, U"E 4"); // E 4
	basicTone[3] = createSine(349, U"F 4"); // F 4
	basicTone[4] = createSine(392, U"G 4"); // G 4
	basicTone[5] = createSine(440, U"A 4"); // A 4
	basicTone[6] = createSine(493, U"B 4"); // B 4
	basicTone[7] = createSine(523, U"C 5"); // C 5
	basicTone[8] = createSine(587, U"D 5"); // D 5
	basicTone[9] = createSine(659, U"E 5"); // E 5
}

static EnvelopeSettings envelope = EnvelopeSettings(0.1, 0.2, 0.8, 0.4, 0.1, -0.02, 0.04, 0.5);
static double previewPressTime = 1.0;
static double previewViewTime = 4.0;

static int selectedBuffer = 0;
static void limitSelection() {
	int maxIndex = soundEngine_getSoundBufferCount() - 1;
	if (selectedBuffer < 0) selectedBuffer = 0;
	if (selectedBuffer > maxIndex) selectedBuffer = maxIndex;
}

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Start sound thread
	printText("Initializing sound\n");
	soundEngine_initialize();

	// Create something to test
	printText("Creating test project\n");
	createTestProject();

	// Create a window
	window = window_create(U"Sound generator", 800, 600);

	// Bind methods to events
	window_setKeyboardEvent(window, [](const KeyboardEvent& event) {
		DsrKey key = event.dsrKey;
		if (event.keyboardEventType == KeyboardEventType::KeyDown) {
			if (key == DsrKey_Escape) {
				running = false;
			} else if (key >= DsrKey_1 && key <= DsrKey_9) {
				int toneIndex = key - DsrKey_1;
				// TODO: Stop or reactivate sounds that are still fading out with the same tone to reduce the number of sound players running at the same time.
				playing[toneIndex] = soundEngine_playSound(basicTone[toneIndex], true, 1.0f, 1.0f, envelope);
			} else if (key == DsrKey_0) {
				playing[9] = soundEngine_playSound(basicTone[9], true, 1.0f, 1.0f, envelope);
			} else if (key == DsrKey_Return) {
				// TODO: Loop while holding return and then turn off looping on release.
				soundEngine_playSound(selectedBuffer, false);
			} else if (key == DsrKey_A) {
				// Play from left side.
				soundEngine_playSound(testSound, false, 1.0f, 0.0f);
			} else if (key == DsrKey_S) {
				// Play with half effect.
				soundEngine_playSound(testSound, false, 0.5f, 0.5f);
			} else if (key == DsrKey_D) {
				// Play from right side.
				soundEngine_playSound(testSound, false, 0.0f, 1.0f);
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
				soundEngine_releaseSound(playing[toneIndex]);
			} else if (key == DsrKey_0) {
				soundEngine_releaseSound(playing[9]);
			} else if (key == DsrKey_Space) {
				soundEngine_stopAllSounds();
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
		// Run the application in a delayed loop.
		time_sleepSeconds(0.01);
		window_executeEvents(window);
		// Fill the background
		AlignedImageRgbaU8 canvas = window_getCanvas(window);
		image_fill(canvas, ColorRgbaI32(64, 64, 64, 255));
		int width = image_getWidth(canvas);
		// Draw things
		int height = 50;
		int top = 0;
		soundEngine_drawEnvelope(canvas, IRect(0, 0, width, height), envelope, previewPressTime, previewViewTime);
		top += height;
		for (int s = 0; s < soundEngine_getSoundBufferCount(); s++) {
			soundEngine_drawSound(canvas, IRect(0, top, width, height), s, s == selectedBuffer);
			top += height;
		}
		// Draw interface
		window_drawComponents(window);
		// Show the final image
		window_showCanvas(window);
	}
	// Close sound thread
	printText("Terminating sound\n");
	soundEngine_terminate();
}
