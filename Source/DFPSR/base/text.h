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

#ifndef DFPSR_BASE_TEXT
#define DFPSR_BASE_TEXT

#include <stdint.h>
#include <string>

// TODO: Try to hide in the implementation
#include <iostream>
#include <sstream>

#include "Buffer.h"
#include "../collection/List.h"

namespace dsr {

using DsrChar = char32_t;

class ReadableString {
protected:
	// A local pointer to the sub-allocation
	const char32_t* readSection = nullptr;
	// The length of the current string in characters
	int sectionLength = 0;
public:
	int length() const;
	DsrChar read(int index) const;
	DsrChar operator[] (int index) const;
public:
	// Empty string
	ReadableString();
	// Destructor
	virtual ~ReadableString();
	// UTF-32 litteral from U""
	// WARNING! May crash if content is freed, even if ReadableString is freed before
	//          ReadableString may share its buffer with sub-strings of the same type
	ReadableString(const DsrChar *content);
protected:
	// Returns true iff the range is safely inside of the string
	bool checkBound(int start, int length, bool warning = true) const;
	// Internal constructor
	ReadableString(const DsrChar *content, int sectionLength);
	// Create a string from an existing string
	// When there's no reference counter, it's important that the memory remains allocated until the application terminates
	// Just like when reading elements in a for loop, out-of-range only causes an exception if length > 0
	//   Length lesser than 1 will always return an empty string
	virtual ReadableString getRange(int start, int length) const;
public:
	// Converting to unknown character encoding using only the ascii character subset
	// A bug in GCC linking forces these to be virtual
	virtual std::ostream& toStream(std::ostream& out) const;
	virtual std::string toStdString() const;
public:
	// Get the index of the first character in content matching toFind, or -1 if it doesn't exist.
	int findFirst(DsrChar toFind, int startIndex = 0) const;
	// Get the index of the last character in content matching toFind, or -1 if it doesn't exist.
	int findLast(DsrChar toFind) const;
	// Exclusive intervals represent the divisions between characters |⁰ A |¹ B |² C |³...
	//   0..2 of "ABC" then equals "AB", which has length 2 just like the index difference
	//   0..3 gets the whole "ABC" range, by starting from zero and ending with the character count
	ReadableString exclusiveRange(int inclusiveStart, int exclusiveEnd) const;
	// Inclusive intervals represent whole characters | A⁰ | B¹ | C² |...
	//   0..2 of "ABC" then equals "ABC", by taking character 0 (A), 1 (B) and 2 (C)
	ReadableString inclusiveRange(int inclusiveStart, int inclusiveEnd) const;
	// Simplified ranges
	ReadableString before(int exclusiveEnd) const;
	ReadableString until(int inclusiveEnd) const;
	ReadableString from(int inclusiveStart) const;
	ReadableString after(int exclusiveStart) const;
	// Split into a list of strings without allocating any new text buffers
	//   The result can be kept after the original string has been freed, because the buffer is reference counted
	List<ReadableString> split(DsrChar separator) const;
	// Value conversion
	int64_t toInteger() const;
	double toDouble() const;
};

class String;

// Reusable conversion methods
void uintToString_arabic(String& target, uint64_t value);
void intToString_arabic(String& target, int64_t value);
void doubleToString_arabic(String& target, double value);

// Used as format tags around numbers passed to string_append or string_combine
// New types can implement printing to String by making wrappers from this class
class Printable {
public:
	// The method for appending the printable object into the target string
	virtual String& toStreamIndented(String& target, const ReadableString& indentation) const = 0;
	String& toStream(String& target) const;
	String toStringIndented(const ReadableString& indentation) const;
	String toString() const;
	std::ostream& toStreamIndented(std::ostream& out, const ReadableString& indentation) const;
	std::ostream& toStream(std::ostream& out) const;
	std::string toStdString() const;
	virtual ~Printable();
};

// A safe and simple string type
//   Can be constructed from ascii litterals "", but U"" is more universal
//   Can be used without ReadableString, but ReadableString can be wrapped over U"" litterals without allocation
//   UTF-32
//     Endianness is native
//     No combined characters allowed, use precomposed instead, so that the strings can guarantee a fixed character size
class String : public ReadableString {
protected:
	// A reference counted pointer to the buffer, just to keep the allocation
	std::shared_ptr<Buffer> buffer;
	// Same as readSection, but with write access
	char32_t* writeSection = nullptr;
public:
	// The number of DsrChar characters that can be contained in the allocation before reaching the buffer's end
	//   This doesn't imply that it's always okay to write to the remaining space, because the buffer may be shared
	int capacity();
protected:
	// Internal constructor
	String(std::shared_ptr<Buffer> buffer, DsrChar *content, int sectionLength);
	// Create a string from the existing buffer without allocating any heap memory
	ReadableString getRange(int start, int length) const override;
private:
	// Replaces the buffer with a new buffer holding at least newLength characters
	// Guarantees that the new buffer is not shared by other strings, so that it may be written to freely
	void reallocateBuffer(int32_t newLength, bool preserve);
	// Call before writing to the buffer
	//   This hides that Strings share buffers when assigning by value or taking partial strings
	void cloneIfShared();
	void expand(int32_t newLength, bool affectUsedLength);
public:
	// Constructors
	String();
	String(const char* source);
	String(const char32_t* source);
	String(const std::string& source);
	String(const ReadableString& source);
	String(const String& source);
public:
	// Ensures safely that at least minimumLength characters can he held in the buffer
	void reserve(int32_t minimumLength);
	// Extend the String using more text
	void append(const char* source);
	void append(const ReadableString& source);
	void append(const char32_t* source);
	void append(const std::string& source);
	// Extend the String using another character
	void appendChar(DsrChar source);
public:
	// Access
	void write(int index, DsrChar value);
	void clear();
};

// Define this overload for non-virtual source types that cannot inherit from Printable
String& string_toStreamIndented(String& target, const Printable& source, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const char* value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const ReadableString& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const char32_t* value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const std::string& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const float& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const double& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const int64_t& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const uint64_t& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const int32_t& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const uint32_t& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const int16_t& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const uint16_t& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const int8_t& value, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const uint8_t& value, const ReadableString& indentation);

// Procedural API
// TODO: Create procedural constructors
// TODO: Make wrappers around member methods
String string_load(const ReadableString& filename);
void string_save(const ReadableString& filename, const ReadableString& content);
bool string_match(const ReadableString& a, const ReadableString& b);
bool string_caseInsensitiveMatch(const ReadableString& a, const ReadableString& b);
String string_upperCase(const ReadableString &text);
String string_lowerCase(const ReadableString &text);
String string_removeAllWhiteSpace(const ReadableString &text);
ReadableString string_removeOuterWhiteSpace(const ReadableString &text);
int64_t string_parseInteger(const ReadableString& content);
double string_parseDouble(const ReadableString& content);
String string_mangleQuote(const ReadableString &rawText);
String string_unmangleQuote(const ReadableString& mangledText);
// Append one element
template<typename TYPE>
inline void string_append(String& target, TYPE value) {
	string_toStream(target, value);
}
// Append multiple elements
template<typename HEAD, typename... TAIL>
inline void string_append(String& target, HEAD head, TAIL... tail) {
	string_append(target, head);
	string_append(target, tail...);
}
// Combine a number of strings, characters and numbers
//   If an input type is rejected, create a Printable object to wrap around it
template<typename... ARGS>
inline String string_combine(ARGS... args) {
	String result;
	string_append(result, args...);
	return result;
}

// Operations
inline String operator+ (const ReadableString& a, const ReadableString& b) { return string_combine(a, b); }
inline String operator+ (const char32_t* a, const ReadableString& b) { return string_combine(a, b); }
inline String operator+ (const ReadableString& a, const char32_t* b) { return string_combine(a, b); }
inline String operator+ (const String& a, const String& b) { return string_combine(a, b); }
inline String operator+ (const char32_t* a, const String& b) { return string_combine(a, b); }
inline String operator+ (const String& a, const char32_t* b) { return string_combine(a, b); }
inline String operator+ (const String& a, const ReadableString& b) { return string_combine(a, b); }
inline String operator+ (const ReadableString& a, const String& b) { return string_combine(a, b); }

// Print information
template<typename... ARGS>
void printText(ARGS... args) {
	String result = string_combine(args...);
	result.toStream(std::cout);
}

// Use for text printing that are useful when debugging but should not be given out in a release
#ifdef NDEBUG
	// Supress debugText in release mode
	template<typename... ARGS>
	void debugText(ARGS... args) {}
#else
	// Print debugText in debug mode
	template<typename... ARGS>
	void debugText(ARGS... args) { printText(args...); }
#endif

// Raise an exception
//   Only catch errors to display useful error messages, emergency backups or crash logs before terminating
//   Further execution after a partial transaction will break object invariants
void throwErrorMessage(const String& message);
template<typename... ARGS>
void throwError(ARGS... args) {
	String result = string_combine(args...);
	throwErrorMessage(result);
}


// ---------------- Overloaded serialization ----------------


// Templates reused for all types
// The source must inherit from Printable or have its own string_feedIndented overload
template<typename T>
String& string_toStream(String& target, const T& source) {
	return string_toStreamIndented(target, source, U"");
}
template<typename T>
String string_toStringIndented(const T& source, const ReadableString& indentation) {
	String result;
	string_toStreamIndented(result, source, indentation);
	return result;
}
template<typename T>
String string_toString(const T& source) {
	return string_toStringIndented(source, U"");
}
template<typename T>
std::ostream& string_toStreamIndented(std::ostream& target, const T& source, const ReadableString& indentation) {
	return target << string_toStringIndented(source, indentation);
}
template<typename T>
std::ostream& string_toStream(std::ostream& target, const T& source) {
	return target << string_toString(source);
}

// ---------------- Below uses hard-coded portability for specific operating systems ----------------


// Get a path separator for the target operating system.
const char32_t* file_separator();


}

#endif

