// zlib open source license
//
// Copyright (c) 2022 David Forsgren Piuva
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

#include "TextBox.h"
#include <functional>

using namespace dsr;

PERSISTENT_DEFINITION(TextBox)

void TextBox::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"BackColor");
	target.declareAttribute(U"ForeColor");
	target.declareAttribute(U"Text");
	target.declareAttribute(U"MultiLine");
}

Persistent* TextBox::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"Color") || string_caseInsensitiveMatch(name, U"BackColor")) {
		return &(this->backColor);
	} else if (string_caseInsensitiveMatch(name, U"ForeColor")) {
		return &(this->foreColor);
	} else if (string_caseInsensitiveMatch(name, U"Text")) {
		return &(this->text);
	} else if (string_caseInsensitiveMatch(name, U"MultiLine")) {
		return &(this->multiLine);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

TextBox::TextBox() {}

bool TextBox::isContainer() const {
	return false;
}

// Limit exclusive indices to the text.
void TextBox::limitSelection() {
	int64_t textLength = string_length(this->text.value);
	if (this->selectionStart < 0) this->selectionStart = 0;
	if (this->beamLocation < 0) this->beamLocation = 0;
	if (this->selectionStart > textLength) this->selectionStart = textLength;
	if (this->beamLocation > textLength) this->beamLocation = textLength;
}

static void tabJump(int64_t &x, int64_t tabWidth) {
	x += tabWidth - (x % tabWidth);
}

static int64_t monospacesPerTab = 4;

// Pre-condition: text does not contain any linebreak.
void iterateCharactersInLine(const ReadableString& text, const RasterFont &font, std::function<void(int64_t index, DsrChar code, int64_t left, int64_t right)> characterAction) {
	int64_t right = 0;
	int64_t monospaceWidth = font_getMonospaceWidth(font);
	int64_t tabWidth = monospaceWidth * monospacesPerTab;
	for (int64_t i = 0; i <= string_length(text); i++) {
		DsrChar code = text[i];
		int64_t left = right;
		if (code == U'\t') {
			tabJump(right, tabWidth);
		} else {
			right += monospaceWidth;
		}
		characterAction(i, code, left, right);
	}
}

// Iterate over the whole text once for both selection and characters.
// Returns the beam's X location in pixels.
int64_t printMonospaceLine(OrderedImageRgbaU8 &target, const ReadableString& text, const RasterFont &font, ColorRgbaI32 foreColor, bool focused, int64_t originX, int64_t selectionLeft, int64_t selectionRight, int64_t beamIndex, int64_t topY, int64_t bottomY) {
	int64_t characterHeight = bottomY - topY;
	int64_t beamPixelX = 0;
	iterateCharactersInLine(text, font, [&target, &font, &foreColor, &beamPixelX, originX, selectionLeft, selectionRight, beamIndex, topY, characterHeight, focused](int64_t index, DsrChar code, int64_t left, int64_t right) {
		left += originX;
		right += originX;
		if (index == beamIndex) beamPixelX = left;
		if (focused && selectionLeft <= index && index < selectionRight) {
			draw_rectangle(target, IRect(left, topY, right - left, characterHeight), ColorRgbaI32(0, 0, 100, 255));
			font_printCharacter(target, font, code, IVector2D(left, topY), ColorRgbaI32(255, 255, 255, 255));
		} else {
			font_printCharacter(target, font, code, IVector2D(left, topY), foreColor);
		}
	});
	return beamPixelX;
}

void TextBox::indexLines() {
	int64_t newLength = string_length(this->text.value);
	if (newLength != this->indexedAtLength) {
		int64_t currentLength = 0;
		int64_t worstCaseLength = 0;
		// Index the lines for fast scrolling and rendering.
		this->lines.clear();
		int64_t sectionStart = 0;
		for (int64_t i = 0; i <= newLength; i++) {
			DsrChar c = this->text.value[i];
			if (c == U'\n' || c == U'\0') {
				if (currentLength > worstCaseLength) {
					worstCaseLength = currentLength;
				}
				currentLength = 0;
				this->lines.pushConstruct(sectionStart, i);
				sectionStart = i + 1;
			} else if (c == U'\t') {
				currentLength += 4;
			} else {
				currentLength += 1;
			}
		}
		this->indexedAtLength = newLength;
		this->worstCaseLineMonospaces = worstCaseLength;
	}
}

LVector2D TextBox::getTextOrigin(bool includeVerticalScroll) {
	int64_t rowStride = font_getSize(this->font);
	int64_t offsetX = this->borderX - this->horizontalScrollBar.getValue();
	int64_t offsetY = 0;
	if (this->multiLine.value) {
		offsetY = this->borderY;
	} else {
		offsetY = (image_getHeight(this->image) - rowStride) / 2;
	}
	if (includeVerticalScroll) {
		offsetY -= this->verticalScrollBar.getValue() * rowStride;
	}
	return LVector2D(offsetX, offsetY);
}

// TODO: Reuse scaled background images as a separate layer.
// TODO: Allow using different colors for beam, selection, selected text, normal text...
//       Maybe ask a separate color palette for specific things using the specific class of textboxes.
//       Color palettes can be independent of the media machine, allowing them to be mixed freely with different themes.
//       Color palettes can be loaded together with the layout to instantly have the requested standard colors by name.
//       Color palettes can have a standard column order of input to easily pack multiple color themes into the same color palette image.
//         Just a long list of names for the different X coordinates and the user selects a Y coordinate as the color theme.
//         New components will have to use existing parts of the palette by keeping the names reusable.
//       Separate components should be able to override any color for programmability, but default values should refer to the current color palette.
//         If no color is assigned, the class will give it a standard color from the theme.
//         Should classes be separate for themes and palettes?
void TextBox::generateGraphics() {
	int32_t width = this->location.width();
	int32_t height = this->location.height();
	if (width < 1) { width = 1; }
	if (height < 1) { height = 1; }
	bool focused = this->isFocused();
	if (!this->hasImages || this->drawnAsFocused != focused) {
		this->hasImages = true;
		this->drawnAsFocused = focused;
		completeAssets();
		this->indexLines();
		ColorRgbaI32 foreColorRgba = ColorRgbaI32(this->foreColor.value, 255);
		// Create a scaled image
		component_generateImage(this->theme, this->textBox, width, height, this->backColor.value.red, this->backColor.value.green, this->backColor.value.blue, 0, focused ? 1 : 0)(this->image);
		this->limitSelection();
		LVector2D origin = this->getTextOrigin(false);
		int64_t rowStride = font_getSize(this->font);
		int64_t targetHeight = image_getHeight(this->image);
		int64_t firstVisibleLine = this->verticalScrollBar.getValue();

		// Find character indices for left and right sides.
		int64_t selectionLeft = std::min(this->selectionStart, this->beamLocation);
		int64_t selectionRight = std::max(this->selectionStart, this->beamLocation);
		bool hasSelection = selectionLeft < selectionRight;

		// Draw the text with selection and get the beam's pixel location.
		int64_t topY = origin.y;
		for (int64_t row = firstVisibleLine; row < this->lines.length() && topY < targetHeight; row++) {
			int64_t startIndex = this->lines[row].lineStartIndex;
			int64_t endIndex = this->lines[row].lineEndIndex;
			ReadableString currentLine = string_exclusiveRange(this->text.value, startIndex, endIndex);
			int64_t beamPixelX = printMonospaceLine(this->image, currentLine, this->font, foreColorRgba, focused, origin.x, selectionLeft - startIndex, selectionRight - startIndex, this->beamLocation - startIndex, topY, topY + rowStride);
			// Draw a beam if the textbox is focused.
			if (focused && this->beamLocation >= startIndex && this->beamLocation <= endIndex) {
				int64_t beamWidth = 2;
				draw_rectangle(this->image, IRect(beamPixelX - 1, topY - 1, beamWidth, rowStride + 2), hasSelection ? ColorRgbaI32(255, 255, 255, 255) : foreColorRgba);
			}
			topY += rowStride;
		}
		this->verticalScrollBar.draw(this->image, this->theme, this->backColor.value);
		this->horizontalScrollBar.draw(this->image, this->theme, this->backColor.value);
	}
}

void TextBox::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	this->generateGraphics();
	draw_copy(targetImage, this->image, relativeLocation.left(), relativeLocation.top());
}

int64_t TextBox::findBeamLocationInLine(int64_t rowIndex, int64_t pixelX) {
	LVector2D origin = this->getTextOrigin(true);
	// Clamp to the closest row if going outside.
	if (rowIndex < 0) rowIndex = 0;
	if (rowIndex >= this->lines.length()) rowIndex = this->lines.length() - 1;
	int64_t beamIndex = 0;
	int64_t closestDistance = 1000000000000;
	int64_t startIndex = this->lines[rowIndex].lineStartIndex;
	int64_t endIndex = this->lines[rowIndex].lineEndIndex;
	ReadableString currentLine = string_exclusiveRange(this->text.value, startIndex, endIndex);
	iterateCharactersInLine(currentLine, font, [&beamIndex, &closestDistance, &origin, pixelX](int64_t index, DsrChar code, int64_t left, int64_t right) {
		int64_t center = origin.x + (left + right) / 2;
		int64_t newDistance = std::abs(pixelX - center);
		if (newDistance < closestDistance) {
			beamIndex = index;
			closestDistance = newDistance;
		}
	});
	return startIndex + beamIndex;
}

BeamLocation TextBox::findBeamLocation(const LVector2D &pixelLocation) {
	LVector2D origin = this->getTextOrigin(true);
	int64_t rowStride = font_getSize(this->font);
	int64_t rowIndex = (pixelLocation.y - origin.y) / rowStride;
	return BeamLocation(rowIndex, this->findBeamLocationInLine(rowIndex, pixelLocation.x));
}

static int64_t findBeamRow(List<LineIndex> lines, int64_t beamLocation) {
	int64_t result = 0;
	for (int64_t row = 0; row < lines.length(); row++) {
		int64_t startIndex = lines[row].lineStartIndex;
		int64_t endIndex = lines[row].lineEndIndex;
		if (beamLocation >= startIndex && beamLocation <= endIndex) {
			result = row;
		}
	}
	return result;
}

// Returns the beam's pixel offset relative to the origin.
static int64_t getBeamPixelOffset(const ReadableString &text, const RasterFont &font, List<LineIndex> lines, const BeamLocation &beam) {
	int64_t result = 0;
	int64_t lineStartIndex = lines[beam.rowIndex].lineStartIndex;
	int64_t lineEndIndex = lines[beam.rowIndex].lineEndIndex;
	int64_t localBeamIndex = beam.characterIndex - lineStartIndex;
	ReadableString currentLine = string_exclusiveRange(text, lineStartIndex, lineEndIndex);
	iterateCharactersInLine(currentLine, font, [&result, localBeamIndex](int64_t index, DsrChar code, int64_t left, int64_t right) {
		if (index == localBeamIndex) result = left;
	});
	return result;
}

void TextBox::receiveMouseEvent(const MouseEvent& event) {
	bool verticalScrollIntercepted = this->verticalScrollBar.receiveMouseEvent(this->location, event);
	bool horizontalScrollIntercepted = this->horizontalScrollBar.receiveMouseEvent(this->location, event);
	bool scrollIntercepted = verticalScrollIntercepted || horizontalScrollIntercepted;
	if (event.mouseEventType == MouseEventType::MouseDown && !scrollIntercepted) {
		this->mousePressed = true;
		BeamLocation newBeam = findBeamLocation(LVector2D(event.position.x - this->location.left(), event.position.y - this->location.top()));
		if (newBeam.characterIndex != this->selectionStart || newBeam.characterIndex != this->beamLocation) {
			this->selectionStart = newBeam.characterIndex;
			this->beamLocation = newBeam.characterIndex;
			this->hasImages = false;
		}
	} else if (this->mousePressed && event.mouseEventType == MouseEventType::MouseMove) {
		if (this->mousePressed) {
			BeamLocation newBeam = findBeamLocation(LVector2D(event.position.x - this->location.left(), event.position.y - this->location.top()));
			if (newBeam.characterIndex != this->beamLocation) {
				this->beamLocation = newBeam.characterIndex;
				this->hasImages = false;
			}
		}
	} else if (this->mousePressed && event.mouseEventType == MouseEventType::MouseUp) {
		this->mousePressed = false;
	}
	if (scrollIntercepted) {
		this->hasImages = false; // Force redraw on scrollbar interception
	} else {
		VisualComponent::receiveMouseEvent(event);
	}
}

// TODO: Move stub implementation to an API and allow system wrappers to override it with a real implementation copying and pasting across different applications.
String pasteBinStub;
void saveToClipBoard(const ReadableString &text) {
	pasteBinStub = text;
}
ReadableString readFromClipBoard() {
	return pasteBinStub;
}

ReadableString TextBox::getSelectedText() {
	int64_t selectionLeft = std::min(this->selectionStart, this->beamLocation);
	int64_t selectionRight = std::max(this->selectionStart, this->beamLocation);
	return string_exclusiveRange(this->text.value, selectionLeft, selectionRight);
}

void TextBox::replaceSelection(const ReadableString &replacingText) {
	int64_t selectionLeft = std::min(this->selectionStart, this->beamLocation);
	int64_t selectionRight = std::max(this->selectionStart, this->beamLocation);
	this->text.value = string_combine(string_before(this->text.value, selectionLeft), replacingText, string_from(this->text.value, selectionRight));
	// Place beam on the right side of the replacement without selecting anything
	this->selectionStart = selectionLeft + string_length(replacingText);
	this->beamLocation = selectionStart;
	this->hasImages = false;
	this->indexedAtLength = -1;
	this->indexLines();
	this->limitScrolling(true);
}

void TextBox::replaceSelection(DsrChar replacingCharacter) {
	int64_t selectionLeft = std::min(this->selectionStart, this->beamLocation);
	int64_t selectionRight = std::max(this->selectionStart, this->beamLocation);
	String newText = string_before(this->text.value, selectionLeft);
	string_appendChar(newText, replacingCharacter);
	string_append(newText, string_from(this->text.value, selectionRight));
	this->text.value = newText;
	// Place beam on the right side of the replacement without selecting anything
	this->selectionStart = selectionLeft + 1;
	this->beamLocation = selectionStart;
	this->hasImages = false;
	this->indexedAtLength = -1;
	this->indexLines();
	this->limitScrolling(true);
}

void TextBox::placeBeamAtCharacter(int64_t characterIndex, bool removeSelection) {
	this->beamLocation = characterIndex;
	if (removeSelection) {
		this->selectionStart = characterIndex;
	}
	this->hasImages = false;
	this->limitScrolling(true);
}

void TextBox::moveBeamVertically(int64_t rowIndexOffset, bool removeSelection) {
	// Find the current beam's row index.
	int64_t oldRowIndex = findBeamRow(this->lines, this->beamLocation);
	// Find another row.
	int64_t newRowIndex = oldRowIndex + rowIndexOffset;
	if (newRowIndex < 0) { newRowIndex = 0; }
	if (newRowIndex >= this->lines.length()) { newRowIndex = this->lines.length() - 1; }
	// Get old pixel offset from the beam.
	LVector2D origin = this->getTextOrigin(true);
	BeamLocation oldBeam = BeamLocation(oldRowIndex, this->beamLocation);
	int64_t oldPixelOffset = origin.x + getBeamPixelOffset(this->text.value, this->font, this->lines, oldBeam);
	// Get the closest location in the new row.
	int64_t newCharacterIndex = findBeamLocationInLine(newRowIndex, oldPixelOffset);
	placeBeamAtCharacter(newCharacterIndex, removeSelection);
	limitScrolling(true);
}

static const uint32_t combinationKey_leftShift = 1 << 0;
static const uint32_t combinationKey_rightShift = 1 << 1;
static const uint32_t combinationKey_shift = combinationKey_leftShift | combinationKey_rightShift;
static const uint32_t combinationKey_leftControl = 1 << 2;
static const uint32_t combinationKey_rightControl = 1 << 3;
static const uint32_t combinationKey_control = combinationKey_leftControl | combinationKey_rightControl;

static int64_t getLineStart(const ReadableString &text, int64_t searchStart) {
	for (int64_t i = searchStart - 1; i >= 0; i--) {
		if (text[i] == U'\n') {
			return i + 1;
		}
	}
	return 0;
}

static int64_t getLineEnd(const ReadableString &text, int64_t searchStart) {
	for (int64_t i = searchStart; i < string_length(text); i++) {
		if (text[i] == U'\n') {
			return i;
		}
	}
	return string_length(text);
}

void TextBox::receiveKeyboardEvent(const KeyboardEvent& event) {
	// Insert and scroll-lock is not supported.
	if (event.keyboardEventType == KeyboardEventType::KeyDown) {
		if (event.dsrKey == DsrKey_LeftShift) {
			this->combinationKeys |= combinationKey_leftShift;
		} else if (event.dsrKey == DsrKey_RightShift) {
			this->combinationKeys |= combinationKey_rightShift;
		} else if (event.dsrKey == DsrKey_LeftControl) {
			this->combinationKeys |= combinationKey_leftControl;
		} else if (event.dsrKey == DsrKey_RightControl) {
			this->combinationKeys |= combinationKey_rightControl;
		}
	} else if (event.keyboardEventType == KeyboardEventType::KeyUp) {
		if (event.dsrKey == DsrKey_LeftShift) {
			this->combinationKeys &= ~combinationKey_leftShift;
		} else if (event.dsrKey == DsrKey_RightShift) {
			this->combinationKeys &= ~combinationKey_rightShift;
		} else if (event.dsrKey == DsrKey_LeftControl) {
			this->combinationKeys &= ~combinationKey_leftControl;
		} else if (event.dsrKey == DsrKey_RightControl) {
			this->combinationKeys &= ~combinationKey_rightControl;
		}
	} else if (event.keyboardEventType == KeyboardEventType::KeyType) {
		int64_t textLength = string_length(this->text.value);
		bool selected = this->selectionStart != this->beamLocation;
		bool printable = event.character == U'\t' || (31 < event.character && event.character < 127) || 159 < event.character;
		bool canGoLeft = textLength > 0 && this->beamLocation > 0;
		bool canGoRight = textLength > 0 && this->beamLocation < textLength;
		bool holdShift = this->combinationKeys & combinationKey_shift;
		bool holdControl = this->combinationKeys & combinationKey_control;
		bool removeSelection = !holdShift;
		if (holdControl) {
			if (event.dsrKey == DsrKey_LeftArrow) {
				// Move to the line start using Ctrl + LeftArrow instead of Home
				this->placeBeamAtCharacter(getLineStart(this->text.value, this->beamLocation), removeSelection);
			} else if (event.dsrKey == DsrKey_RightArrow) {
				// Move to the line end using Ctrl + RightArrow instead of End
				this->placeBeamAtCharacter(getLineEnd(this->text.value, this->beamLocation), removeSelection);
			} else if (event.dsrKey == DsrKey_X) {
				// Cut selection using Ctrl + X
				saveToClipBoard(this->getSelectedText());
				this->replaceSelection(U"");
			} else if (event.dsrKey == DsrKey_C) {
				// Copy selection using Ctrl + C
				saveToClipBoard(this->getSelectedText());
			} else if (event.dsrKey == DsrKey_V) {
				// Paste selection using Ctrl + V
				this->replaceSelection(readFromClipBoard());
			} else if (event.dsrKey == DsrKey_A) {
				// Select all using Ctrl + A
				this->selectionStart = 0;
				this->beamLocation = string_length(this->text.value);
				this->hasImages = false;
			} else if (event.dsrKey == DsrKey_N) {
				// Select nothing using Ctrl + N
				this->selectionStart = this->beamLocation;
				this->hasImages = false;
			}
		} else {
			if (selected && (event.dsrKey == DsrKey_BackSpace || event.dsrKey == DsrKey_Delete)) {
				// Remove selection
				this->replaceSelection(U"");
			} else if (event.dsrKey == DsrKey_BackSpace && canGoLeft) {
				// Erase left of beam
				this->beamLocation--;
				this->replaceSelection(U"");
			} else if (event.dsrKey == DsrKey_Delete && canGoRight) {
				// Erase right of beam
				this->beamLocation++;
				this->replaceSelection(U"");
			} else if (event.dsrKey == DsrKey_Home) {
				// Move to the line start using Home
				this->placeBeamAtCharacter(getLineStart(this->text.value, this->beamLocation), removeSelection);
			} else if (event.dsrKey == DsrKey_End) {
				// Move to the line end using End
				this->placeBeamAtCharacter(getLineEnd(this->text.value, this->beamLocation), removeSelection);
			} else if (event.dsrKey == DsrKey_LeftArrow && canGoLeft) {
				// Move left using LeftArrow
				this->placeBeamAtCharacter(this->beamLocation - 1, removeSelection);
			} else if (event.dsrKey == DsrKey_RightArrow && canGoRight) {
				// Move right using RightArrow
				this->placeBeamAtCharacter(this->beamLocation + 1, removeSelection);
			} else if (event.dsrKey == DsrKey_UpArrow) {
				// Move up using UpArrow
				this->moveBeamVertically(-1, removeSelection);
			} else if (event.dsrKey == DsrKey_DownArrow) {
				// Move down using DownArrow
				this->moveBeamVertically(1, removeSelection);
			} else if (event.dsrKey == DsrKey_Return) {
				if (this->multiLine.value) {
					this->replaceSelection(U'\n');
				}
			} else if (printable) {
				this->replaceSelection(event.character);
			}
		}
		//printText(U"KeyType char=", event.character, " key=", event.dsrKey, U"\n");
	}
	VisualComponent::receiveKeyboardEvent(event);
}

bool TextBox::pointIsInside(const IVector2D& pixelPosition) {
	this->generateGraphics();
	// Get the point relative to the component instead of its direct container
	IVector2D localPoint = pixelPosition - this->location.upperLeft();
	// Sample opacity at the location
	return dsr::image_readPixel_border(this->image, localPoint.x, localPoint.y).alpha > 127;
}

void TextBox::changedTheme(VisualTheme newTheme) {
	this->textBox = theme_getScalableImage(newTheme, U"TextBox");
	this->verticalScrollBar.loadTheme(newTheme, this->backColor.value);
	this->horizontalScrollBar.loadTheme(newTheme, this->backColor.value);
	this->hasImages = false;
}

void TextBox::loadFont() {
	if (!font_exists(this->font)) {
		this->font = font_getDefault();
	}
	if (!font_exists(this->font)) {
		throwError("Failed to load the default font for a ListBox!\n");
	}
}

void TextBox::completeAssets() {
	if (this->textBox.methodIndex == -1) {
		VisualTheme newTheme = theme_getDefault();
		this->textBox = theme_getScalableImage(newTheme, U"TextBox");
		this->verticalScrollBar.loadTheme(newTheme, this->backColor.value);
		this->horizontalScrollBar.loadTheme(newTheme, this->backColor.value);
	}
	this->loadFont();
}

void TextBox::changedLocation(const IRect &oldLocation, const IRect &newLocation) {
	// If the component has changed dimensions then redraw the image
	if (oldLocation.size() != newLocation.size()) {
		this->hasImages = false;
		this->limitScrolling(true);
	}
}

void TextBox::changedAttribute(const ReadableString &name) {
	if (!string_caseInsensitiveMatch(name, U"Visible")) {
		this->hasImages = false;
		if (string_caseInsensitiveMatch(name, U"Text")) {
			this->indexedAtLength = -1;
			this->limitSelection();
			this->limitScrolling(true);
		}
	}
	VisualComponent::changedAttribute(name);
}

void TextBox::updateScrollRange() {
	this->loadFont();
	// How high is one element?
	int64_t verticalStep = font_getSize(this->font);
	// How many elements are visible at the same time?
	int64_t visibleRangeY = (this->location.height() - this->borderY * 2) / verticalStep;
	if (visibleRangeY < 1) visibleRangeY = 1;
	// How many lines are there in total to see.
	int64_t itemCount = this->lines.length() + 1; // Reserve an extra line for the horizontal scroll-bar.
	// The range of indices that the listbox can start viewing from.
	int64_t maxScrollY = itemCount - visibleRangeY;
	// If visible range exceeds the collection, we should still allow starting element zero to get a valid range.
	if (maxScrollY < 0) maxScrollY = 0;
	// Apply the scroll range.
	this->verticalScrollBar.updateScrollRange(ScrollRange(0, maxScrollY, visibleRangeY));
	// Calculate range for horizontal scroll.
	int64_t monospaceWidth = font_getMonospaceWidth(this->font);
	int64_t rightMostPixel = this->worstCaseLineMonospaces * monospaceWidth;
	int64_t visibleRangeX = this->location.width() - this->borderX * 2;
	if (visibleRangeX < 1) visibleRangeX = 1;
	int64_t maxScrollX = rightMostPixel; // Allow scrolling all the way out, so that one can write left to right without constantly panorating on a long line.
	if (maxScrollX < 0) maxScrollX = 0;
	this->horizontalScrollBar.updateScrollRange(ScrollRange(0, maxScrollX, visibleRangeX));
}

void TextBox::limitScrolling(bool keepBeamVisible) {
	// Update the scroll range.
	this->indexLines();
	this->updateScrollRange();
	// Limit scrolling with the updated range.
	if (keepBeamVisible) {
		int64_t beamRow = findBeamRow(this->lines, this->beamLocation);
		BeamLocation beam = BeamLocation(beamRow, this->beamLocation);
		// What will origin.x be used for?
		int64_t pixelOffsetX = getBeamPixelOffset(this->text.value, this->font, this->lines, beam);
		this->verticalScrollBar.limitScrolling(this->location, true, beamRow);
		this->horizontalScrollBar.limitScrolling(this->location, true, pixelOffsetX);
	} else {
		this->verticalScrollBar.limitScrolling(this->location);
		this->horizontalScrollBar.limitScrolling(this->location);
	}
}
