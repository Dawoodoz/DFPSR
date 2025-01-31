// zlib open source license
//
// Copyright (c) 2018 to 2019 David Forsgren Piuva
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.

#ifndef DFPSR_GUI_COMPONENT_PANEL
#define DFPSR_GUI_COMPONENT_PANEL

#include "../VisualComponent.h"

namespace dsr {

class Panel : public VisualComponent {
PERSISTENT_DECLARATION(Panel)
public:
	// Attributes
	PersistentBoolean solid; // If true, the panel itself will be drawn.
	PersistentBoolean plain; // If true, a solid color will be drawn instead of a buffered image to save time and memory.
	// Name of theme class used to draw the background.
	//   "Panel" is used if backgroundClass is empty or not found.
	PersistentString backgroundClass;
	PersistentColor color = PersistentColor(130, 130, 130); // The color being used when solid is set to true.
	void declareAttributes(StructureDefinition &target) const override;
	Persistent* findAttribute(const ReadableString &name) override;
private:
	void loadTheme(const VisualTheme &theme);
	void completeAssets();
	void generateGraphics();
	// Settings fetched from the theme
	String finalBackgroundClass; // The selected BackgroundClass/Class from layout settings or the component's default theme class "Panel".
	int background_filter = 0; // 0 for solid, 1 for alpha filter.
	// Images
	MediaMethod background;
	OrderedImageRgbaU8 imageBackground; // Alpha is copied to the target and should be 255
	// Generated
	bool hasImages = false;
public:
	Panel();
public:
	bool isContainer() const override;
	void drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) override;
	void changedTheme(VisualTheme newTheme) override;
	void changedLocation(const IRect &oldLocation, const IRect &newLocation) override;
	void changedAttribute(const ReadableString &name) override;
};

}

#endif

