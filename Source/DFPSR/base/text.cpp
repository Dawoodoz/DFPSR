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

String dsr::string_load(const ReadableString& filename, bool mustExist) {
	// TODO: Load files using Unicode filenames
	TO_RAW_ASCII(asciiFilename, filename);
	std::ifstream inputFile(asciiFilename);
	if (inputFile.is_open()) {
		std::stringstream outputBuffer;
		// TODO: Feed directly to String
		outputBuffer << inputFile.rdbuf();
		std::string content = outputBuffer.str();
		String result;
		result.reserve(content.size());
		for (int i = 0; i < (int)(content.size()); i++) {
			result.appendChar(content[i]);
		}
		inputFile.close();
		return result;
	} else {
		if (mustExist) {
			throwError("Failed to load ", filename, "\n");
		}
		// If the file cound not be found and opened, a null string is returned
		return String();
	}
}

void dsr::string_save(const ReadableString& filename, const ReadableString& content) {
	// TODO: Load files using Unicode filenames
	TO_RAW_ASCII(asciiFilename, filename);
	TO_RAW_ASCII(asciiContent, content);
	std::ofstream outputFile;
	outputFile.open(asciiFilename);
	if (outputFile.is_open()) {
		outputFile << asciiContent;
		outputFile.close();
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
#define APPEND(TARGET, SOURCE, LENGTH) { \
	int oldLength = (TARGET)->length(); \
	(TARGET)->expand(oldLength + (int)(LENGTH), true); \
	for (int i = 0; i < (int)(LENGTH); i++) { \
		(TARGET)->write(oldLength + i, (SOURCE)[i]); \
	} \
}
// TODO: See if ascii litterals can be checked for values above 127 in compile-time
void String::append(const char* source) { APPEND(this, source, strlen(source)); }
// TODO: Use memcpy when appending input of the same format
void String::append(const ReadableString& source) { APPEND(this, source, source.length()); }
void String::append(const char32_t* source) { APPEND(this, source, strlen_utf32(source)); }
void String::append(const std::string& source) { APPEND(this, source.c_str(), (int)source.size()); }
void String::appendChar(DsrChar source) { APPEND(this, &source, 1); }

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

