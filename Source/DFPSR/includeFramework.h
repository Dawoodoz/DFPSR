
// Header for including the most commonly needed parts of the framework.

#ifndef DFPSR_INCLUDED_FRAMEWORK
#define DFPSR_INCLUDED_FRAMEWORK
	// Strings, buffers, files, et cetera.
	#include "includeEssentials.h"

	// Types needed to use the APIs
	#include "math/includeMath.h" // Mathematical types
	#include "collection/includeCollection.h" // Generic collections

	// 2D API
	#include "api/imageAPI.h" // Creating images and modifying pixels
	#include "api/textureAPI.h" // Creating textures and sampling pixels
	#include "api/drawAPI.h" // Efficient drawing on images
	#include "api/filterAPI.h" // Efficient image generation, resizing and filtering
	// 3D API
	#include "api/modelAPI.h" // Polygon models for 3D rendering
	// GUI API
	#include "api/guiAPI.h" // Handling windows, interfaces and components
	#include "api/mediaMachineAPI.h" // A machine for running image functions
	#include "api/fontAPI.h" // Printing text to images
	// Convenient API
	#include "api/algorithmAPI.h" // Functions for performing operations on whole collections
	#include "api/timeAPI.h" // Methods for time and delays
	#include "api/configAPI.h" // Making it easy to load your application's settings from configuration files

	// TODO: Create more APIs
	#include "gui/VisualTheme.h" // An unfinished system for changing how the GUI looks

#endif
