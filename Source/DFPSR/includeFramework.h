
// Header for including the most commonly needed parts of the framework

#ifndef DFPSR_INCLUDED_FRAMEWORK
#define DFPSR_INCLUDED_FRAMEWORK

	// Types needed to use the APIs
	#include "math/includeMath.h"
	#include "api/stringAPI.h"

	// Additional functionality for convenience (not to be used in any API)
	#include "collection/includeCollection.h" // Safer and easier to use than std collections

	// 2D API
	#include "api/imageAPI.h" // Creating images and modifying pixels
	#include "api/drawAPI.h" // Efficient drawing on images
	#include "api/filterAPI.h" // Efficient image generation, resizing and filtering
	// 3D API
	#include "api/modelAPI.h" // Polygon models for 3D rendering
	// GUI API
	#include "api/guiAPI.h" // Handling windows, interfaces and components
	#include "api/mediaMachineAPI.h" // A machine for running image functions
	#include "api/fontAPI.h" // Printing text to images
	// File API
	#include "api/bufferAPI.h" // Storing binary data
	#include "api/fileAPI.h" // Saving and loading binary files
	// Convenient API
	#include "api/timeAPI.h" // Methods for time and delays
	#include "api/configAPI.h" // Making it easy to load your application's settings from configuration files

	// TODO: Create more APIs
	#include "gui/VisualTheme.h" // Place in the gui API

#endif
