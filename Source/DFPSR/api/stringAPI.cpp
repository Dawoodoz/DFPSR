// zlib open source license
//
// Copyright (c) 2017 to 2020 David Forsgren Piuva
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

// Gets access to private members by making them public for the whole module
#define DFPSR_INTERNAL_ACCESS

#include <fstream>
#include <streambuf>
#include <cstring>
#include <stdexcept>
#include "stringAPI.h"
#include "../api/fileAPI.h"

using namespace dsr;

static void atomic_append(String &target, const char* source);
static void atomic_append(String &target, const ReadableString& source);
static void atomic_append(String &target, const char32_t* source);
static void atomic_append(String &target, const std::string& source);

static int64_t strlen_utf32(const char32_t *content) {
	int64_t length = 0;
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

ReadableString::ReadableString() {}

ReadableString::~ReadableString() {}

ReadableString::ReadableString(const DsrChar *content)
: readSection(content), length(strlen_utf32(content)) {}

ReadableString::ReadableString(const String& source) {
	this->readSection = source.readSection;
	this->length = source.length;
	this->buffer = source.buffer;
}

// Not the fastest constructor, but won't bloat the public header
// Hopefully most compilers know how to optimize this
static ReadableString createSubString(const DsrChar *content, int64_t length, const Buffer &buffer) {
	ReadableString result;
	result.readSection = content;
	result.length = length;
	result.buffer = buffer;
	return result;
}

static String createSubString_shared(const DsrChar *content, int64_t length, const Buffer &buffer, char32_t* writeSection) {
	String result;
	result.readSection = content;
	result.length = length;
	result.buffer = buffer;
	result.writeSection = writeSection;
	return result;
}

String::String() {}
String::String(const char* source) { atomic_append(*this, source); }
String::String(const char32_t* source) { atomic_append(*this, source); }
String::String(const std::string& source) { atomic_append(*this, source); }
String::String(const String& source) {
	// Share immutable buffer
	this->readSection = source.readSection;
	this->length = source.length;
	this->buffer = source.buffer;
	this->writeSection = source.writeSection;
}
String::String(const ReadableString& source) {
	if (buffer_exists(source.buffer)) {
		this->readSection = source.readSection;
		this->length = source.length;
		this->buffer = source.buffer;
		this->writeSection = const_cast<char32_t*>(source.readSection); // Still safe because of immutability
	} else {
		// No buffer to share, just appending the content
		atomic_append(*this, source);
	}
}

DsrChar ReadableString::operator[] (int64_t index) const {
	if (index < 0 || index >= this->length) {
		return U'\0';
	} else {
		return this->readSection[index];
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
	for (int64_t i = 0; i < result.length; i++) {
		out.put(toAscii(result.readSection[i]));
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
	if (a.length != b.length) {
		return false;
	} else {
		for (int64_t i = 0; i < a.length; i++) {
			if (a.readSection[i] != b.readSection[i]) {
				return false;
			}
		}
		return true;
	}
}

bool dsr::string_caseInsensitiveMatch(const ReadableString& a, const ReadableString& b) {
	if (a.length != b.length) {
		return false;
	} else {
		for (int64_t i = 0; i < a.length; i++) {
			if (towupper(a.readSection[i]) != towupper(b.readSection[i])) {
				return false;
			}
		}
		return true;
	}
}

std::ostream& ReadableString::toStream(std::ostream& out) const {
	for (int64_t i = 0; i < this->length; i++) {
		out.put(toAscii(this->readSection[i]));
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
	string_reserve(result, text.length);
	for (int64_t i = 0; i < text.length; i++) {
		string_appendChar(result, towupper(text[i]));
	}
	return result;
}

String dsr::string_lowerCase(const ReadableString &text) {
	String result;
	string_reserve(result, text.length);
	for (int64_t i = 0; i < text.length; i++) {
		string_appendChar(result, towlower(text[i]));
	}
	return result;
}

static int64_t findFirstNonWhite(const ReadableString &text) {
	for (int64_t i = 0; i < text.length; i++) {
		DsrChar c = text[i];
		if (!character_isWhiteSpace(c)) {
			return i;
		}
	}
	return -1;
}

static int64_t findLastNonWhite(const ReadableString &text) {
	for (int64_t i = text.length - 1; i >= 0; i--) {
		DsrChar c = text[i];
		if (!character_isWhiteSpace(c)) {
			return i;
		}
	}
	return -1;
}

// Allow passing literals without allocating heap memory for the result
ReadableString dsr::string_removeOuterWhiteSpace(const ReadableString &text) {
	int64_t first = findFirstNonWhite(text);
	int64_t last = findLastNonWhite(text);
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
	string_reserve(result, rawText.length + 2);
	string_appendChar(result, U'\"'); // Begin quote
	for (int64_t i = 0; i < rawText.length; i++) {
		DsrChar c = rawText[i];
		if (c == U'\"') { // Double quote
			string_append(result, U"\\\"");
		} else if (c == U'\\') { // Backslash
			string_append(result, U"\\\\");
		} else if (c == U'\a') { // Audible bell
			string_append(result, U"\\a");
		} else if (c == U'\b') { // Backspace
			string_append(result, U"\\b");
		} else if (c == U'\f') { // Form feed
			string_append(result, U"\\f");
		} else if (c == U'\n') { // Line feed
			string_append(result, U"\\n");
		} else if (c == U'\r') { // Carriage return
			string_append(result, U"\\r");
		} else if (c == U'\t') { // Horizontal tab
			string_append(result, U"\\t");
		} else if (c == U'\v') { // Vertical tab
			string_append(result, U"\\v");
		} else if (c == U'\0') { // Null terminator
			string_append(result, U"\\0");
		} else {
			string_appendChar(result, c);
		}
	}
	string_appendChar(result, U'\"'); // End quote
	return result;
}

String dsr::string_unmangleQuote(const ReadableString& mangledText) {
	int64_t firstQuote = string_findFirst(mangledText, '\"');
	int64_t lastQuote = string_findLast(mangledText, '\"');
	String result;
	if (firstQuote == -1 || lastQuote == -1 || firstQuote == lastQuote) {
		throwError(U"Cannot unmangle using string_unmangleQuote without beginning and ending with quote signs!\n", mangledText, "\n");
	} else {
		for (int64_t i = firstQuote + 1; i < lastQuote; i++) {
			DsrChar c = mangledText[i];
			if (c == U'\\') { // Escape character
				DsrChar c2 = mangledText[i + 1];
				if (c2 == U'\"') { // Double quote
					string_appendChar(result, U'\"');
				} else if (c2 == U'\\') { // Back slash
					string_appendChar(result, U'\\');
				} else if (c2 == U'a') { // Audible bell
					string_appendChar(result, U'\a');
				} else if (c2 == U'b') { // Backspace
					string_appendChar(result, U'\b');
				} else if (c2 == U'f') { // Form feed
					string_appendChar(result, U'\f');
				} else if (c2 == U'n') { // Line feed
					string_appendChar(result, U'\n');
				} else if (c2 == U'r') { // Carriage return
					string_appendChar(result, U'\r');
				} else if (c2 == U't') { // Horizontal tab
					string_appendChar(result, U'\t');
				} else if (c2 == U'v') { // Vertical tab
					string_appendChar(result, U'\v');
				} else if (c2 == U'0') { // Null terminator
					string_appendChar(result, U'\0');
				}
				i++; // Consume both characters
			} else {
				// Detect bad input
				if (c == U'\"') { // Double quote
					 throwError(U"Unmangled double quote sign detected in string_unmangleQuote!\n", mangledText, "\n");
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
					string_appendChar(result, c);
				}
			}
		}
	}
	return result;
}

static void uintToString_arabic(String& target, uint64_t value) {
	static const int bufferSize = 20;
	DsrChar digits[bufferSize];
	int64_t usedSize = 0;
	if (value == 0) {
		string_appendChar(target, U'0');
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
			string_appendChar(target, digits[usedSize]);
		}
	}
}

static void intToString_arabic(String& target, int64_t value) {
	if (value >= 0) {
		uintToString_arabic(target, (uint64_t)value);
	} else {
		string_appendChar(target, U'-');
		uintToString_arabic(target, (uint64_t)(-value));
	}
}

// TODO: Implement own version to ensure that nothing strange is happening from buggy std implementations
static void doubleToString_arabic(String& target, double value) {
	std::ostringstream buffer;
	buffer << std::fixed << value; // Generate using a fixed number of decimals
	std::string result = buffer.str();
	// Remove trailing zero decimal digits
	int64_t decimalCount = 0;
	int64_t lastValueIndex = -1;
	for (size_t c = 0; c < result.length(); c++) {
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
	for (int64_t c = 0; c <= lastValueIndex; c++) {
		string_appendChar(target, result[c]);
	}
}

#define TO_RAW_ASCII(TARGET, SOURCE) \
	char TARGET[SOURCE.length + 1]; \
	for (int64_t i = 0; i < SOURCE.length; i++) { \
		TARGET[i] = toAscii(SOURCE[i]); \
	} \
	TARGET[SOURCE.length] = '\0';

// A function definition for receiving a stream of bytes
//   Instead of using std's messy inheritance
using ByteWriterFunction = std::function<void(uint8_t value)>;

// A function definition for receiving a stream of UTF-32 characters
//   Instead of using std's messy inheritance
using UTF32WriterFunction = std::function<void(DsrChar character)>;

// Filter out unwanted characters for improved portability
static void feedCharacter(const UTF32WriterFunction &reciever, DsrChar character) {
	if (character != U'\0' && character != U'\r') {
		reciever(character);
	}
}

// Appends the content of buffer as a BOM-free Latin-1 file into target
// fileLength is ignored when nullTerminated is true
template <bool nullTerminated>
static void feedStringFromFileBuffer_Latin1(const UTF32WriterFunction &reciever, const uint8_t* buffer, int64_t fileLength = 0) {
	for (int64_t i = 0; i < fileLength || nullTerminated; i++) {
		DsrChar character = (DsrChar)(buffer[i]);
		if (nullTerminated && character == 0) { return; }
		feedCharacter(reciever, character);
	}
}
// Appends the content of buffer as a BOM-free UTF-8 file into target
// fileLength is ignored when nullTerminated is true
template <bool nullTerminated>
static void feedStringFromFileBuffer_UTF8(const UTF32WriterFunction &reciever, const uint8_t* buffer, int64_t fileLength = 0) {
	for (int64_t i = 0; i < fileLength || nullTerminated; i++) {
		uint8_t byteA = buffer[i];
		if (byteA < (uint32_t)0b10000000) {
			// Single byte (1xxxxxxx)
			if (nullTerminated && byteA == 0) { return; }
			feedCharacter(reciever, (DsrChar)byteA);
		} else {
			uint32_t character = 0;
			int extraBytes = 0;
			if (byteA >= (uint32_t)0b11000000) { // At least two leading ones
				if (byteA < (uint32_t)0b11100000) { // Less than three leading ones
					character = byteA & (uint32_t)0b00011111;
					extraBytes = 1;
				} else if (byteA < (uint32_t)0b11110000) { // Less than four leading ones
					character = byteA & (uint32_t)0b00001111;
					extraBytes = 2;
				} else if (byteA < (uint32_t)0b11111000) { // Less than five leading ones
					character = byteA & (uint32_t)0b00000111;
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
uint16_t read16bits(const uint8_t* buffer, int64_t startOffset) {
	uint16_t byteA = buffer[startOffset];
	uint16_t byteB = buffer[startOffset + 1];
	if (LittleEndian) {
		return (byteB << 8) | byteA;
	} else {
		return (byteA << 8) | byteB;
	}
}

// Appends the content of buffer as a BOM-free UTF-16 file into target as UTF-32
// fileLength is ignored when nullTerminated is true
template <bool LittleEndian, bool nullTerminated>
static void feedStringFromFileBuffer_UTF16(const UTF32WriterFunction &reciever, const uint8_t* buffer, int64_t fileLength = 0) {
	for (int64_t i = 0; i < fileLength || nullTerminated; i += 2) {
		// Read the first 16-bit word
		uint16_t wordA = read16bits<LittleEndian>(buffer, i);
		// Check if another word is needed
		//   Assuming that wordA >= 0x0000 and wordA <= 0xFFFF as uint16_t,
		//   we can just check if it's within the range reserved for 32-bit encoding
		if (wordA <= 0xD7FF || wordA >= 0xE000) {
			// Not in the reserved range, just a single 16-bit character
			if (nullTerminated && wordA == 0) { return; }
			feedCharacter(reciever, (DsrChar)wordA);
		} else {
			// The given range was reserved and therefore using 32 bits
			i += 2;
			uint16_t wordB = read16bits<LittleEndian>(buffer, i);
			uint32_t higher10Bits = wordA & (uint32_t)0b1111111111;
			uint32_t lower10Bits  = wordB & (uint32_t)0b1111111111;
			DsrChar finalChar = (DsrChar)(((higher10Bits << 10) | lower10Bits) + (uint32_t)0x10000);
			feedCharacter(reciever, finalChar);
		}
	}
}
// Sends the decoded UTF-32 characters from the encoded buffer into target.
// The text encoding should be specified using a BOM at the start of buffer, otherwise Latin-1 is assumed.
static void feedStringFromFileBuffer(const UTF32WriterFunction &reciever, const uint8_t* buffer, int64_t fileLength) {
	// After removing the BOM bytes, the rest can be seen as a BOM-free text file with a known format
	if (fileLength >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) { // UTF-8
		feedStringFromFileBuffer_UTF8<false>(reciever, buffer + 3, fileLength - 3);
	} else if (fileLength >= 2 && buffer[0] == 0xFE && buffer[1] == 0xFF) { // UTF-16 BE
		feedStringFromFileBuffer_UTF16<false, false>(reciever, buffer + 2, fileLength - 2);
	} else if (fileLength >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE) { // UTF-16 LE
		feedStringFromFileBuffer_UTF16<true, false>(reciever, buffer + 2, fileLength - 2);
	} else if (fileLength >= 4 && buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0xFE && buffer[3] == 0xFF) { // UTF-32 BE
		//feedStringFromFileBuffer_UTF32BE(receiver, buffer + 4, fileLength - 4);
		throwError(U"UTF-32 BE format is not yet supported!\n");
	} else if (fileLength >= 4 && buffer[0] == 0xFF && buffer[1] == 0xFE && buffer[2] == 0x00 && buffer[3] == 0x00) { // UTF-32 LE
		//feedStringFromFileBuffer_UTF32BE(receiver, buffer + 4, fileLength - 4);
		throwError(U"UTF-32 LE format is not yet supported!\n");
	} else if (fileLength >= 3 && buffer[0] == 0xF7 && buffer[1] == 0x64 && buffer[2] == 0x4C) { // UTF-1
		//feedStringFromFileBuffer_UTF1(receiver, buffer + 3, fileLength - 3);
		throwError(U"UTF-1 format is not yet supported!\n");
	} else if (fileLength >= 3 && buffer[0] == 0x0E && buffer[1] == 0xFE && buffer[2] == 0xFF) { // SCSU
		//feedStringFromFileBuffer_SCSU(receiver, buffer + 3, fileLength - 3);
		throwError(U"SCSU format is not yet supported!\n");
	} else if (fileLength >= 3 && buffer[0] == 0xFB && buffer[1] == 0xEE && buffer[2] == 0x28) { // BOCU
		//feedStringFromFileBuffer_BOCU-1(receiver, buffer + 3, fileLength - 3);
		throwError(U"BOCU-1 format is not yet supported!\n");
	} else if (fileLength >= 4 && buffer[0] == 0x2B && buffer[1] == 0x2F && buffer[2] == 0x76) { // UTF-7
		// Ignoring fourth byte with the dialect of UTF-7 when just showing the error message
		throwError(U"UTF-7 format is not yet supported!\n");
	} else {
		// No BOM detected, assuming Latin-1 (because it directly corresponds to a unicode sub-set)
		feedStringFromFileBuffer_Latin1<false>(reciever, buffer, fileLength);
	}
}

// Sends the decoded UTF-32 characters from the encoded null terminated buffer into target.
// buffer may not contain any BOM, and must be null terminated in the specified encoding.
static void feedStringFromRawData(const UTF32WriterFunction &reciever, const uint8_t* buffer, CharacterEncoding encoding) {
	if (encoding == CharacterEncoding::Raw_Latin1) {
		feedStringFromFileBuffer_Latin1<true>(reciever, buffer);
	} else if (encoding == CharacterEncoding::BOM_UTF8) {
		feedStringFromFileBuffer_UTF8<true>(reciever, buffer);
	} else if (encoding == CharacterEncoding::BOM_UTF16BE) {
		feedStringFromFileBuffer_UTF16<false, true>(reciever, buffer);
	} else if (encoding == CharacterEncoding::BOM_UTF16LE) {
		feedStringFromFileBuffer_UTF16<true, true>(reciever, buffer);
	} else {
		throwError("Unhandled encoding in feedStringFromRawData!\n");
	}
}

String dsr::string_dangerous_decodeFromData(const void* data, CharacterEncoding encoding) {
	String result;
	// Measure the size of the result by scanning the content in advance
	int64_t characterCount = 0;
	UTF32WriterFunction measurer = [&characterCount](DsrChar character) {
		characterCount++;
	};
	feedStringFromRawData(measurer, (const uint8_t*)data, encoding);
	// Pre-allocate the correct amount of memory based on the simulation
	string_reserve(result, characterCount);
	// Stream output to the result string
	UTF32WriterFunction reciever = [&result](DsrChar character) {
		string_appendChar(result, character);
	};
	feedStringFromRawData(reciever, (const uint8_t*)data, encoding);
	return result;
}

String dsr::string_loadFromMemory(Buffer fileContent) {
	String result;
	// Measure the size of the result by scanning the content in advance
	int64_t characterCount = 0;
	UTF32WriterFunction measurer = [&characterCount](DsrChar character) {
		characterCount++;
	};
	feedStringFromFileBuffer(measurer, buffer_dangerous_getUnsafeData(fileContent), buffer_getSize(fileContent));
	// Pre-allocate the correct amount of memory based on the simulation
	string_reserve(result, characterCount);
	// Stream output to the result string
	UTF32WriterFunction reciever = [&result](DsrChar character) {
		string_appendChar(result, character);
	};
	feedStringFromFileBuffer(reciever, buffer_dangerous_getUnsafeData(fileContent), buffer_getSize(fileContent));
	return result;
}

// Loads a text file of unknown format
//   Removes carriage-return characters to make processing easy with only line-feed for breaking lines
String dsr::string_load(const ReadableString& filename, bool mustExist) {
	Buffer encoded = file_loadBuffer(filename, mustExist);
	if (!buffer_exists(encoded)) {
		return String();
	} else {
		return string_loadFromMemory(encoded);
	}
}

template <CharacterEncoding characterEncoding>
static void encodeCharacter(const ByteWriterFunction &receiver, DsrChar character) {
	if (characterEncoding == CharacterEncoding::Raw_Latin1) {
		// Replace any illegal characters with questionmarks
		if (character > 255) { character = U'?'; }
		receiver(character);
	} else if (characterEncoding == CharacterEncoding::BOM_UTF8) {
		// Replace any illegal characters with questionmarks
		if (character > 0x10FFFF) { character = U'?'; }
		if (character < (1 << 7)) {
			// 0xxxxxxx
			receiver(character);
		} else if (character < (1 << 11)) {
			// 110xxxxx 10xxxxxx
			receiver((uint32_t)0b11000000 | ((character & ((uint32_t)0b11111 << 6)) >> 6));
			receiver((uint32_t)0b10000000 |  (character &  (uint32_t)0b111111));
		} else if (character < (1 << 16)) {
			// 1110xxxx 10xxxxxx 10xxxxxx
			receiver((uint32_t)0b11100000 | ((character & ((uint32_t)0b1111 << 12)) >> 12));
			receiver((uint32_t)0b10000000 | ((character & ((uint32_t)0b111111 << 6)) >> 6));
			receiver((uint32_t)0b10000000 |  (character &  (uint32_t)0b111111));
		} else if (character < (1 << 21)) {
			// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			receiver((uint32_t)0b11110000 | ((character & ((uint32_t)0b111 << 18)) >> 18));
			receiver((uint32_t)0b10000000 | ((character & ((uint32_t)0b111111 << 12)) >> 12));
			receiver((uint32_t)0b10000000 | ((character & ((uint32_t)0b111111 << 6)) >> 6));
			receiver((uint32_t)0b10000000 |  (character &  (uint32_t)0b111111));
		}
	} else { // Assuming UTF-16
		if (character > 0x10FFFF) { character = U'?'; }
		if (character <= 0xD7FF || (character >= 0xE000 && character <= 0xFFFF)) {
			// xxxxxxxx xxxxxxxx (Limited range)
			uint32_t higher8Bits = (character & (uint32_t)0b1111111100000000) >> 8;
			uint32_t lower8Bits  =  character & (uint32_t)0b0000000011111111;
			if (characterEncoding == CharacterEncoding::BOM_UTF16BE) {
				receiver(higher8Bits);
				receiver(lower8Bits);
			} else { // Assuming UTF-16 LE
				receiver(lower8Bits);
				receiver(higher8Bits);
			}
		} else if (character >= 0x010000 && character <= 0x10FFFF) {
			// 110110xxxxxxxxxx 110111xxxxxxxxxx
			uint32_t code = character - (uint32_t)0x10000;
			uint32_t byteA = ((code & (uint32_t)0b11000000000000000000) >> 18) | (uint32_t)0b11011000;
			uint32_t byteB =  (code & (uint32_t)0b00111111110000000000) >> 10;
			uint32_t byteC = ((code & (uint32_t)0b00000000001100000000) >> 8)  | (uint32_t)0b11011100;
			uint32_t byteD =   code & (uint32_t)0b00000000000011111111;
			if (characterEncoding == CharacterEncoding::BOM_UTF16BE) {
				receiver(byteA);
				receiver(byteB);
				receiver(byteC);
				receiver(byteD);
			} else { // Assuming UTF-16 LE
				receiver(byteB);
				receiver(byteA);
				receiver(byteD);
				receiver(byteC);
			}
		}
	}
}

// Template for encoding a whole string
template <CharacterEncoding characterEncoding, LineEncoding lineEncoding>
static void encodeText(const ByteWriterFunction &receiver, String content, bool writeBOM, bool writeNullTerminator) {
	if (writeBOM) {
		// Write byte order marks
		if (characterEncoding == CharacterEncoding::BOM_UTF8) {
			receiver(0xEF);
			receiver(0xBB);
			receiver(0xBF);
		} else if (characterEncoding == CharacterEncoding::BOM_UTF16BE) {
			receiver(0xFE);
			receiver(0xFF);
		} else if (characterEncoding == CharacterEncoding::BOM_UTF16LE) {
			receiver(0xFF);
			receiver(0xFE);
		}
	}
	// Write encoded content
	for (int64_t i = 0; i < string_length(content); i++) {
		DsrChar character = content[i];
		if (character == U'\n') {
			if (lineEncoding == LineEncoding::CrLf) {
				encodeCharacter<characterEncoding>(receiver, U'\r');
				encodeCharacter<characterEncoding>(receiver, U'\n');
			} else { // Assuming that lineEncoding == LineEncoding::Lf
				encodeCharacter<characterEncoding>(receiver, U'\n');
			}
		} else {
			encodeCharacter<characterEncoding>(receiver, character);
		}
	}
	if (writeNullTerminator) {
		// Terminate internal strings with \0 to prevent getting garbage data after unpadded buffers
		if (characterEncoding == CharacterEncoding::BOM_UTF16BE || characterEncoding == CharacterEncoding::BOM_UTF16LE) {
			receiver(0);
			receiver(0);
		} else {
			receiver(0);
		}
	}
}

// Macro for converting run-time arguments into template arguments for encodeText
#define ENCODE_TEXT(RECEIVER, CONTENT, CHAR_ENCODING, LINE_ENCODING, WRITE_BOM, WRITE_NULL_TERMINATOR) \
	if (CHAR_ENCODING == CharacterEncoding::Raw_Latin1) { \
		if (LINE_ENCODING == LineEncoding::CrLf) { \
			encodeText<CharacterEncoding::Raw_Latin1, LineEncoding::CrLf>(RECEIVER, CONTENT, false, WRITE_NULL_TERMINATOR); \
		} else if (LINE_ENCODING == LineEncoding::Lf) { \
			encodeText<CharacterEncoding::Raw_Latin1, LineEncoding::Lf>(RECEIVER, CONTENT, false, WRITE_NULL_TERMINATOR); \
		} \
	} else if (CHAR_ENCODING == CharacterEncoding::BOM_UTF8) { \
		if (LINE_ENCODING == LineEncoding::CrLf) { \
			encodeText<CharacterEncoding::BOM_UTF8, LineEncoding::CrLf>(RECEIVER, CONTENT, WRITE_BOM, WRITE_NULL_TERMINATOR); \
		} else if (LINE_ENCODING == LineEncoding::Lf) { \
			encodeText<CharacterEncoding::BOM_UTF8, LineEncoding::Lf>(RECEIVER, CONTENT, WRITE_BOM, WRITE_NULL_TERMINATOR); \
		} \
	} else if (CHAR_ENCODING == CharacterEncoding::BOM_UTF16BE) { \
		if (LINE_ENCODING == LineEncoding::CrLf) { \
			encodeText<CharacterEncoding::BOM_UTF16BE, LineEncoding::CrLf>(RECEIVER, CONTENT, WRITE_BOM, WRITE_NULL_TERMINATOR); \
		} else if (LINE_ENCODING == LineEncoding::Lf) { \
			encodeText<CharacterEncoding::BOM_UTF16BE, LineEncoding::Lf>(RECEIVER, CONTENT, WRITE_BOM, WRITE_NULL_TERMINATOR); \
		} \
	} else if (CHAR_ENCODING == CharacterEncoding::BOM_UTF16LE) { \
		if (LINE_ENCODING == LineEncoding::CrLf) { \
			encodeText<CharacterEncoding::BOM_UTF16LE, LineEncoding::CrLf>(RECEIVER, CONTENT, WRITE_BOM, WRITE_NULL_TERMINATOR); \
		} else if (LINE_ENCODING == LineEncoding::Lf) { \
			encodeText<CharacterEncoding::BOM_UTF16LE, LineEncoding::Lf>(RECEIVER, CONTENT, WRITE_BOM, WRITE_NULL_TERMINATOR); \
		} \
	}

// Encoding to a buffer before saving all at once as a binary file.
//   This tells the operating system how big the file is in advance and prevent the worst case of stalling for minutes!
bool dsr::string_save(const ReadableString& filename, const ReadableString& content, CharacterEncoding characterEncoding, LineEncoding lineEncoding) {
	Buffer buffer = string_saveToMemory(content, characterEncoding, lineEncoding);
	if (buffer_exists(buffer)) {
		return file_saveBuffer(filename, buffer);
	} else {
		return false;
	}
}

Buffer dsr::string_saveToMemory(const ReadableString& content, CharacterEncoding characterEncoding, LineEncoding lineEncoding, bool writeByteOrderMark, bool writeNullTerminator) {
	int64_t byteCount = 0;
	ByteWriterFunction counter = [&byteCount](uint8_t value) {
		byteCount++;
	};
	ENCODE_TEXT(counter, content, characterEncoding, lineEncoding, writeByteOrderMark, writeNullTerminator);
	Buffer result = buffer_create(byteCount);
	SafePointer<uint8_t> byteWriter = buffer_getSafeData<uint8_t>(result, "Buffer for string encoding");
	ByteWriterFunction receiver = [&byteWriter](uint8_t value) {
		*byteWriter = value;
		byteWriter += 1;
	};
	ENCODE_TEXT(receiver, content, characterEncoding, lineEncoding, writeByteOrderMark, writeNullTerminator);
	return result;
}

static int64_t getNewBufferSize(int64_t minimumSize) {
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
		return minimumSize;
	}
}
// Replaces the buffer with a new buffer holding at least newLength characters
// Guarantees that the new buffer is not shared by other strings, so that it may be written to freely
static void reallocateBuffer(String &target, int64_t newLength, bool preserve) {
	// Holding oldData alive while copying to the new buffer
	Buffer oldBuffer = target.buffer; // Kept for reference counting only, do not remove.
	const char32_t* oldData = target.readSection;
	target.buffer = buffer_create(getNewBufferSize(newLength * sizeof(DsrChar)));
	target.readSection = target.writeSection = reinterpret_cast<char32_t*>(buffer_dangerous_getUnsafeData(target.buffer));
	if (preserve && oldData) {
		memcpy(target.writeSection, oldData, target.length * sizeof(DsrChar));
	}
}
// Call before writing to the buffer
//   This hides that Strings share buffers when assigning by value or taking partial strings
static void cloneIfShared(String &target) {
	if (target.buffer.use_count() > 1) {
		reallocateBuffer(target, target.length, true);
	}
}

void dsr::string_clear(String& target) {
	cloneIfShared(target);
	target.length = 0;
}

// The number of DsrChar characters that can be contained in the allocation before reaching the buffer's end
//   This doesn't imply that it's always okay to write to the remaining space, because the buffer may be shared
static int64_t getCapacity(const ReadableString &source) {
	if (buffer_exists(source.buffer)) {
		// Get the allocation
		uint8_t* data = buffer_dangerous_getUnsafeData(source.buffer);
		uint8_t* start = (uint8_t*)(source.readSection);
		// Get the offset from the parent
		intptr_t offset = start - data;
		// Subtract offset from the buffer size to get the remaining space
		return (buffer_getSize(source.buffer) - offset) / sizeof(DsrChar);
	} else {
		return 0;
	}
}

static void expand(String &target, int64_t newLength, bool affectUsedLength) {
	cloneIfShared(target);
	if (newLength > target.length) {
		if (newLength > getCapacity(target)) {
			reallocateBuffer(target, newLength, true);
		}
		if (affectUsedLength) {
			target.length = newLength;
		}
	}
}

void dsr::string_reserve(String& target, int64_t minimumLength) {
	expand(target, minimumLength, false);
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
	int64_t oldLength = (TARGET).length; \
	expand((TARGET), oldLength + (int64_t)(LENGTH), true); \
	for (int64_t i = 0; i < (int64_t)(LENGTH); i++) { \
		(TARGET).writeSection[oldLength + i] = ((SOURCE)[i]) & MASK; \
	} \
}
// TODO: See if ascii litterals can be checked for values above 127 in compile-time
static void atomic_append(String &target, const char* source) { APPEND(target, source, strlen(source), 0xFF); }
// TODO: Use memcpy when appending input of the same format
static void atomic_append(String &target, const ReadableString& source) { APPEND(target, source, source.length, 0xFFFFFFFF); }
static void atomic_append(String &target, const char32_t* source) { APPEND(target, source, strlen_utf32(source), 0xFFFFFFFF); }
static void atomic_append(String &target, const std::string& source) { APPEND(target, source.c_str(), (int64_t)source.size(), 0xFF); }

void dsr::string_appendChar(String& target, DsrChar value) { APPEND(target, &value, 1, 0xFFFFFFFF); }

String& dsr::string_toStreamIndented(String& target, const Printable& source, const ReadableString& indentation) {
	return source.toStreamIndented(target, indentation);
}
String& dsr::string_toStreamIndented(String& target, const char* value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	atomic_append(target, value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const ReadableString& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	atomic_append(target, value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const char32_t* value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	atomic_append(target, value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const std::string& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	atomic_append(target, value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const float& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	doubleToString_arabic(target, (double)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const double& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	doubleToString_arabic(target, value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const int64_t& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	intToString_arabic(target, value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const uint64_t& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	uintToString_arabic(target, value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const int32_t& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	intToString_arabic(target, (int64_t)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const uint32_t& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	uintToString_arabic(target, (uint64_t)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const int16_t& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	intToString_arabic(target, (int64_t)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const uint16_t& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	uintToString_arabic(target, (uint64_t)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const int8_t& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	intToString_arabic(target, (int64_t)value);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const uint8_t& value, const ReadableString& indentation) {
	atomic_append(target, indentation);
	uintToString_arabic(target, (uint64_t)value);
	return target;
}

static const std::function<void(const ReadableString &message, MessageType type)> defaultMessageAction = [](const ReadableString &message, MessageType type) {
	if (type == MessageType::Error) {
		throw std::runtime_error(message.toStdString());
	} else {
		message.toStream(std::cout);
	}
};

static std::function<void(const ReadableString &message, MessageType type)> globalMessageAction = defaultMessageAction;

void dsr::string_sendMessage(const ReadableString &message, MessageType type) {
	globalMessageAction(message, type);
	if (type == MessageType::Error) {
		throw std::runtime_error("The message handler provided using string_assignMessageHandler did not throw an exception or terminate the program for the given error!\n");
	}
}

void dsr::string_sendMessage_default(const ReadableString &message, MessageType type) {
	defaultMessageAction(message, type);
}

void dsr::string_assignMessageHandler(std::function<void(const ReadableString &message, MessageType type)> newHandler) {
	globalMessageAction = newHandler;
}

void dsr::string_unassignMessageHandler() {
	globalMessageAction = defaultMessageAction;
}

void dsr::string_split_callback(std::function<void(ReadableString separatedText)> action, const ReadableString& source, DsrChar separator, bool removeWhiteSpace) {
	int64_t sectionStart = 0;
	for (int64_t i = 0; i < source.length; i++) {
		DsrChar c = source[i];
		if (c == separator) {
			ReadableString element = string_exclusiveRange(source, sectionStart, i);
			if (removeWhiteSpace) {
				action(string_removeOuterWhiteSpace(element));
			} else {
				action(element);
			}
			sectionStart = i + 1;
		}
	}
	if (source.length > sectionStart) {
		if (removeWhiteSpace) {
			action(string_removeOuterWhiteSpace(string_exclusiveRange(source, sectionStart, source.length)));
		} else {
			action(string_exclusiveRange(source, sectionStart, source.length));
		}
	}
}

List<String> dsr::string_split(const ReadableString& source, DsrChar separator, bool removeWhiteSpace) {
	List<String> result;
	String commonBuffer;
	if (buffer_exists(source.buffer)) {
		// Re-use the existing buffer
		commonBuffer = createSubString_shared(source.readSection, source.length, source.buffer, const_cast<char32_t*>(source.readSection));
	} else {
		// Clone the whole input into one allocation to avoid fragmenting the heap with many small allocations
		commonBuffer = source;
	}
	// Source is allocated as String
	string_split_callback([&result, removeWhiteSpace](String element) {
		if (removeWhiteSpace) {
			result.push(string_removeOuterWhiteSpace(element));
		} else {
			result.push(element);
		}
	}, commonBuffer, separator, removeWhiteSpace);
	return result;
}

int64_t dsr::string_splitCount(const ReadableString& source, DsrChar separator) {
	int64_t result;
	string_split_callback([&result](ReadableString element) {
		result++;
	}, source, separator);
	return result;
}

int64_t dsr::string_toInteger(const ReadableString& source) {
	int64_t result;
	bool negated;
	result = 0;
	negated = false;
	for (int64_t i = 0; i < source.length; i++) {
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
	int64_t digitDivider;
	result = 0.0;
	negated = false;
	reachedDecimal = false;
	digitDivider = 1;
	for (int64_t i = 0; i < source.length; i++) {
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

int64_t dsr::string_length(const ReadableString& source) {
	return source.length;
}

int64_t dsr::string_findFirst(const ReadableString& source, DsrChar toFind, int64_t startIndex) {
	for (int64_t i = startIndex; i < source.length; i++) {
		if (source[i] == toFind) {
			return i;
		}
	}
	return -1;
}

int64_t dsr::string_findLast(const ReadableString& source, DsrChar toFind) {
	for (int64_t i = source.length - 1; i >= 0; i--) {
		if (source[i] == toFind) {
			return i;
		}
	}
	return -1;
}

ReadableString dsr::string_exclusiveRange(const ReadableString& source, int64_t inclusiveStart, int64_t exclusiveEnd) {
	// Return empty string for each complete miss
	if (inclusiveStart >= source.length || exclusiveEnd <= 0) { return ReadableString(); }
	// Automatically clamping to valid range
	if (inclusiveStart < 0) { inclusiveStart = 0; }
	if (exclusiveEnd > source.length) { exclusiveEnd = source.length; }
	// Return the overlapping interval
	return createSubString(&(source.readSection[inclusiveStart]), exclusiveEnd - inclusiveStart, source.buffer);
}

ReadableString dsr::string_inclusiveRange(const ReadableString& source, int64_t inclusiveStart, int64_t inclusiveEnd) {
	return string_exclusiveRange(source, inclusiveStart, inclusiveEnd + 1);
}

ReadableString dsr::string_before(const ReadableString& source, int64_t exclusiveEnd) {
	return string_exclusiveRange(source, 0, exclusiveEnd);
}

ReadableString dsr::string_until(const ReadableString& source, int64_t inclusiveEnd) {
	return string_inclusiveRange(source, 0, inclusiveEnd);
}

ReadableString dsr::string_from(const ReadableString& source, int64_t inclusiveStart) {
	return string_exclusiveRange(source, inclusiveStart, source.length);
}

ReadableString dsr::string_after(const ReadableString& source, int64_t exclusiveStart) {
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
	int64_t readIndex = 0;
	if (allowWhiteSpace) {
		PATTERN_STAR(WhiteSpace);
	}
	CHARACTER_OPTIONAL(U'-');
	// At least one digit required
	PATTERN_PLUS(IntegerCharacter);
	if (allowWhiteSpace) {
		PATTERN_STAR(WhiteSpace);
	}
	return readIndex == source.length;
}

// To avoid consuming the all digits on Digit* before reaching Digit+ when there is no decimal, whole integers are judged by string_isInteger
bool dsr::string_isDouble(const ReadableString& source, bool allowWhiteSpace) {
	// Solving the UnsignedDouble <- Digit+ | Digit* '.' Digit+ ambiguity is done easiest by checking if there's a decimal before handling the white-space and negation
	if (string_findFirst(source, U'.') == -1) {
		// No decimal detected
		return string_isInteger(source, allowWhiteSpace);
	} else {
		int64_t readIndex = 0;
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
		return readIndex == source.length;
	}
}

int64_t dsr::string_getBufferUseCount(const ReadableString& text) {
	return text.buffer.use_count();
}
