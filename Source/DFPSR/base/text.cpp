// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
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

#include "text.h"
#include <fstream>
#include <streambuf>
#include <cstring>
#include <stdexcept>

using namespace dsr;

static int strlen_utf32(const char32_t *content) {
	int length = 0;
	while (content[length] != 0) {
		length++;
	}
	return length;
}

static char toAscii(DsrChar c) {
	if (c > 127) {
		return '?';
	} else {
		return c;
	}
}

String& Printable::toStream(String& target) const {
	return this->toStreamIndented(target, U"");
}

String Printable::toStringIndented(const ReadableString& indentation) const {
	String result;
	this->toStreamIndented(result, indentation);
	return result;
}

String Printable::toString() const {
	return this->toStringIndented(U"");
}

std::ostream& Printable::toStreamIndented(std::ostream& out, const ReadableString& indentation) const {
	String result;
	this->toStreamIndented(result, indentation);
	for (int i = 0; i < result.length(); i++) {
		out.put(toAscii(result.read(i)));
	}
	return out;
}

std::ostream& Printable::toStream(std::ostream& out) const {
	return this->toStreamIndented(out, U"");
}

std::string Printable::toStdString() const {
	std::ostringstream result;
	this->toStream(result);
	return result.str();
}

Printable::~Printable() {}

bool dsr::string_match(const ReadableString& a, const ReadableString& b) {
	if (a.length() != b.length()) {
		return false;
	} else {
		for (int i = 0; i < a.length(); i++) {
			if (a.read(i) != b.read(i)) {
				return false;
			}
		}
		return true;
	}
}

bool dsr::string_caseInsensitiveMatch(const ReadableString& a, const ReadableString& b) {
	if (a.length() != b.length()) {
		return false;
	} else {
		for (int i = 0; i < a.length(); i++) {
			if (towupper(a.read(i)) != towupper(b.read(i))) {
				return false;
			}
		}
		return true;
	}
}

std::ostream& ReadableString::toStream(std::ostream& out) const {
	for (int i = 0; i < this->length(); i++) {
		out.put(toAscii(this->read(i)));
	}
	return out;
}

std::string ReadableString::toStdString() const {
	std::ostringstream result;
	this->toStream(result);
	return result.str();
}

String dsr::string_upperCase(const ReadableString &text) {
	String result;
	result.reserve(text.length());
	for (int i = 0; i < text.length(); i++) {
		result.appendChar(towupper(text[i]));
	}
	return result;
}

String dsr::string_lowerCase(const ReadableString &text) {
	String result;
	result.reserve(text.length());
	for (int i = 0; i < text.length(); i++) {
		result.appendChar(towlower(text[i]));
	}
	return result;
}

String dsr::string_removeAllWhiteSpace(const ReadableString &text) {
	String result;
	result.reserve(text.length());
	for (int i = 0; i < text.length(); i++) {
		DsrChar c = text[i];
		if (!character_isWhiteSpace(c)) {
			result.appendChar(c);
		}
	}
	return result;
}

ReadableString dsr::string_removeOuterWhiteSpace(const ReadableString &text) {
	int first = -1;
	int last = -1;
	for (int i = 0; i < text.length(); i++) {
		DsrChar c = text[i];
		if (!character_isWhiteSpace(c)) {
			first = i;
			break;
		}
	}
	for (int i = text.length() - 1; i >= 0; i--) {
		DsrChar c = text[i];
		if (!character_isWhiteSpace(c)) {
			last = i;
			break;
		}
	}
	if (first == -1) {
		// Only white space
		return ReadableString();
	} else {
		// Subset
		return string_inclusiveRange(text, first, last);
	}
}

String dsr::string_mangleQuote(const ReadableString &rawText) {
	String result;
	result.reserve(rawText.length() + 2);
	result.appendChar(U'\"'); // Begin quote
	for (int i = 0; i < rawText.length(); i++) {
		DsrChar c = rawText[i];
		if (c == U'\"') { // Double quote
			result.append(U"\\\"");
		} else if (c == U'\\') { // Backslash
			result.append(U"\\\\");
		} else if (c == U'\a') { // Audible bell
			result.append(U"\\a");
		} else if (c == U'\b') { // Backspace
			result.append(U"\\b");
		} else if (c == U'\f') { // Form feed
			result.append(U"\\f");
		} else if (c == U'\n') { // Line feed
			result.append(U"\\n");
		} else if (c == U'\r') { // Carriage return
			result.append(U"\\r");
		} else if (c == U'\t') { // Horizontal tab
			result.append(U"\\t");
		} else if (c == U'\v') { // Vertical tab
			result.append(U"\\v");
		} else if (c == U'\0') { // Null terminator
			result.append(U"\\0");
		} else {
			result.appendChar(c);
		}
	}
	result.appendChar(U'\"'); // End quote
	return result;
}

String dsr::string_unmangleQuote(const ReadableString& mangledText) {
	int firstQuote = string_findFirst(mangledText, '\"');
	int lastQuote = string_findLast(mangledText, '\"');
	String result;
	if (firstQuote == -1 || lastQuote == -1 || firstQuote == lastQuote) {
		throwError(U"Cannot unmangle using string_unmangleQuote without beginning and ending with quote signs!\n", mangledText, "\n");
	} else {
		for (int i = firstQuote + 1; i < lastQuote; i++) {
			DsrChar c = mangledText[i];
			if (c == U'\\') { // Escape character
				DsrChar c2 = mangledText[i + 1];
				if (c2 == U'\"') { // Double quote
					result.appendChar(U'\"');
				} else if (c2 == U'\\') { // Back slash
					result.appendChar(U'\\');
				} else if (c2 == U'a') { // Audible bell
					result.appendChar(U'\a');
				} else if (c2 == U'b') { // Backspace
					result.appendChar(U'\b');
				} else if (c2 == U'f') { // Form feed
					result.appendChar(U'\f');
				} else if (c2 == U'n') { // Line feed
					result.appendChar(U'\n');
				} else if (c2 == U'r') { // Carriage return
					result.appendChar(U'\r');
				} else if (c2 == U't') { // Horizontal tab
					result.appendChar(U'\t');
				} else if (c2 == U'v') { // Vertical tab
					result.appendChar(U'\v');
				} else if (c2 == U'0') { // Null terminator
					result.appendChar(U'\0');
				}
				i++; // Consume both characters
			} else {
				// Detect bad input
				if (c == U'\"') { // Double quote
					 throwError(U"Unmangled double quote sign detected in string_unmangleQuote!\n", mangledText, "\n");
				} else if (c == U'\\') { // Back slash
					 throwError(U"Unmangled back slash detected in string_unmangleQuote!\n", mangledText, "\n");
				} else if (c == U'\a') { // Audible bell
					 throwError(U"Unmangled audible bell detected in string_unmangleQuote!\n", mangledText, "\n");
				} else if (c == U'\b') { // Backspace
					 throwError(U"Unmangled backspace detected in string_unmangleQuote!\n", mangledText, "\n");
				} else if (c == U'\f') { // Form feed
					 throwError(U"Unmangled form feed detected in string_unmangleQuote!\n", mangledText, "\n");
				} else if (c == U'\n') { // Line feed
					 throwError(U"Unmangled line feed detected in string_unmangleQuote!\n", mangledText, "\n");
				} else if (c == U'\r') { // Carriage return
					 throwError(U"Unmangled carriage return detected in string_unmangleQuote!\n", mangledText, "\n");
				} else if (c == U'\0') { // Null terminator
					 throwError(U"Unmangled null terminator detected in string_unmangleQuote!\n", mangledText, "\n");
				} else {
					result.appendChar(c);
				}
			}
		}
	}
	return result;
}

static void uintToString_arabic(String& target, uint64_t value) {
	static const int bufferSize = 20;
	DsrChar digits[bufferSize];
	int usedSize = 0;
	if (value == 0) {
		target.appendChar(U'0');
	} else {
		while (usedSize < bufferSize) {
			DsrChar digit = U'0' + (value % 10u);
			digits[usedSize] = digit;
			usedSize++;
			value /= 10u;
			if (value == 0) {
				break;
			}
		}
		while (usedSize > 0) {
			usedSize--;
			target.appendChar(digits[usedSize]);
		}
	}
}

static void intToString_arabic(String& target, int64_t value) {
	if (value >= 0) {
		uintToString_arabic(target, (uint64_t)value);
	} else {
		target.appendChar(U'-');
		uintToString_arabic(target, (uint64_t)(-value));
	}
}

// TODO: Implement own version to ensure that nothing strange is happening from buggy std implementations
static void doubleToString_arabic(String& target, double value) {
	std::ostringstream buffer;
	buffer << std::fixed << value; // Generate using a fixed number of decimals
	std::string result = buffer.str();
	// Remove trailing zero decimal digits
	int decimalCount = 0;
	int lastValueIndex = -1;
	for (int c = 0; c < (int)result.length(); c++) {
		if (result[c] == '.') {
			decimalCount++;
		} else if (result[c] == ',') {
			result[c] = '.'; // Convert nationalized french decimal serialization into international decimals
			decimalCount++;
		} else if (decimalCount > 0 && result[c] >= '1' && result[c] <= '9') {
			lastValueIndex = c;
		} else  if (decimalCount == 0 && result[c] >= '0' && result[c] <= '9') {
			lastValueIndex = c;
		}
	}
	for (int c = 0; c <= lastValueIndex; c++) {
		target.appendChar(result[c]);
	}
}

#define TO_RAW_ASCII(TARGET, SOURCE) \
	char TARGET[SOURCE.length() + 1]; \
	for (int i = 0; i < SOURCE.length(); i++) { \
		TARGET[i] = toAscii(SOURCE[i]); \
	} \
	TARGET[SOURCE.length()] = '\0';

static inline void byteToStream(std::ostream &target, uint8_t value) {
	target.write((const char*)&value, 1);
}

// A function definition for receiving a stream of UTF-32 characters
//   Instead of using std's messy inheritance
using UTF32WriterFunction = std::function<void(DsrChar character)>;

// Filter out unwanted characters for improved portability
static void feedCharacter(const UTF32WriterFunction &reciever, DsrChar character) {
	if (character != U'\r') {
		reciever(character);
	}
}

// Appends the content of buffer as a BOM-free Latin-1 file into target
static void feedStringFromFileBuffer_Latin1(const UTF32WriterFunction &reciever, const uint8_t* buffer, int64_t fileLength) {
	for (int64_t i = 0; i < fileLength; i++) {
		DsrChar character = (DsrChar)(buffer[i]);
		if (character != U'\r') {
			feedCharacter(reciever, character);
		}
	}
}
// Appends the content of buffer as a BOM-free UTF-8 file into target
static void feedStringFromFileBuffer_UTF8(const UTF32WriterFunction &reciever, const uint8_t* buffer, int64_t fileLength) {
	for (int64_t i = 0; i < fileLength; i++) {
		uint8_t byteA = buffer[i];
		if (byteA < 0b10000000) {
			// Single byte (1xxxxxxx)
			feedCharacter(reciever, (DsrChar)byteA);
		} else {
			uint32_t character = 0;
			int extraBytes = 0;
			if (byteA >= 0b11000000) { // At least two leading ones
				if (byteA < 0b11100000) { // Less than three leading ones
					character = byteA & 0b00011111;
					extraBytes = 1;
				} else if (byteA < 0b11110000) { // Less than four leading ones
					character = byteA & 0b00011111;
					extraBytes = 2;
				} else if (byteA < 0b11111000) { // Less than five leading ones
					character = byteA & 0b00011111;
					extraBytes = 3;
				} else {
					// Invalid UTF-8 format
					throwError(U"Invalid UTF-8 multi-chatacter beginning with 0b111111xx!");
				}
			} else {
				// Invalid UTF-8 format
				throwError(U"Invalid UTF-8 multi-chatacter beginning with 0b10xxxxxx!");
			}
			while (extraBytes > 0) {
				i += 1; uint32_t nextByte = buffer[i];
				character = (character << 6) | (nextByte & 0b00111111);
				extraBytes--;
			}
			feedCharacter(reciever, (DsrChar)character);
		}
	}
}

template <bool LittleEndian>
uint16_t read16bits(const uint8_t* buffer, int startOffset) {
	uint16_t byteA = buffer[startOffset];
	uint16_t byteB = buffer[startOffset + 1];
	if (LittleEndian) {
		return (byteB << 8) | byteA;
	} else {
		return (byteA << 8) | byteB;
	}
}

// Appends the content of buffer as a BOM-free UTF-16 file into target
template <bool LittleEndian>
static void feedStringFromFileBuffer_UTF16(const UTF32WriterFunction &reciever, const uint8_t* buffer, int64_t fileLength) {
	for (int64_t i = 0; i < fileLength; i += 2) {
		// Read the first 16-bit word
		uint16_t wordA = read16bits<LittleEndian>(buffer, i);
		// Check if another word is needed
		//   Assuming that wordA >= 0x0000 and wordA <= 0xFFFF as uint16_t,
		//   we can just check if it's within the range reserved for 32-bit encoding
		if (wordA <= 0xD7FF || wordA >= 0xE000) {
			// Not in the reserved range, just a single 16-bit character
			feedCharacter(reciever, (DsrChar)wordA);
		} else {
			// The given range was reserved and therefore using 32 bits
			i += 2;
			uint16_t wordB = read16bits<LittleEndian>(buffer, i);
			uint32_t higher10Bits = wordA & 0b1111111111;
			uint32_t lower10Bits = wordB & 0b1111111111;
			feedCharacter(reciever, (DsrChar)(((higher10Bits << 10) | lower10Bits) + 0x10000));
		}
	}
}
// Appends the content of buffer as a text file of unknown format into target
static void feedStringFromFileBuffer(const UTF32WriterFunction &reciever, const uint8_t* buffer, int64_t fileLength) {
	// After removing the BOM bytes, the rest can be seen as a BOM-free text file with a known format
	if (fileLength >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) { // UTF-8
		feedStringFromFileBuffer_UTF8(reciever, buffer + 3, fileLength - 3);
	} else if (fileLength >= 2 && buffer[0] == 0xFE && buffer[1] == 0xFF) { // UTF-16 BE
		feedStringFromFileBuffer_UTF16<false>(reciever, buffer + 2, fileLength - 2);
	} else if (fileLength >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE) { // UTF-16 LE
		feedStringFromFileBuffer_UTF16<true>(reciever, buffer + 2, fileLength - 2);
	} else if (fileLength >= 4 && buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0xFE && buffer[3] == 0xFF) { // UTF-32 BE
		//feedStringFromFileBuffer_UTF32BE(target, buffer + 4, fileLength - 4);
		throwError(U"UTF-32 BE format is not yet supported!\n");
	} else if (fileLength >= 4 && buffer[0] == 0xFF && buffer[1] == 0xFE && buffer[2] == 0x00 && buffer[3] == 0x00) { // UTF-32 LE
		//feedStringFromFileBuffer_UTF32BE(target, buffer + 4, fileLength - 4);
		throwError(U"UTF-32 LE format is not yet supported!\n");
	} else if (fileLength >= 3 && buffer[0] == 0xF7 && buffer[1] == 0x64 && buffer[2] == 0x4C) { // UTF-1
		//feedStringFromFileBuffer_UTF1(target, buffer + 3, fileLength - 3);
		throwError(U"UTF-1 format is not yet supported!\n");
	} else if (fileLength >= 3 && buffer[0] == 0x0E && buffer[1] == 0xFE && buffer[2] == 0xFF) { // SCSU
		//feedStringFromFileBuffer_SCSU(target, buffer + 3, fileLength - 3);
		throwError(U"SCSU format is not yet supported!\n");
	} else if (fileLength >= 3 && buffer[0] == 0xFB && buffer[1] == 0xEE && buffer[2] == 0x28) { // BOCU
		//feedStringFromFileBuffer_BOCU-1(target, buffer + 3, fileLength - 3);
		throwError(U"BOCU-1 format is not yet supported!\n");
	} else if (fileLength >= 4 && buffer[0] == 0x2B && buffer[1] == 0x2F && buffer[2] == 0x76) { // UTF-7
		// Ignoring fourth byte with the dialect of UTF-7 when just showing the error message
		throwError(U"UTF-7 format is not yet supported!\n");
	} else {
		// No BOM detected, assuming Latin-1 (because it directly corresponds to a unicode sub-set)
		feedStringFromFileBuffer_Latin1(reciever, buffer, fileLength);
	}
}

String dsr::string_loadFromMemory(const Buffer &fileContent) {
	String result;
	// Measure the size of the result by scanning the content in advance
	int64_t characterCount = 0;
	UTF32WriterFunction measurer = [&characterCount](DsrChar character) {
		characterCount++;
	};
	feedStringFromFileBuffer(measurer, fileContent.getUnsafeData(), fileContent.size);
	// Pre-allocate the correct amount of memory based on the simulation
	result.reserve(characterCount);
	// Stream output to the result string
	UTF32WriterFunction reciever = [&result](DsrChar character) {
		result.appendChar(character);
	};
	feedStringFromFileBuffer(reciever, fileContent.getUnsafeData(), fileContent.size);
	return result;
}

// Loads a text file of unknown format
//   Removes carriage-return characters to make processing easy with only line-feed for breaking lines
String dsr::string_load(const ReadableString& filename, bool mustExist) {
	// TODO: Load files using Unicode filenames when available
	TO_RAW_ASCII(asciiFilename, filename);
	std::ifstream fileStream(asciiFilename, std::ios_base::in | std::ios_base::binary);
	if (fileStream.is_open()) {
		String result;
		// Get the file's length and allocate an array for the raw encoding
		fileStream.seekg (0, fileStream.end);
		int64_t fileLength = fileStream.tellg();
		fileStream.seekg (0, fileStream.beg);
		uint8_t* buffer = (uint8_t*)malloc(fileLength);
		fileStream.read((char*)buffer, fileLength);
		// Measure the size of the result by scanning the content in advance
		int64_t characterCount = 0;
		UTF32WriterFunction measurer = [&characterCount](DsrChar character) {
			characterCount++;
		};
		feedStringFromFileBuffer(measurer, buffer, fileLength);
		// Pre-allocate the correct amount of memory based on the simulation
		result.reserve(characterCount);
		// Stream output to the result string
		UTF32WriterFunction reciever = [&result](DsrChar character) {
			result.appendChar(character);
		};
		feedStringFromFileBuffer(reciever, buffer, fileLength);
		free(buffer);
		return result;
	} else {
		if (mustExist) {
			throwError(U"The text file ", filename, U" could not be opened for reading.\n");
		}
		// If the file cound not be found and opened, a null string is returned
		return String();
	}
}

#define AT_MOST_BITS(BIT_COUNT) if (character >= 1 << BIT_COUNT) { character = U'?'; }

template <CharacterEncoding characterEncoding>
static void encodeCharacterToStream(std::ostream &target, DsrChar character) {
	if (characterEncoding == CharacterEncoding::Raw_Latin1) {
		// Replace any illegal characters with questionmarks
		AT_MOST_BITS(8);
		byteToStream(target, character);
	} else if (characterEncoding == CharacterEncoding::BOM_UTF8) {
		// Replace any illegal characters with questionmarks
		AT_MOST_BITS(21);
		if (character < (1 << 7)) {
			// 0xxxxxxx
			byteToStream(target, character);
		} else if (character < (1 << 11)) {
			// 110xxxxx 10xxxxxx
			byteToStream(target, 0b11000000 | ((character & (0b11111 << 6)) >> 6));
			byteToStream(target, 0b10000000 | (character & 0b111111));
		} else if (character < (1 << 16)) {
			// 1110xxxx 10xxxxxx 10xxxxxx
			byteToStream(target, 0b11100000 | ((character & (0b1111 << 12)) >> 12));
			byteToStream(target, 0b10000000 | ((character & (0b111111 << 6)) >> 6));
			byteToStream(target, 0b10000000 | (character & 0b111111));
		} else if (character < (1 << 21)) {
			// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			byteToStream(target, 0b11110000 | ((character & (0b111 << 18)) >> 18));
			byteToStream(target, 0b10000000 | ((character & (0b111111 << 12)) >> 12));
			byteToStream(target, 0b10000000 | ((character & (0b111111 << 6)) >> 6));
			byteToStream(target, 0b10000000 | (character & 0b111111));
		}
	} else { // Assuming UTF-16
		AT_MOST_BITS(20);
		if (character <= 0xD7FF || (character >= 0xE000 && character <= 0xFFFF)) {
			// xxxxxxxx xxxxxxxx (Limited range)
			uint32_t higher8Bits = (character & 0b1111111100000000) >> 8;
			uint32_t lower8Bits  =  character & 0b0000000011111111;
			if (characterEncoding == CharacterEncoding::BOM_UTF16BE) {
				byteToStream(target, higher8Bits);
				byteToStream(target, lower8Bits);
			} else { // Assuming UTF-16 LE
				byteToStream(target, lower8Bits);
				byteToStream(target, higher8Bits);
			}
		} else if (character >= 0x010000 && character <= 0x10FFFF) {
			// 110110xxxxxxxxxx 110111xxxxxxxxxx
			uint32_t code = character - 0x10000;
			uint32_t higher10Bits = (code & 0b11111111110000000000) >> 10;
			uint32_t lower10Bits  =  code & 0b00000000001111111111;
			uint32_t byteA = (0b110110 << 2) | ((higher10Bits & (0b11 << 8)) >> 8);
			uint32_t byteB = higher10Bits & 0b11111111;
			uint32_t byteC = (0b110111 << 2) | ((lower10Bits & (0b11 << 8)) >> 8);
			uint32_t byteD = lower10Bits & 0b11111111;
			if (characterEncoding == CharacterEncoding::BOM_UTF16BE) {
				byteToStream(target, byteA);
				byteToStream(target, byteB);
				byteToStream(target, byteC);
				byteToStream(target, byteD);
			} else { // Assuming UTF-16 LE
				byteToStream(target, byteB);
				byteToStream(target, byteA);
				byteToStream(target, byteD);
				byteToStream(target, byteC);
			}
		}
	}
}

// Template for writing a whole string to a file
template <CharacterEncoding characterEncoding, LineEncoding lineEncoding>
static void writeCharacterToStream(std::ostream &target, String content) {
	// Write byte order marks
	if (characterEncoding == CharacterEncoding::BOM_UTF8) {
		byteToStream(target, 0xEF);
		byteToStream(target, 0xBB);
		byteToStream(target, 0xBF);
	} else if (characterEncoding == CharacterEncoding::BOM_UTF16BE) {
		byteToStream(target, 0xFE);
		byteToStream(target, 0xFF);
	} else if (characterEncoding == CharacterEncoding::BOM_UTF16LE) {
		byteToStream(target, 0xFF);
		byteToStream(target, 0xFE);
	}
	// Write encoded content
	for (int i = 0; i < string_length(content); i++) {
		DsrChar character = content[i];
		if (character == U'\n') {
			if (lineEncoding == LineEncoding::CrLf) {
				encodeCharacterToStream<characterEncoding>(target, U'\r');
				encodeCharacterToStream<characterEncoding>(target, U'\n');
			} else { // Assuming that lineEncoding == LineEncoding::Lf
				encodeCharacterToStream<characterEncoding>(target, U'\n');
			}
		} else {
			encodeCharacterToStream<characterEncoding>(target, character);
		}
	}
}

// Macros for dynamcally selecting templates
#define WRITE_TEXT_STRING(CHAR_ENCODING, LINE_ENCODING) \
	writeCharacterToStream<CHAR_ENCODING, LINE_ENCODING>(fileStream, content);
#define WRITE_TEXT_LINE_ENCODINGS(CHAR_ENCODING) \
	if (lineEncoding == LineEncoding::CrLf) { \
		WRITE_TEXT_STRING(CHAR_ENCODING, LineEncoding::CrLf); \
	} else if (lineEncoding == LineEncoding::Lf) { \
		WRITE_TEXT_STRING(CHAR_ENCODING, LineEncoding::Lf); \
	}
void dsr::string_save(const ReadableString& filename, const ReadableString& content, CharacterEncoding characterEncoding, LineEncoding lineEncoding) {
	// TODO: Load files using Unicode filenames
	TO_RAW_ASCII(asciiFilename, filename);
	std::ofstream fileStream(asciiFilename, std::ios_base::out | std::ios_base::binary);
	if (fileStream.is_open()) {
		if (characterEncoding == CharacterEncoding::Raw_Latin1) {
			WRITE_TEXT_LINE_ENCODINGS(CharacterEncoding::Raw_Latin1);
		} else if (characterEncoding == CharacterEncoding::BOM_UTF8) {
			WRITE_TEXT_LINE_ENCODINGS(CharacterEncoding::BOM_UTF8);
		} else if (characterEncoding == CharacterEncoding::BOM_UTF16BE) {
			WRITE_TEXT_LINE_ENCODINGS(CharacterEncoding::BOM_UTF16BE);
		} else if (characterEncoding == CharacterEncoding::BOM_UTF16LE) {
			WRITE_TEXT_LINE_ENCODINGS(CharacterEncoding::BOM_UTF16LE);
		}
		fileStream.close();
	} else {
		throwError("Failed to save ", filename, "\n");
	}
}

const char32_t* dsr::file_separator() {
	#ifdef _WIN32
		return U"\\";
	#else
		return U"/";
	#endif
}

int ReadableString::length() const {
	return this->sectionLength;
}

bool ReadableString::checkBound(int start, int length, bool warning) const {
	if (start < 0 || start + length > this->length()) {
		if (warning) {
			String message;
			string_append(message, U"\n");
			string_append(message, U" _____________________ Sub-string bound exception! _____________________\n");
			string_append(message, U"/\n");
			string_append(message, U"|  Characters from ", start, U" to ", (start + length - 1), U" are out of bound!\n");
			string_append(message, U"|  In source string of 0..", (this->length() - 1), U".\n");
			string_append(message, U"\\_______________________________________________________________________\n");
			throwError(message);
		}
		return false;
	} else {
		return true;
	}
}

DsrChar ReadableString::read(int index) const {
	if (index < 0 || index >= this->sectionLength) {
		return '\0';
	} else {
		return this->readSection[index];
	}
}

DsrChar ReadableString::operator[] (int index) const { return this->read(index); }

ReadableString::ReadableString() {}
ReadableString::~ReadableString() {}

ReadableString::ReadableString(const DsrChar *content, int sectionLength)
: readSection(content), sectionLength(sectionLength) {}

ReadableString::ReadableString(const DsrChar *content)
: readSection(content), sectionLength(strlen_utf32(content)) {}

String::String() {}
String::String(const char* source) { this->append(source); }
String::String(const char32_t* source) { this->append(source); }
String::String(const std::string& source) { this->append(source); }
String::String(const ReadableString& source) { this->append(source); }
String::String(const String& source) { this->append(source); }

String::String(std::shared_ptr<Buffer> buffer, DsrChar *content, int sectionLength)
 : ReadableString(content, sectionLength), buffer(buffer), writeSection(content) {}

int String::capacity() {
	if (this->buffer.get() == nullptr) {
		return 0;
	} else {
		// Get the parent allocation
		uint8_t* parentBuffer = this->buffer->getUnsafeData();
		// Get the offset from the parent
		intptr_t offset = (uint8_t*)this->writeSection - parentBuffer;
		// Subtract offset from the buffer size to get the remaining space
		return (this->buffer->size - offset) / sizeof(DsrChar);
	}
}

ReadableString ReadableString::getRange(int start, int length) const {
	if (length < 1) {
		return ReadableString();
	} else if (this->checkBound(start, length)) {
		return ReadableString(&(this->readSection[start]), length);
	} else {
		return ReadableString();
	}
}

ReadableString String::getRange(int start, int length) const {
	if (length < 1) {
		return ReadableString();
	} else if (this->checkBound(start, length)) {
		return String(this->buffer, &(this->writeSection[start]), length);
	} else {
		return ReadableString();
	}
}

static int32_t getNewBufferSize(int32_t minimumSize) {
	if (minimumSize <= 128) {
		return 128;
	} else if (minimumSize <= 512) {
		return 512;
	} else if (minimumSize <= 2048) {
		return 2048;
	} else if (minimumSize <= 8192) {
		return 8192;
	} else if (minimumSize <= 32768) {
		return 32768;
	} else if (minimumSize <= 131072) {
		return 131072;
	} else if (minimumSize <= 524288) {
		return 524288;
	} else if (minimumSize <= 2097152) {
		return 2097152;
	} else if (minimumSize <= 8388608) {
		return 8388608;
	} else if (minimumSize <= 33554432) {
		return 33554432;
	} else if (minimumSize <= 134217728) {
		return 134217728;
	} else if (minimumSize <= 536870912) {
		return 536870912;
	} else {
		return 2147483647;
	}
}
void String::reallocateBuffer(int32_t newLength, bool preserve) {
	// Holding oldData alive while copying to the new buffer
	std::shared_ptr<Buffer> oldBuffer = this->buffer;
	const char32_t* oldData = this->readSection;
	this->buffer = std::make_shared<Buffer>(getNewBufferSize(newLength * sizeof(DsrChar)));
	this->readSection = this->writeSection = reinterpret_cast<char32_t*>(this->buffer->getUnsafeData());
	if (preserve && oldData) {
		memcpy(this->writeSection, oldData, this->sectionLength * sizeof(DsrChar));
	}
}

// Call before writing to the buffer
//   This hides that Strings share buffers when assigning by value or taking partial strings
void String::cloneIfShared() {
	if (this->buffer.use_count() > 1) {
		this->reallocateBuffer(this->sectionLength, true);
	}
}

void String::expand(int32_t newLength, bool affectUsedLength) {
	if (newLength > this->sectionLength) {
		if (newLength > this->capacity()) {
			this->reallocateBuffer(newLength, true);
		}
	}
	if (affectUsedLength) {
		this->sectionLength = newLength;
	}
}

void String::reserve(int32_t minimumLength) {
	this->expand(minimumLength, false);
}

void String::write(int index, DsrChar value) {
	this->cloneIfShared();
	if (index < 0 || index >= this->sectionLength) {
		// TODO: Give a warning
	} else {
		this->writeSection[index] = value;
	}
}

void String::clear() {
	this->sectionLength = 0;
}

// This macro has to be used because a static template wouldn't be able to inherit access to private methods from the target class.
//   Better to use a macro without type safety in the implementation than to expose yet another template in a global header.
// Proof that appending to one string doesn't affect another:
//   If it has to reallocate
//     * Then it will have its own buffer without conflicts
//   If it doesn't have to reallocate
//     If it shares the buffer
//       If source is empty
//         * Then no risk of overwriting neighbor strings if we don't write
//       If source isn't empty
//         * Then the buffer will be cloned when the first character is written
//     If it doesn't share the buffer
//       * Then no risk of writing
#define APPEND(TARGET, SOURCE, LENGTH, MASK) { \
	int64_t oldLength = (TARGET)->length(); \
	(TARGET)->expand(oldLength + (int64_t)(LENGTH), true); \
	for (int64_t i = 0; i < (int64_t)(LENGTH); i++) { \
		(TARGET)->write(oldLength + i, ((SOURCE)[i]) & MASK); \
	} \
}
// TODO: See if ascii litterals can be checked for values above 127 in compile-time
void String::append(const char* source) { APPEND(this, source, strlen(source), 0xFF); }
// TODO: Use memcpy when appending input of the same format
void String::append(const ReadableString& source) { APPEND(this, source, source.length(), 0xFFFFFFFF); }
void String::append(const char32_t* source) { APPEND(this, source, strlen_utf32(source), 0xFFFFFFFF); }
void String::append(const std::string& source) { APPEND(this, source.c_str(), (int)source.size(), 0xFF); }
void String::appendChar(DsrChar source) { APPEND(this, &source, 1, 0xFFFFFFFF); }

String& dsr::string_toStreamIndented(String& target, const Printable& source, const ReadableString& indentation) {
	return source.toStreamIndented(target, indentation);
}
String& dsr::string_toStreamIndented(String& target, const char* value, const ReadableString& indentation) {
	target.append(indentation);
	target.append(value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const ReadableString& value, const ReadableString& indentation) {
	target.append(indentation);
	target.append(value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const char32_t* value, const ReadableString& indentation) {
	target.append(indentation);
	target.append(value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const std::string& value, const ReadableString& indentation) {
	target.append(indentation);
	target.append(value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const float& value, const ReadableString& indentation) {
	target.append(indentation);
	doubleToString_arabic(target, (double)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const double& value, const ReadableString& indentation) {
	target.append(indentation);
	doubleToString_arabic(target, value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const int64_t& value, const ReadableString& indentation) {
	target.append(indentation);
	intToString_arabic(target, value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const uint64_t& value, const ReadableString& indentation) {
	target.append(indentation);
	uintToString_arabic(target, value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const int32_t& value, const ReadableString& indentation) {
	target.append(indentation);
	intToString_arabic(target, (int64_t)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const uint32_t& value, const ReadableString& indentation) {
	target.append(indentation);
	uintToString_arabic(target, (uint64_t)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const int16_t& value, const ReadableString& indentation) {
	target.append(indentation);
	intToString_arabic(target, (int64_t)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const uint16_t& value, const ReadableString& indentation) {
	target.append(indentation);
	uintToString_arabic(target, (uint64_t)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const int8_t& value, const ReadableString& indentation) {
	target.append(indentation);
	intToString_arabic(target, (int64_t)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const uint8_t& value, const ReadableString& indentation) {
	target.append(indentation);
	uintToString_arabic(target, (uint64_t)value);
	return target;
}

void dsr::throwErrorMessage(const String& message) {
	throw std::runtime_error(message.toStdString());
}

void dsr::string_split_inPlace(List<ReadableString> &target, const ReadableString& source, DsrChar separator, bool appendResult) {
	if (!appendResult) {
		target.clear();
	}
	int sectionStart = 0;
	for (int i = 0; i < source.length(); i++) {
		DsrChar c = source[i];
		if (c == separator) {
			target.push(string_exclusiveRange(source, sectionStart, i));
			sectionStart = i + 1;
		}
	}
	if (source.length() > sectionStart) {
		target.push(string_exclusiveRange(source, sectionStart, source.length()));;
	}
}

List<ReadableString> dsr::string_split(const ReadableString& source, DsrChar separator) {
	List<ReadableString> result;
	string_split_inPlace(result, source, separator);
	return result;
}

int64_t dsr::string_toInteger(const ReadableString& source) {
	int64_t result;
	bool negated;
	result = 0;
	negated = false;
	for (int i = 0; i < source.length(); i++) {
		DsrChar c = source[i];
		if (c == '-' || c == '~') {
			negated = !negated;
		} else if (c >= '0' && c <= '9') {
			result = (result * 10) + (int)(c - '0');
		} else if (c == ',' || c == '.') {
			// Truncate any decimals by ignoring them
			break;
		}
	}
	if (negated) {
		return -result;
	} else {
		return result;
	}
}

double dsr::string_toDouble(const ReadableString& source) {
	double result;
	bool negated;
	bool reachedDecimal;
	int digitDivider;
	result = 0.0;
	negated = false;
	reachedDecimal = false;
	digitDivider = 1;
	for (int i = 0; i < source.length(); i++) {
		DsrChar c = source[i];
		if (c == '-' || c == '~') {
			negated = !negated;
		} else if (c >= '0' && c <= '9') {
			if (reachedDecimal) {
				digitDivider = digitDivider * 10;
				result = result + ((double)(c - '0') / (double)digitDivider);
			} else {
				result = (result * 10) + (double)(c - '0');
			}
		} else if (c == ',' || c == '.') {
			reachedDecimal = true;
		}
	}
	if (negated) {
		return -result;
	} else {
		return result;
	}
}

int dsr::string_length(const ReadableString& source) {
	return source.length();
}

int dsr::string_findFirst(const ReadableString& source, DsrChar toFind, int startIndex) {
	for (int i = startIndex; i < source.length(); i++) {
		if (source[i] == toFind) {
			return i;
		}
	}
	return -1;
}

int dsr::string_findLast(const ReadableString& source, DsrChar toFind) {
	for (int i = source.length() - 1; i >= 0; i--) {
		if (source[i] == toFind) {
			return i;
		}
	}
	return -1;
}

ReadableString dsr::string_exclusiveRange(const ReadableString& source, int inclusiveStart, int exclusiveEnd) {
	return source.getRange(inclusiveStart, exclusiveEnd - inclusiveStart);
}

ReadableString dsr::string_inclusiveRange(const ReadableString& source, int inclusiveStart, int inclusiveEnd) {
	return source.getRange(inclusiveStart, inclusiveEnd + 1 - inclusiveStart);
}

ReadableString dsr::string_before(const ReadableString& source, int exclusiveEnd) {
	return string_exclusiveRange(source, 0, exclusiveEnd);
}

ReadableString dsr::string_until(const ReadableString& source, int inclusiveEnd) {
	return string_inclusiveRange(source, 0, inclusiveEnd);
}

ReadableString dsr::string_from(const ReadableString& source, int inclusiveStart) {
	return string_exclusiveRange(source, inclusiveStart, source.length());
}

ReadableString dsr::string_after(const ReadableString& source, int exclusiveStart) {
	return string_from(source, exclusiveStart + 1);
}

bool dsr::character_isDigit(DsrChar c) {
	return c >= U'0' && c <= U'9';
}

bool dsr::character_isIntegerCharacter(DsrChar c) {
	return c == U'-' || character_isDigit(c);
}

bool dsr::character_isValueCharacter(DsrChar c) {
	return c == U'.' || character_isIntegerCharacter(c);
}

bool dsr::character_isWhiteSpace(DsrChar c) {
	return c == U' ' || c == U'\t' || c == U'\v' || c == U'\f' || c == U'\n' || c == U'\r';
}

// Macros for implementing regular expressions with a greedy approach consuming the first match
//   Optional accepts 0 or 1 occurence
//   Forced accepts 1 occurence
//   Star accepts 0..N occurence
//   Plus accepts 1..N occurence
#define CHARACTER_OPTIONAL(CHARACTER) if (source[readIndex] == CHARACTER) { readIndex++; }
#define CHARACTER_FORCED(CHARACTER) if (source[readIndex] == CHARACTER) { readIndex++; } else { return false; }
#define CHARACTER_STAR(CHARACTER) while (source[readIndex] == CHARACTER) { readIndex++; }
#define CHARACTER_PLUS(CHARACTER) CHARACTER_FORCED(CHARACTER) CHARACTER_STAR(CHARACTER)
#define PATTERN_OPTIONAL(PATTERN) if (character_is##PATTERN(source[readIndex])) { readIndex++; }
#define PATTERN_FORCED(PATTERN) if (character_is##PATTERN(source[readIndex])) { readIndex++; } else { return false; }
#define PATTERN_STAR(PATTERN) while (character_is##PATTERN(source[readIndex])) { readIndex++; }
#define PATTERN_PLUS(PATTERN) PATTERN_FORCED(PATTERN) PATTERN_STAR(PATTERN)

// The greedy approach works here, because there's no ambiguity
bool dsr::string_isInteger(const ReadableString& source, bool allowWhiteSpace) {
	int readIndex = 0;
	if (allowWhiteSpace) {
		PATTERN_STAR(WhiteSpace);
	}
	CHARACTER_OPTIONAL(U'-');
	// At least one digit required
	PATTERN_PLUS(IntegerCharacter);
	if (allowWhiteSpace) {
		PATTERN_STAR(WhiteSpace);
	}
	return true;
}

// To avoid consuming the all digits on Digit* before reaching Digit+ when there is no decimal, whole integers are judged by string_isInteger
bool dsr::string_isDouble(const ReadableString& source, bool allowWhiteSpace) {
	// Solving the UnsignedDouble <- Digit+ | Digit* '.' Digit+ ambiguity is done easiest by checking if there's a decimal before handling the white-space and negation
	if (string_findFirst(source, U'.') == -1) {
		// No decimal detected
		return string_isInteger(source, allowWhiteSpace);
	} else {
		int readIndex = 0;
		if (allowWhiteSpace) {
			PATTERN_STAR(WhiteSpace);
		}
		// Double <- UnsignedDouble | '-' UnsignedDouble
		CHARACTER_OPTIONAL(U'-');
		// UnsignedDouble <- Digit* '.' Digit+
		// Any number of integer digits
		PATTERN_STAR(IntegerCharacter);
		// Only dot for decimal
		CHARACTER_FORCED(U'.')
		// At least one decimal digit
		PATTERN_PLUS(IntegerCharacter);
		if (allowWhiteSpace) {
			PATTERN_STAR(WhiteSpace);
		}
		return true;
	}
}

