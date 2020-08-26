
/*
	An application for previewing tiles and sprites together for potential games.
	If you design game assets separatelly, they will often look much worse when you put them together.
	Unmatching scale, shadows, colors, themes, et cetera...
	That's why it's important to preview your assets together as early as possible while still designing them.
*/

/*
BUGS:
	* The mouse move is repeated automatically when changing pixel scale, but the same doesn't work for when the window itself moved.
		How can a new mouse-move event be triggered from the current location when toggling full-screen so that the window itself moves?
	* Tiles placed at different heights do not have synchronized rounding between each other.
		Try to round the Y offset separatelly from the XZ location's screen coordinate.
	* The light buffer gets white from point light when there's nothing drawn on the background.
		This prevent using other background colors than black.

3D BUGS:
	DRAWN:
		* There's an ugly seam from not connecting the other side of cylinder fields.
			Probably haven't created any extra triangle strip on that region.
	SHADOWS:
		* The bounding box of shadows differs from the visible pixel's bound in the config file.
			Expand the bound using the shadow model's points to include everything safely.
		* When eroding the dimensions of shadow shapes, there's gaps when placing tiles next to each other
			Can erosion and bias be applied in each shadow map while sampling or as a separate pass?
			Is this much bias even needed when using bilinear interpolation in depth divided space directly from the texture?
		* There's no way to close the gaps on height fields without using black pixels to create zero offset at the ends.
			This creates open holes when not using zero clipping.
			An optional triangle patch can be added along the open sides. (all for planes and excluding sides for cylinders)

VISUALS:
	* Make a directed light source that casts light and shadows from a fixed direction but can fade like a point light.
		Useful for street-lights and sky-lights that want to avoid normalizing and projecting light directions per pixel.
		Can be used both with and without casting shadows.
		Can use intensity maps to project patterns within the square.
			A rough 2D convex hull from the image can be generated for a tighter light frustum.
			Otherwise, one can just apply a round mask and use a cone.
	* Projective background decals.
		Used like passive lights but drawing to the diffuse layer and ignoring dynamic sprites.
		Will only be drawn when updating passive blocks or adding to existing background blocks.
		A 3D transform defines where the decal is placed like a cube in world space.
			The near and far clipping can use a fading threshold to allow placing explosion decals without creating hard seams.
		New sprites added after a decal should not be affected by an old sprite.
			How can this be solved without resorting to dangerous polymorphism.
		Allow defining decals locally for each level by loading their images from a temporary image pool of level specific content.
			This can be used to write instructions specific to a certain mission and give a unique look to an otherwise generic level.
			Billboards and signs can also be possible to reuse with custom images and text.
	* Static 3D models that are rendered when the background updates.
		These have normal resolution and can be freely rotated, scaled and colored.
		They draw shadows just like the pre-rendered sprites.
	* See if there's a shadow smoothing method worth using on the CPU.
		The blend filter is already quite heavy with the saturation, so it might as well do something more useful than a single multiplication as the main feature.
		The difficult thing is to preserve details from normal mapping and tiny details while making shadow edges look smooth.
	* Allow having many high-quality light sources by introducing fully passive lights.
		Useful for indirect light from the sky and general ambient light.
		The background stores RGBA light buffers to make passive lights super cheap.
			This light will mostly store soft light, so shadows from dynamic sprites will
			draw blob shadows as decals on the background before drawing themselves.
			This will give an illusion of dynamic ambient occlusion,
			especially if surface normals affect the intensity using custom shadow decals.
		Dynamic sprites overwrites with their own interpretation of the passive light.
		Dynamic lights add to the light buffer without caring about what's background and what's dynamic.
		A quad-tree stencil will remember which areas have foreground drawn on top of the background.
			This stencil is later used for a pass of dynamic light from passive light sources using stored primary cubemaps.
			The background will divide the light using multiple cube-maps for the same illumination by adding offset varitations in the light sampling function.
	* Make a reusable system for distance adaptive light sources.
		The same illumination filter should take multiple cubemaps rendered from slightly different locations.
			These can be interleaved into a unified packed look-up if the distortion
			of looking it up from the same offset is compensated for somehow.
		The first cubemap will be persistent and used later for dynamic light.
		The later cubemaps will be temporary when generating the background's softer light.
USABILITY:
	* Tool for selecting and removing passive sprites.
		Use both unique handles for simplicity and the raw look-up for handling multiple sprites at once:
			Given an optional integer argument (defaulted to zero) to background sprite construction.
			This allow making custom filtering of sprites by category or giving a unique index to a sprite.
			A lookup can later return references to the sprite instances together with the key and allow custom filtering.
			A deletion lookup can take a function returning true when the background sprite should be deleted.
				The full 3D location and custom key will be returned for filtering.
				If the game wants to filter by direction or anything else, then encode that into the key.
OPTIMIZE:
	* Make a tile based light culling.
		The background has pre-stored minimum and maximum depth for tiles of 32² pixel blocks.
			The screen has 64² pixel min-max blocks reading from 4-9 background blocks.
		Drawing active sprites will write using its own 32² max blocks to the screens depth bound.
			Minimum is kept because drawing can only increase and rarely covers whole areas.
		Each 64² block on the screen then generates a tilted cube hull of the region's visible pixels.
			This tells which light frustums are seen and which parts of their cube maps have to be rendered.
		After rendering the seen shadow-map viewports, blocks including the same set of light sources are merged horizontally.
			A vertical split of blocks is used for multi-threading.
		Example light count for square light regions (real regions will be shaped by 3D light frustums intersecting visible pixel bounds)
			0--01----10-0
			1--12-21-10-0
			1--12-21-10-0
			1-----10----0
	* Decrease peak time using a vertical brick pattern using a half row offset on odd background block columns.
		This is optimized for wide aspect ratios, which is more common than standing formats.
		Cutting the peak repainting area into half without increasing the minimum buffered region.
		Scheduling updates of nearby blocks can take one at a time when there's nothing that must be updated instantly.
	* Create a debug feature in spriteAPI for displaying the octree using lines.
		One color for the owned space and another for the sprite bounding boxes.
		Pressing a certain button in Sandbox should toggle the debug drawing to allow asserting that the tree is well balanced for the level's size.

LATER:
	* Make a ground layer using height and blend maps for outdoor scenes.
		Each tile region will decide if ground should be drawn there.
			Disabling the ground on a tile will look at the main tile replacing the ground for walking heights.
		Grass and small stones will use a separate system, because background sprites do not adapt to the ground height.
			These can be generated from deterministic random values compared against blend maps to save space.
			Additional natural sprites can be added one by one at specific locations.
	* When loading the frames from an atlas, crop the images further and apply separate offsets per frame.
		This will significantly improve rendering speed for 8 direction sprites.
*/

#include "../../DFPSR/includeFramework.h"
#include "sprite/spriteAPI.h"
#include "../../DFPSR/image/PackOrder.h"
#include <assert.h>
#include <limits>

using namespace dsr;

static const String mediaPath = string_combine(U"media", file_separator());
static const String imagePath = string_combine(mediaPath, U"images", file_separator());

// Variables
static bool running = true;
static IVector2D mousePos;
static bool panorate = false;
static bool tileAlign = false;
static bool showOverlays = true;
static int debugView = 0;
static int mouseLights = 1;

// The window handle
static Window window;

static int random(const int minimum, const int maximum) {
	if (maximum > minimum) {
		return (std::rand() % (maximum + 1 - minimum)) + minimum;
	} else {
		return minimum;
	}
}

// Variables
static Sprite brush(0, dir0, IVector3D(), true);
static const int brushStep = ortho_miniUnitsPerTile / 32;
static int buttonPressed[4] = {0, 0, 0, 0};
static IVector2D cameraMovement;
static const float cameraSpeed = 1.0f;

// World
static SpriteWorld world;
bool ambientLight = true;

void sandbox_main() {
	// Create the world
	world = spriteWorld_create(OrthoSystem(string_load(string_combine(mediaPath, U"Ortho.ini"))), 256);

	// Create a window
	String title = U"David Piuva's Software Renderer - Graphics sandbox";
	window = window_create(title, 1600, 900);
	//window = window_create_fullscreen(title);

	// Load an interface to the window
	window_loadInterfaceFromFile(window, mediaPath + U"interface.lof");

	// Tell the application to terminate when the window is closed
	window_setCloseEvent(window, []() {
		running = false;
	});
	// Get direct window events
	window_setMouseEvent(window, [](const MouseEvent& event) {
		if (event.mouseEventType == MouseEventType::MouseMove) {
			if (panorate) {
				// Move the camera in exact pixels
				spriteWorld_moveCameraInPixels(world, mousePos - event.position);
			}
			mousePos = event.position;
		}
	});
	window_setKeyboardEvent(window, [](const KeyboardEvent& event) {
		DsrKey key = event.dsrKey;
		if (event.keyboardEventType == KeyboardEventType::KeyDown) {
			if (key == DsrKey_V) {
				debugView = 0;
			} else if (key == DsrKey_B) {
				debugView = 1;
			} else if (key == DsrKey_N) {
				debugView = 2;
			} else if (key == DsrKey_M) {
				debugView = 3;
			} else if (key == DsrKey_L) {
				debugView = 4;
			} else if (key >= DsrKey_1 && key <= DsrKey_9) {
				window_setPixelScale(window, key - DsrKey_0);
			} else if (key == DsrKey_R) {
				ambientLight = !ambientLight;
			} else if (key == DsrKey_T) {
				tileAlign = !tileAlign;
			} else if (key == DsrKey_F) {
				showOverlays = !showOverlays;
			} else if (key == DsrKey_K) {
				mouseLights = (mouseLights + 1) % 5;
			} else if (key == DsrKey_Q) {
				// Previous type
				brush.typeIndex = (brush.typeIndex + sprite_getTypeCount() - 1) % sprite_getTypeCount();
			} else if (key == DsrKey_E) {
				// Next type
				brush.typeIndex = (brush.typeIndex + 1) % sprite_getTypeCount();
			} else if (key == DsrKey_X) {
				brush.direction = correctDirection(brush.direction + dir90);
			} else if (key == DsrKey_C) {
				// Rotate the world clockwise using four camera angles
				spriteWorld_setCameraDirectionIndex(world, (spriteWorld_getCameraDirectionIndex(world) + 1) % 4);
			} else if (key == DsrKey_Z) {
				// Rotate the world counter-clockwise using four camera angles
				spriteWorld_setCameraDirectionIndex(world, (spriteWorld_getCameraDirectionIndex(world) + 3) % 4);
			} else if (key == DsrKey_F11) {
				// Toggle full-screen
				window_setFullScreen(window, !window_isFullScreen(window));
			} else if (key == DsrKey_Escape) {
				// Terminate safely after the next frame
				running = false;
			} else if (key == DsrKey_LeftArrow || key == DsrKey_A) {
				buttonPressed[0] = 1;
			} else if (key == DsrKey_RightArrow || key == DsrKey_D) {
				buttonPressed[1] = 1;
			} else if (key == DsrKey_UpArrow || key == DsrKey_W) {
				buttonPressed[2] = 1;
			} else if (key == DsrKey_DownArrow || key == DsrKey_S) {
				buttonPressed[3] = 1;
			}
		} else if (event.keyboardEventType == KeyboardEventType::KeyUp) {
			if (key == DsrKey_LeftArrow || key == DsrKey_A) {
				buttonPressed[0] = 0;
			} else if (key == DsrKey_RightArrow || key == DsrKey_D) {
				buttonPressed[1] = 0;
			} else if (key == DsrKey_UpArrow || key == DsrKey_W) {
				buttonPressed[2] = 0;
			} else if (key == DsrKey_DownArrow || key == DsrKey_S) {
				buttonPressed[3] = 0;
			}
		}
		cameraMovement.x = buttonPressed[1] - buttonPressed[0];
		cameraMovement.y = buttonPressed[3] - buttonPressed[2];
	});
	// Get component handles and assign actions
	Component mainPanel = window_getRoot(window);
	component_setMouseDownEvent(mainPanel, [](const MouseEvent& event) {
		if (event.key == MouseKeyEnum::Left) {
			// Place a new visual instance using the brush
			spriteWorld_addBackgroundSprite(world, brush);
		} else if (event.key == MouseKeyEnum::Right) {
			panorate = true;
		}
	});
	component_setMouseUpEvent(mainPanel, [](const MouseEvent& event) {
		if (event.key == MouseKeyEnum::Right) {
			panorate = false;
		}
	});
	component_setMouseScrollEvent(mainPanel, [](const MouseEvent& event) {
		if (event.key == MouseKeyEnum::ScrollUp) {
			brush.location.y += brushStep;
		} else if (event.key == MouseKeyEnum::ScrollDown) {
			brush.location.y -= brushStep;
		}
	});

	Component exitButton = window_findComponentByName(window, U"ExitButton");
	component_setPressedEvent(exitButton, []() {
		running = false;
	});

	// Create sprite types
	sprite_loadTypeFromFile(imagePath, U"Floor");
	sprite_loadTypeFromFile(imagePath, U"WoodenFloor");
	sprite_loadTypeFromFile(imagePath, U"WoodenFence");
	sprite_loadTypeFromFile(imagePath, U"WoodenBarrel");
	sprite_loadTypeFromFile(imagePath, U"Pillar");
	sprite_loadTypeFromFile(imagePath, U"Character_Mage");

	// Create passive sprites
	for (int z = -300; z < 300; z++) {
		for (int x = -300; x < 300; x++) {
			// The bottom floor does not have to throw shadows
			spriteWorld_addBackgroundSprite(world, Sprite(random(0, 1), random(0, 3) * dir90, IVector3D(x * ortho_miniUnitsPerTile, 0, z * ortho_miniUnitsPerTile), false));
		}
	}
	for (int z = -300; z < 300; z++) {
		for (int x = -300; x < 300; x++) {
			if (random(1, 4) == 1) {
				// Obstacles should cast shadows when possible
				spriteWorld_addBackgroundSprite(world, Sprite(random(2, 4), random(0, 3) * dir90, IVector3D(x * ortho_miniUnitsPerTile, 0, z * ortho_miniUnitsPerTile), true));
			} else if (random(1, 20) == 1) {
				// Characters are just static geometry for testing
				spriteWorld_addBackgroundSprite(world, Sprite(5, random(0, 7) * dir45, IVector3D(x * ortho_miniUnitsPerTile, 0, z * ortho_miniUnitsPerTile), true));
			}
		}
	}

	// Animation timing
	double frameStartTime = time_getSeconds();
	double secondsPerFrame = 0.0;
	double stepRemainder = 0.0;

	// Profiling
	double profileStartTime = time_getSeconds();
	int64_t profileFrameCount = 0; // Frames per second
	float profileFrameRate = 0.0f;
	double maxFrameTime = 0.0, lastMaxFrameTime = 0.0; // Peak per second

	while(running) {
		double startTime;

		// Execute actions
		window_executeEvents(window);

		// Request buffers after executing the events, to get newly allocated buffers after resize events
		AlignedImageRgbaU8 colorBuffer = window_getCanvas(window);

		// Calculate a number of whole millisecond ticks per frame
		//   By performing game logic in multiples of msTicks, integer operations
		//   can be scaled without comming to a full stop in high frame rates
		stepRemainder += secondsPerFrame * 1000.0;
		int msTicks = (int)stepRemainder;
		stepRemainder -= (double)msTicks;

		// Move the camera
		int cameraSteps = (int)(cameraSpeed * msTicks);
		// TODO: Find a way to move the camera using exact pixel offsets so that the camera's 3D location is only generating the 2D offset when rotating.
		//       Can the brush be guaranteed to come back to the mouse location after adding and subtracting the same 2D camera offset?
		//         A new integer coordinate system along the ground might move half a pixel vertically and a full pixel sideways in the diagonal view.
		//       Otherwise the approximation defeats the whole purpose of using whole integers in msTicks.
		spriteWorld_moveCameraInPixels(world, cameraMovement * cameraSteps);

		// Remove temporary visuals
		spriteWorld_clearTemporary(world);

		// Place the brush
		IVector3D mouseWorldPos = spriteWorld_findGroundAtPixel(world, colorBuffer, mousePos);
		brush.location.x = mouseWorldPos.x;
		brush.location.z = mouseWorldPos.z;
		if (tileAlign) {
			brush.location = ortho_roundToTile(brush.location);
		}

		// Illuminate the world using soft light from the sky
		if (ambientLight) {
			spriteWorld_createTemporary_directedLight(world, FVector3D(1.0f, -1.0f, 0.0f), 0.1f, ColorRgbI32(255, 255, 255));
		}

		// Create a temporary point light over the brush
		//   Temporary light sources are easier to use for dynamic light because they don't need any handle
		if (mouseLights == 1) {
			spriteWorld_createTemporary_pointLight(world, ortho_miniToFloatingTile(brush.location) + FVector3D(0.0f, 0.5f, 0.0f), 4.0f, 4.0f, ColorRgbI32(128, 255, 128), true);
		} else if (mouseLights == 2) {
			spriteWorld_createTemporary_pointLight(world, ortho_miniToFloatingTile(brush.location) + FVector3D(-2.0f, 0.5f, 1.0f), 4.0f, 2.0f, ColorRgbI32(255, 128, 128), true);
			spriteWorld_createTemporary_pointLight(world, ortho_miniToFloatingTile(brush.location) + FVector3D(2.0f, 0.52f, -1.0f), 4.0f, 2.0f, ColorRgbI32(128, 255, 128), true);
		} else if (mouseLights == 3) {
			spriteWorld_createTemporary_pointLight(world, ortho_miniToFloatingTile(brush.location) + FVector3D(-2.0f, 0.5f, 1.0f), 4.0f, 1.333f, ColorRgbI32(255, 128, 128), true);
			spriteWorld_createTemporary_pointLight(world, ortho_miniToFloatingTile(brush.location) + FVector3D(1.0f, 0.51f, 2.0f), 4.0f, 1.333f, ColorRgbI32(128, 255, 128), true);
			spriteWorld_createTemporary_pointLight(world, ortho_miniToFloatingTile(brush.location) + FVector3D(2.0f, 0.52f, -1.0f), 4.0f, 1.333f, ColorRgbI32(128, 128, 255), true);
		} else if (mouseLights == 4) {
			spriteWorld_createTemporary_pointLight(world, ortho_miniToFloatingTile(brush.location) + FVector3D(-2.0f, 0.5f, 1.0f), 4.0f, 1.0f, ColorRgbI32(255, 128, 128), true);
			spriteWorld_createTemporary_pointLight(world, ortho_miniToFloatingTile(brush.location) + FVector3D(1.0f, 0.51f, 2.0f), 4.0f, 1.0f, ColorRgbI32(128, 255, 128), true);
			spriteWorld_createTemporary_pointLight(world, ortho_miniToFloatingTile(brush.location) + FVector3D(2.0f, 0.52f, -1.0f), 4.0f, 1.0f, ColorRgbI32(128, 128, 255), true);
			spriteWorld_createTemporary_pointLight(world, ortho_miniToFloatingTile(brush.location) + FVector3D(-1.0f, 0.53f, -2.0f), 4.0f, 1.0f, ColorRgbI32(255, 255, 128), true);
		}

		// Show the brush
		spriteWorld_addTemporarySprite(world, brush);

		// Draw the world
		spriteWorld_draw(world, colorBuffer);

		// Debug views (Slow but failsafe)
		if (debugView == 1) {
			draw_copy(colorBuffer, spriteWorld_getDiffuseBuffer(world));
		} else if (debugView == 2) {
			draw_copy(colorBuffer, spriteWorld_getNormalBuffer(world));
		} else if (debugView == 3) {
			AlignedImageF32 heightBuffer = spriteWorld_getHeightBuffer(world);
			for (int y = 0; y < image_getHeight(colorBuffer); y++) {
				for (int x = 0; x < image_getWidth(colorBuffer); x++) {
					float height = image_readPixel_clamp(heightBuffer, x, y) * 255.0f;
					if (height < 0.0f) { height = 0.0f; }
					if (height > 255.0f) { height = 255.0f; }
					image_writePixel(colorBuffer, x, y, ColorRgbaI32(height, 0, 0, 255));
				}
			}
		} else if (debugView == 4) {
			draw_copy(colorBuffer, spriteWorld_getLightBuffer(world));
		}

		// Overlays mode
		if (showOverlays) {
			startTime = time_getSeconds();
				window_drawComponents(window);
			debugText("Draw GUI: ", (time_getSeconds() - startTime) * 1000.0, " ms\n");

			IVector2D writer = IVector2D(10, 55);
			font_printLine(colorBuffer, font_getDefault(), string_combine(U"FPS: ", profileFrameRate), writer, ColorRgbaI32(255, 255, 255, 255)); writer.y += 20;
			font_printLine(colorBuffer, font_getDefault(), string_combine(U"avg ms: ", 1000.0f / profileFrameRate), writer, ColorRgbaI32(255, 255, 255, 255)); writer.y += 20;
			font_printLine(colorBuffer, font_getDefault(), string_combine(U"max ms: ", 1000.0f * lastMaxFrameTime), writer, ColorRgbaI32(255, 255, 255, 255)); writer.y += 20;
		}

		window_showCanvas(window);

		double newTime = time_getSeconds();
		secondsPerFrame = newTime - frameStartTime;
		frameStartTime = newTime;
		debugText("Total frame: ", secondsPerFrame * 1000.0, " ms\n\n");

		// Profiling
		if (secondsPerFrame > maxFrameTime) { maxFrameTime = secondsPerFrame; }
		profileFrameCount++;
		if (newTime > profileStartTime + 1.0) {
			double duration = newTime - profileStartTime;
			profileFrameRate = (double)profileFrameCount / duration;
			profileStartTime = newTime;
			profileFrameCount = 0;
			lastMaxFrameTime = maxFrameTime;
			maxFrameTime = 0.0;
		}
	}
}

