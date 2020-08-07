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
	// Returning the character by value prevents writing to memory that might be a constant literal or shared with other strings
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
public:
	// Create a string from an existing string
	// When there's no reference counter, it's important that the memory remains allocated until the application terminates
	// Just like when reading elements in a for loop, out-of-range only causes an exception if length > 0
	//   Length lesser than 1 will always return an empty string
	virtual ReadableString getRange(int start, int length) const;
	// Converting to unknown character encoding using only the ascii character subset
	// A bug in GCC linking forces these to be virtual
	virtual std::ostream& toStream(std::ostream& out) const;
	virtual std::string toStdString() const;
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
	// Internal constructor
	String(std::shared_ptr<Buffer> buffer, DsrChar *content, int sectionLength);
public:
	// The number of DsrChar characters that can be contained in the allocation before reaching the buffer's end
	//   This doesn't imply that it's always okay to write to the remaining space, because the buffer may be shared
	int capacity();
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


// ---------------- Procedural API ----------------


// Post-condition: Returns the length of source.
//   Example: string_length(U"ABC") == 3
int string_length(const ReadableString& source);
// Post-condition: Returns the base-zero index of source's first occurence of toFind, starting from startIndex. Returns -1 if not found.
//   Example: string_findFirst(U"ABCABCABC", U'A') == 0
//   Example: string_findFirst(U"ABCABCABC", U'B') == 1
//   Example: string_findFirst(U"ABCABCABC", U'C') == 2
//   Example: string_findFirst(U"ABCABCABC", U'D') == -1
int string_findFirst(const ReadableString& source, DsrChar toFind, int startIndex = 0);
// Post-condition: Returns the base-zero index of source's last occurence of toFind.  Returns -1 if not found.
//   Example: string_findLast(U"ABCABCABC", U'A') == 6
//   Example: string_findLast(U"ABCABCABC", U'B') == 7
//   Example: string_findLast(U"ABCABCABC", U'C') == 8
//   Example: string_findLast(U"ABCABCABC", U'D') == -1
int string_findLast(const ReadableString& source, DsrChar toFind);
// Post-condition: Returns a sub-string of source from before the character at inclusiveStart to before the character at exclusiveEnd
//   Example: string_exclusiveRange(U"0123456789", 2, 4) == U"23"
ReadableString string_exclusiveRange(const ReadableString& source, int inclusiveStart, int exclusiveEnd);
// Post-condition: Returns a sub-string of source from before the character at inclusiveStart to after the character at inclusiveEnd
//   Example: string_inclusiveRange(U"0123456789", 2, 4) == U"234"
ReadableString string_inclusiveRange(const ReadableString& source, int inclusiveStart, int inclusiveEnd);
// Post-condition: Returns a sub-string of source from the start to before the character at exclusiveEnd
//   Example: string_before(U"0123456789", 5) == U"01234"
ReadableString string_before(const ReadableString& source, int exclusiveEnd);
// Post-condition: Returns a sub-string of source from the start to after the character at inclusiveEnd
//   Example: string_until(U"0123456789", 5) == U"012345"
ReadableString string_until(const ReadableString& source, int inclusiveEnd);
// Post-condition: Returns a sub-string of source from before the character at inclusiveStart to the end
//   Example: string_from(U"0123456789", 5) == U"56789"
ReadableString string_from(const ReadableString& source, int inclusiveStart);
// Post-condition: Returns a sub-string of source from after the character at exclusiveStart to the end
//   Example: string_after(U"0123456789", 5) == U"6789"
ReadableString string_after(const ReadableString& source, int exclusiveStart);

// Post-condition:
//   Returns a list of strings from source by splitting along separator.
// The separating characters are excluded from the resulting strings.
// The number of strings returned in the list will equal the number of separating characters plus one, so the result may contain empty strings.
// Each string in the list reuses memory from the input string using reference counting, but the list itself will be allocated.
List<ReadableString> string_split(const ReadableString& source, DsrChar separator);
// Use string_split_inPlace instead of string_split if you want to reuse the memory of an existing list.
//   It will then only allocate when running out of buffer space.
// Side-effects:
//   Fills the target list with strings from source by splitting along separator.
//   If appendResult is false (default), any pre-existing elements in the target list will be cleared before writing the result.
//   If appendResult is true, the result is appended to the existing target list.
void string_split_inPlace(List<ReadableString> &target, const ReadableString& source, DsrChar separator, bool appendResult = false);

// Post-condition: Returns the integer representation of source.
// The result is signed, because the input might unexpectedly have a negation sign.
// The result is large, so that one can easily check the range before assigning to a smaller integer type.
int64_t string_toInteger(const ReadableString& source);
// Post-condition: Returns the double precision floating-point representation of source.
double string_toDouble(const ReadableString& source);

// Post-condition:
//   Returns the content of the file referred to be filename.
//   If mustExist is true, then failure to load will throw an exception.
//   If mustExist is false, then failure to load will return an empty string.
String string_load(const ReadableString& filename, bool mustExist = true);
// Side-effect: Saves content to filename.
void string_save(const ReadableString& filename, const ReadableString& content);

// Post-condition: Returns true iff strings a and b are exactly equal.
bool string_match(const ReadableString& a, const ReadableString& b);
// Post-condition: Returns true iff strings a and b are roughly equal using a case insensitive match.
bool string_caseInsensitiveMatch(const ReadableString& a, const ReadableString& b);

// Post-condition: Returns text converted to upper case.
String string_upperCase(const ReadableString &text);
// Post-condition: Returns text converted to lower case.
String string_lowerCase(const ReadableString &text);

// Post-condition: Returns a clone of text without any white-space (space, tab and carriage-return).
String string_removeAllWhiteSpace(const ReadableString &text);
// Post-condition: Returns a sub-set of text without surrounding white-space (space, tab and carriage-return).
// Unlike string_removeAllWhiteSpace, string_removeOuterWhiteSpace does not require allocating a new buffer.
ReadableString string_removeOuterWhiteSpace(const ReadableString &text);

// Post-condition: Returns rawText wrapped in a quote.
// Special characters are included using escape characters, so that one can quote multiple lines but store it easily.
String string_mangleQuote(const ReadableString &rawText);
// Pre-condition: mangledText must be enclosed in double quotes and special characters must use escape characters (tabs in quotes are okay though).
// Post-condition: Returns mangledText with quotes removed and excape tokens interpreted.
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


// ---------------- Infix syntax ----------------


// Operations
inline String operator+ (const ReadableString& a, const ReadableString& b) { return string_combine(a, b); }
inline String operator+ (const char32_t* a, const ReadableString& b) { return string_combine(a, b); }
inline String operator+ (const ReadableString& a, const char32_t* b) { return string_combine(a, b); }
inline String operator+ (const String& a, const String& b) { return string_combine(a, b); }
inline String operator+ (const char32_t* a, const String& b) { return string_combine(a, b); }
inline String operator+ (const String& a, const char32_t* b) { return string_combine(a, b); }
inline String operator+ (const String& a, const ReadableString& b) { return string_combine(a, b); }
inline String operator+ (const ReadableString& a, const String& b) { return string_combine(a, b); }


// Methods used so often that they don't need to use the string_ prefix


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


// ---------------- Hard-coded portability for specific operating systems ----------------
// TODO: Try to find a place for this outside of the library, similar to how window managers were implemented


// Get a path separator for the target operating system.
const char32_t* file_separator();


}

#endif

