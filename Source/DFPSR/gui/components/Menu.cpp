// zlib open source license
//
// Copyright (c) 2018 to 2023 David Forsgren Piuva
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

#include "Menu.h"

using namespace dsr;

PERSISTENT_DEFINITION(Menu)

static AlignedImageU8 arrowImage = image_fromAscii(
	"< .xX>"
	"<.x.  >"
	"< XX. >"
	"< xXX.>"
	"< XX. >"
	"<.x.  >"
);

void Menu::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"BackColor");
	target.declareAttribute(U"ForeColor");
	target.declareAttribute(U"Text");
	target.declareAttribute(U"Padding");
	target.declareAttribute(U"Spacing");
}

Persistent* Menu::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"Color") || string_caseInsensitiveMatch(name, U"BackColor")) {
		// The short Color alias refers to the back color in Buttons, because most buttons use black text.
		return &(this->backColor);
	} else if (string_caseInsensitiveMatch(name, U"ForeColor")) {
		return &(this->foreColor);
	} else if (string_caseInsensitiveMatch(name, U"Text")) {
		return &(this->text);
	} else if (string_caseInsensitiveMatch(name, U"Padding")) {
		return &(this->padding);
	} else if (string_caseInsensitiveMatch(name, U"Spacing")) {
		return &(this->spacing);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

Menu::Menu() {}

bool Menu::isContainer() const {
	return true;
}

bool Menu::hasArrow() {
	return this->subMenu && this->getChildCount() > 0;
}

static OrderedImageRgbaU8 generateHeadImage(Menu &menu, MediaMethod imageGenerator, int pressed, int width, int height, ColorRgbI32 backColor, ColorRgbI32 foreColor, const ReadableString &text, RasterFont font) {
	// Create a scaled image
	OrderedImageRgbaU8 result;
 	component_generateImage(menu.getTheme(), imageGenerator, width, height, backColor.red, backColor.green, backColor.blue, pressed)(result);
	if (string_length(text) > 0) {
		int backWidth = image_getWidth(result);
		int backHeight = image_getHeight(result);
		int left = menu.padding.value;
		int top = (backHeight - font_getSize(font)) / 2;
		if (pressed) {
			top += 1;
		}
		// Print the text
		font_printLine(result, font, text, IVector2D(left, top), ColorRgbaI32(foreColor, 255));
		// Draw the arrow
		if (menu.hasArrow()) {
			int arrowWidth = image_getWidth(arrowImage);
			int arrowHeight = image_getHeight(arrowImage);
			int arrowLeft = backWidth - arrowWidth - 4;
			int arrowTop = (backHeight - arrowHeight) / 2;
			draw_silhouette(result, arrowImage, ColorRgbaI32(foreColor, 255), arrowLeft, arrowTop);
		}
	}
	return result;
}

void Menu::generateGraphics() {
	int headWidth = this->location.width();
	int headHeight = this->location.height();
	if (headWidth < 1) { headWidth = 1; }
	if (headHeight < 1) { headHeight = 1; }
	if (!this->hasImages) {
		completeAssets();
		this->imageUp = generateHeadImage(*this, this->headImageMethod, 0, headWidth, headHeight, this->backColor.value, this->foreColor.value, this->text.value, this->font);
		this->imageDown = generateHeadImage(*this, this->headImageMethod, 0, headWidth, headHeight, ColorRgbI32(0, 0, 0), ColorRgbI32(255, 255, 255), this->text.value, this->font);
		this->hasImages = true;
	}
}

// Fill the listBackgroundImageMethod with a solid color
void Menu::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	this->generateGraphics();
	draw_alphaFilter(targetImage, this->showingOverlay() ? this->imageDown : this->imageUp, relativeLocation.left(), relativeLocation.top());
}

void Menu::generateBackground() {
	if (!image_exists(this->listBackgroundImage)) {
		int listWidth = this->overlayLocation.width();
		int listHeight = this->overlayLocation.height();
		if (listWidth < 1) { listWidth = 1; }
		if (listHeight < 1) { listHeight = 1; }
		component_generateImage(this->theme, this->listBackgroundImageMethod, listWidth, listHeight, this->backColor.value.red, this->backColor.value.green, this->backColor.value.blue)(this->listBackgroundImage);
	}
}

void Menu::createOverlay() {
	if (!this->showingOverlay()) {
		this->showOverlay();
		this->makeFocused(); // Focus on the current menu path to make others lose focus.
		IRect memberBound = this->children[0]->location;
		for (int i = 1; i < this->getChildCount(); i++) {
			memberBound = IRect::merge(memberBound, this->children[i]->location);
		}
		// Calculate the new list bound.
		this->overlayLocation = memberBound.expanded(this->padding.value) + this->location.upperLeft();
	}
}

bool Menu::managesChildren() {
	return true;
}

bool Menu::pointIsInsideOfOverlay(const IVector2D& pixelPosition) {
	return pixelPosition.x > this->overlayLocation.left() && pixelPosition.x < this->overlayLocation.right() && pixelPosition.y > this->overlayLocation.top() && pixelPosition.y < this->overlayLocation.bottom();
}

void Menu::drawOverlay(ImageRgbaU8& targetImage, const IVector2D &absoluteOffset) {
	this->generateBackground();
	// TODO: Let the theme select between solid and alpha filtered drawing.
	IVector2D overlayOffset = absoluteOffset + this->overlayLocation.upperLeft();
	draw_copy(targetImage, this->listBackgroundImage, overlayOffset.x, overlayOffset.y);
	for (int i = 0; i < this->getChildCount(); i++) {
		this->children[i]->draw(targetImage, absoluteOffset + this->location.upperLeft());
	}
}

void Menu::changedTheme(VisualTheme newTheme) {
	this->headImageMethod = theme_getScalableImage(newTheme, this->subMenu ? U"MenuSub" : U"MenuTop");
	this->listBackgroundImageMethod = theme_getScalableImage(newTheme, U"MenuList");
	this->hasImages = false;
}

void Menu::completeAssets() {
	if (this->headImageMethod.methodIndex == -1) {
		// Work as a sub-menu if the direct parent is also a menu.
		this->subMenu = this->parent != nullptr && dynamic_cast<Menu*>(this->parent) != nullptr;
		this->headImageMethod = theme_getScalableImage(theme_getDefault(), this->subMenu ? U"MenuSub" : U"MenuTop");
		this->listBackgroundImageMethod = theme_getScalableImage(theme_getDefault(), U"MenuList");
	}
	if (this->font.get() == nullptr) {
		this->font = font_getDefault();
	}
}

void Menu::changedLocation(const IRect &oldLocation, const IRect &newLocation) {
	// If the component has changed dimensions then redraw the image
	if (oldLocation.size() != newLocation.size()) {
		this->hasImages = false;
	}
}

void Menu::changedAttribute(const ReadableString &name) {
	if (!string_caseInsensitiveMatch(name, U"Visible")) {
		this->hasImages = false;
	}
	VisualComponent::changedAttribute(name);
}

void Menu::updateStateEvent(ComponentState oldState, ComponentState newState) {
	// If no longer having any type of focus, hide the overlay.
	if ((oldState & componentState_focus) && !(newState & componentState_focus)) {
		// Hide the menu when losing focus.
		this->hideOverlay();
		// State notifications are not triggered from within the same notification, so that one can handle all the updates safely in the desired order.
		this->listBackgroundImage = OrderedImageRgbaU8();
	}
	if (!(newState & componentState_showingOverlayDirect)) {		
		// Clean up the background image to save memory and allow it to be regenerated in another size later.
		this->listBackgroundImage = OrderedImageRgbaU8();
	}
}

void Menu::updateLocationEvent(const IRect& oldLocation, const IRect& newLocation) {	
	int left = this->padding.value;
	int top = this->padding.value;
	int overlap = 3;
	if (this->subMenu) {
		left += newLocation.width() - overlap;
	} else {
		top += newLocation.height() - overlap;
	}
	int maxWidth = 80; // Minimum usable with.
	// Expand list with to fit child components.
	for (int i = 0; i < this->getChildCount(); i++) {
		int width = this->children[i]->getDesiredDimensions().x;
		if (maxWidth < width) maxWidth = width;
	}
	// Stretch out the child components to use the whole width.
	for (int i = 0; i < this->getChildCount(); i++) {
		int height = this->children[i]->getDesiredDimensions().y;
		this->children[i]->applyLayout(IRect(left, top, maxWidth, height));
		top += height + this->spacing.value;
	}
}

static void closeEntireMenu(VisualComponent* menu) {
	while (menu->parent != nullptr) {
		// Hide the menu when closing the menu. Notifications to updateStateEvent will do the proper cleanup for each component's type.
		menu->hideOverlay();
		// Move on to the parent component.
		menu = menu->parent;
	}
}

void Menu::receiveMouseEvent(const MouseEvent& event) {
	int childCount = this->getChildCount();
	MouseEvent localEvent = event;
	localEvent.position -= this->location.upperLeft();
	if (this->showingOverlay() && this->pointIsInsideOfOverlay(event.position)) {
		for (int i = childCount - 1; i >= 0; i--) {
			if (this->children[i]->pointIsInside(localEvent.position)) {
				this->children[i]->makeFocused();
				MouseEvent childEvent = localEvent;
				childEvent.position -= this->children[i]->location.upperLeft();
				this->children[i]->sendMouseEvent(childEvent, true);
				break;
			}
		}
	} else if (this->pointIsInside(event.position)) {
		if (childCount > 0) { // Has a list of members to open, toggle expansion when clicked.
			if (this->subMenu) { // Menu within another menu.
				// Hover to expand sub-menu's list.
				if (event.mouseEventType == MouseEventType::MouseMove && !this->showingOverlay()) {
					this->createOverlay();
				}
			} else { // Top menu, which is usually placed in a toolbar.
				bool toggleExpansion = false;
				if (event.mouseEventType == MouseEventType::MouseDown) {
					// Toggle expansion when headImageMethod is clicked.
					toggleExpansion = true;
				} else if (event.mouseEventType == MouseEventType::MouseMove && !this->showingOverlay()) {
					// Automatically expand hovered top-menus neighboring an opened top menu.
					if (this->parent != nullptr) {
						VisualComponent *toolbar = this->parent;
						if (toolbar->ownsFocus()) {
							for (int i = 0; i < toolbar->getChildCount(); i++) {
								if (toolbar->children[i]->showingOverlay()) {
									toggleExpansion = true;
									break;
								}
							}
						}
					}
				}
				if (toggleExpansion) {
					// Menu components with child members will toggle visibility for its list when pressed.
					if (this->showingOverlay()) {
						closeEntireMenu(this);
					} else {
						this->createOverlay();
					}
				}
			}
		} else { // List item, because it has no children.
			// Childless menu components are treated as menu items that can be clicked to perform an action and close the menu.
			if (event.mouseEventType == MouseEventType::MouseDown) {
				// Hide overlays all the way to root.
				closeEntireMenu(this);
				// Call the event assigned to this menu item.
				this->callback_pressedEvent();
			}
		}
		// Because the main body was interacted with, the mouse events are passed on.
		VisualComponent::receiveMouseEvent(event);
	}
}

IVector2D Menu::getDesiredDimensions() {
	this->completeAssets();
	int widthAdder = this->padding.value * 2;
	int heightAdder = widthAdder;
	if (this->hasArrow()) {
		// Make extra space for the expansion arrowhead when containing a list of members.
		widthAdder += 24;
	}
	return IVector2D(font_getLineWidth(this->font, this->text.value) + widthAdder, font_getSize(this->font) + heightAdder);
}
