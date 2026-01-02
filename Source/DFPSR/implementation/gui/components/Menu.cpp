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
	target.declareAttribute(U"HeadClass");
	target.declareAttribute(U"ListClass");
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
	} else if (string_caseInsensitiveMatch(name, U"HeadClass") || string_caseInsensitiveMatch(name, U"Class")) {
		// Class is an alias for HeadClass.
		return &(this->headClass);
	} else if (string_caseInsensitiveMatch(name, U"ListClass")) {
		return &(this->listClass);
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

static OrderedImageRgbaU8 generateHeadImage(Menu &menu, MediaMethod imageGenerator, int32_t pressed, int32_t focused, int32_t hover, int32_t width, int32_t height, ColorRgbI32 backColor, ColorRgbI32 foreColor, const ReadableString &text, RasterFont font) {
	// Create a scaled image
	OrderedImageRgbaU8 result;
 	component_generateImage(menu.getTheme(), imageGenerator, width, height, backColor.red, backColor.green, backColor.blue, pressed, focused, hover)(result);
	if (string_length(text) > 0) {
		int32_t backWidth = image_getWidth(result);
		int32_t backHeight = image_getHeight(result);
		int32_t left = menu.padding.value;
		int32_t top = (backHeight - font_getSize(font)) / 2;
		if (pressed) {
			top += 1;
		}
		// Print the text
		font_printLine(result, font, text, IVector2D(left, top), ColorRgbaI32(foreColor, 255));
		// Draw the arrow
		if (menu.hasArrow()) {
			int32_t arrowWidth = image_getWidth(arrowImage);
			int32_t arrowHeight = image_getHeight(arrowImage);
			int32_t arrowLeft = backWidth - arrowWidth - 4;
			int32_t arrowTop = (backHeight - arrowHeight) / 2;
			draw_silhouette(result, arrowImage, ColorRgbaI32(foreColor, 255), arrowLeft, arrowTop);
		}
	}
	return result;
}

void Menu::generateGraphics() {
	int32_t headWidth = this->location.width();
	int32_t headHeight = this->location.height();
	if (headWidth < 1) { headWidth = 1; }
	if (headHeight < 1) { headHeight = 1; }
	// headImage is set to an empty handle when something used as input changes.
	if (!image_exists(this->headImage)) {
		completeAssets();
		bool focus = this->isFocused();
		bool hover = this->isHovered();
		bool press = this->pressed && hover;
		this->headImage = generateHeadImage(*this, this->headImageMethod, press, focus, hover, headWidth, headHeight, this->backColor.value, this->foreColor.value, this->text.value, this->font);
	}
}

// Fill the listBackgroundImageMethod with a solid color
void Menu::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	this->generateGraphics();
	if (this->menuHead_filter == 1) {
		draw_alphaFilter(targetImage, this->headImage, relativeLocation.left(), relativeLocation.top());
	} else {
		draw_copy(targetImage, this->headImage, relativeLocation.left(), relativeLocation.top());
	}
}

void Menu::generateBackground() {
	if (!image_exists(this->listBackgroundImage)) {
		int32_t listWidth = this->overlayLocation.width();
		int32_t listHeight = this->overlayLocation.height();
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
		for (int32_t i = 1; i < this->getChildCount(); i++) {
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
	IVector2D overlayOffset = absoluteOffset + this->overlayLocation.upperLeft();
	if (this->menuList_filter == 1) {
		draw_alphaFilter(targetImage, this->listBackgroundImage, overlayOffset.x, overlayOffset.y);
	} else {
		draw_copy(targetImage, this->listBackgroundImage, overlayOffset.x, overlayOffset.y);
	}
	for (int32_t i = 0; i < this->getChildCount(); i++) {
		this->children[i]->draw(targetImage, absoluteOffset + this->location.upperLeft());
	}
}

void Menu::loadTheme(const VisualTheme &theme) {
	// Is it a sub-menu or top menu?
	this->subMenu = this->parent != nullptr && dynamic_cast<Menu*>(this->parent) != nullptr;
	this->finalHeadClass = theme_selectClass(theme, this->headClass.value, this->subMenu ? U"MenuSub" : U"MenuTop");
	this->finalListClass = theme_selectClass(theme, this->listClass.value, U"MenuList");
	this->headImageMethod = theme_getScalableImage(theme, this->finalHeadClass);
	// Check which states the scalable head image is listening to.
	this->headStateListenerMask = theme_getStateListenerMask(this->headImageMethod);
	this->listBackgroundImageMethod = theme_getScalableImage(theme, this->finalListClass);
	// Ask the theme which parts should be drawn using alpha filtering, and fall back on solid drawing.
	this->menuHead_filter = theme_getInteger(theme, this->finalHeadClass, U"Filter", 0);
	this->menuList_filter = theme_getInteger(theme, this->finalListClass, U"Filter", 0);
}

void Menu::changedTheme(VisualTheme newTheme) {
	this->loadTheme(newTheme);
	this->headImage = OrderedImageRgbaU8();
}

void Menu::completeAssets() {
	if (this->headImageMethod.methodIndex == -1) {
		this->loadTheme(theme_getDefault());
	}
	if (this->font.isNull()) {
		this->font = font_getDefault();
	}
}

void Menu::changedLocation(const IRect &oldLocation, const IRect &newLocation) {
	// If the component has changed dimensions then redraw the image
	if (oldLocation.size() != newLocation.size()) {
		this->headImage = OrderedImageRgbaU8();
	}
}

void Menu::changedAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"HeadClass")
	 || string_caseInsensitiveMatch(name, U"ListClass")) {
		// Update from the theme if a theme class has changed.
		this->changedTheme(this->getTheme());
	} else if (!string_caseInsensitiveMatch(name, U"Visible")) {
		this->headImage = OrderedImageRgbaU8();
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
	// Check which states have changed.
	ComponentState changedStates = newState ^ oldState;
	// TODO: Debug print to see if this works and create themes that respond to direct focus and hover states using color modifications as a simple start.
	// Check if any of the changed bits overlap with the states that the head's scalable image generator uses as input.
	if (changedStates & this->headStateListenerMask) {
		// If a state affecting the input has changed, the image should be updated.
		// The pressed argument can also be requested by the scalable images, but that is handled by components themselves.
		this->headImage = OrderedImageRgbaU8();
	}
	// When pressed, changes in hover will affect if the component will appear pressed,
	//   so show that one can safely abort a press done by mistake by releasing outside.
	if (this->pressed && (changedStates & componentState_hoverDirect)) {
		// If a state affecting the input has changed, the image should be updated.
		// The pressed argument can also be requested by the scalable images, but that is handled by components themselves.
		this->headImage = OrderedImageRgbaU8();
	}
}

void Menu::updateLocationEvent(const IRect& oldLocation, const IRect& newLocation) {	
	int32_t left = this->padding.value;
	int32_t top = this->padding.value;
	int32_t overlap = 3;
	if (this->subMenu) {
		left += newLocation.width() - overlap;
	} else {
		top += newLocation.height() - overlap;
	}
	int32_t maxWidth = 80; // Minimum usable with.
	// Expand list with to fit child components.
	for (int32_t i = 0; i < this->getChildCount(); i++) {
		int32_t width = this->children[i]->getDesiredDimensions().x;
		if (maxWidth < width) maxWidth = width;
	}
	// Stretch out the child components to use the whole width.
	for (int32_t i = 0; i < this->getChildCount(); i++) {
		int32_t height = this->children[i]->getDesiredDimensions().y;
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
	int32_t childCount = this->getChildCount();
	MouseEvent localEvent = event;
	localEvent.position -= this->location.upperLeft();
	bool inOverlay = this->showingOverlay() && this->pointIsInsideOfOverlay(event.position);
	bool inHead = this->pointIsInside(event.position);
	if (event.mouseEventType == MouseEventType::MouseUp) {
		// Pass on mouse up events to dragged components, even if not inside of them.
		if (this->dragComponent.isNotNull()) {
			MouseEvent childEvent = localEvent;
			childEvent.position -= this->dragComponent->location.upperLeft();
			this->dragComponent->sendMouseEvent(childEvent, true);
		}
	} else if (inOverlay) {
		// Pass on down and move events to a child component that the cursor is inside of.
		for (int32_t i = childCount - 1; i >= 0; i--) {
			if (this->children[i]->pointIsInside(localEvent.position)) {
				MouseEvent childEvent = localEvent;
				childEvent.position -= this->children[i]->location.upperLeft();
				if (event.mouseEventType == MouseEventType::MouseDown) {
					this->dragComponent = this->children[i];
					this->dragComponent->makeFocused();
				}
				this->children[i]->sendMouseEvent(childEvent, true);
				break;
			}
		}
	}
	// If not interacting with the overlay and the cursor is within the head.
	if (!inOverlay && inHead) {
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
							for (int32_t i = 0; i < toolbar->getChildCount(); i++) {
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
			if ((event.mouseEventType == MouseEventType::MouseDown) && !this->pressed) {
				// Show that the event is about to be triggered.
				this->pressed = true;
				// Update the head image.
				this->headImage = OrderedImageRgbaU8();
			} else if ((event.mouseEventType == MouseEventType::MouseUp) && this->pressed) {
				// Released a press inside, confirming the event.
				// Hide overlays all the way to root.
				closeEntireMenu(this);
				// Call the event assigned to this menu item.
				this->callback_pressedEvent();
			}
		}
		// Because the main body was interacted with, the basic up/down/move/scroll mouse events are triggered.
		VisualComponent::receiveMouseEvent(event);
	}
	// Releasing anywhere should stop pressing.
	if (event.mouseEventType == MouseEventType::MouseUp) {
		this->dragComponent = Handle<VisualComponent>();
		if (this->pressed) {
			// No longer pressed.
			this->pressed = false;
			// Update the head image.
			this->headImage = OrderedImageRgbaU8();
		}
	}
}

IVector2D Menu::getDesiredDimensions() {
	this->completeAssets();
	int32_t widthAdder = this->padding.value * 2;
	int32_t heightAdder = widthAdder;
	if (this->hasArrow()) {
		// Make extra space for the expansion arrowhead when containing a list of members.
		widthAdder += 24;
	}
	return IVector2D(font_getLineWidth(this->font, this->text.value) + widthAdder, font_getSize(this->font) + heightAdder);
}
