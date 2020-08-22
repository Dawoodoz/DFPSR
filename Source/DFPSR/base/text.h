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

#ifndef DFPSR_BASE_TEXT
#define DFPSR_BASE_TEXT

#include <stdint.h>
#include <string>
#include <iostream>
#include <sstream>
#include <functional>
#include "../api/bufferAPI.h"
#include "../collection/List.h"

// Define DFPSR_INTERNAL_ACCESS before any include to get internal access to exposed types
#ifdef DFPSR_INTERNAL_ACCESS
	#define IMPL_ACCESS public
#else
	#define IMPL_ACCESS protected
#endif

namespace dsr {

using DsrChar = char32_t;

// Text files support loading UTF-8/16 BE/LE with BOM or Latin-1 without BOM
enum class CharacterEncoding {
	Raw_Latin1,  // U+00 to U+FF
	BOM_UTF8,    // U+00000000 to U+0010FFFF
	BOM_UTF16BE, // U+00000000 to U+0000D7FF, U+0000E000 to U+0010FFFF
	BOM_UTF16LE  // U+00000000 to U+0000D7FF, U+0000E000 to U+0010FFFF
};

// Carriage-return is removed when loading text files to prevent getting double lines
// A line-feed without a line-feed character is nonsense
// LineEncoding allow re-adding carriage-return before or after each line-break when saving
enum class LineEncoding {
	CrLf, // Microsoft Windows compatible (Can also be read on other platforms by ignoring carriage return)
	Lf // Linux and Macintosh compatible (Might not work on non-portable text editors on Microsoft Windows)
};

class ReadableString {
IMPL_ACCESS:
	// A local pointer to the sub-allocation
	const char32_t* readSection = nullptr;
	// The length of the current string in characters
	int64_t length = 0;
public:
	DsrChar read(int64_t index) const;
	// Returning the character by value prevents writing to memory that might be a constant literal or shared with other strings
	DsrChar operator[] (int64_t index) const;
public:
	// Empty string
	ReadableString();
	// Destructor
	virtual ~ReadableString();
	// UTF-32 litteral from U""
	// WARNING! May crash if content is freed, even if ReadableString is freed before
	//          ReadableString may share its buffer with sub-strings of the same type
	ReadableString(const DsrChar *content);
IMPL_ACCESS:
	// Returns true iff the range is safely inside of the string
	bool checkBound(int64_t start, int64_t length, bool warning = true) const;
	// Internal constructor
	ReadableString(const DsrChar *content, int64_t length);
public:
	// Create a string from an existing string
	// When there's no reference counter, it's important that the memory remains allocated until the application terminates
	// Just like when reading elements in a for loop, out-of-range only causes an exception if length > 0
	//   Length lesser than 1 will always return an empty string
	virtual ReadableString getRange(int64_t start, int64_t length) const;
	// Converting to unknown character encoding using only the ascii character subset
	// A bug in GCC linking forces these to be virtual
	virtual std::ostream& toStream(std::ostream& out) const;
	virtual std::string toStdString() const;
};

class String;

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
IMPL_ACCESS:
	// A reference counted pointer to the buffer, just to keep the allocation
	Buffer buffer;
	// Same as readSection, but with write access
	char32_t* writeSection = nullptr;
	// Internal constructor
	String(Buffer buffer, DsrChar *content, int64_t length);
public:
	// The number of DsrChar characters that can be contained in the allocation before reaching the buffer's end
	//   This doesn't imply that it's always okay to write to the remaining space, because the buffer may be shared
	int64_t capacity();
	// Create a string from the existing buffer without allocating any heap memory
	ReadableString getRange(int64_t start, int64_t length) const override;
IMPL_ACCESS:
	// Replaces the buffer with a new buffer holding at least newLength characters
	// Guarantees that the new buffer is not shared by other strings, so that it may be written to freely
	void reallocateBuffer(int64_t newLength, bool preserve);
	// Call before writing to the buffer
	//   This hides that Strings share buffers when assigning by value or taking partial strings
	void cloneIfShared();
	void expand(int64_t newLength, bool affectUsedLength);
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
	void reserve(int64_t minimumLength);
	// Extend the String using more text
	void append(const char* source);
	void append(const ReadableString& source);
	void append(const char32_t* source);
	void append(const std::string& source);
	// Extend the String using another character
	void appendChar(DsrChar source);
public:
	// Access
	void write(int64_t index, DsrChar value);
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
int64_t string_length(const ReadableString& source);
// Post-condition: Returns the base-zero index of source's first occurence of toFind, starting from startIndex. Returns -1 if not found.
//   Example: string_findFirst(U"ABCABCABC", U'A') == 0
//   Example: string_findFirst(U"ABCABCABC", U'B') == 1
//   Example: string_findFirst(U"ABCABCABC", U'C') == 2
//   Example: string_findFirst(U"ABCABCABC", U'D') == -1
int64_t string_findFirst(const ReadableString& source, DsrChar toFind, int64_t startIndex = 0);
// Post-condition: Returns the base-zero index of source's last occurence of toFind.  Returns -1 if not found.
//   Example: string_findLast(U"ABCABCABC", U'A') == 6
//   Example: string_findLast(U"ABCABCABC", U'B') == 7
//   Example: string_findLast(U"ABCABCABC", U'C') == 8
//   Example: string_findLast(U"ABCABCABC", U'D') == -1
int64_t string_findLast(const ReadableString& source, DsrChar toFind);
// Post-condition: Returns a sub-string of source from before the character at inclusiveStart to before the character at exclusiveEnd
//   Example: string_exclusiveRange(U"0123456789", 2, 4) == U"23"
ReadableString string_exclusiveRange(const ReadableString& source, int64_t inclusiveStart, int64_t exclusiveEnd);
// Post-condition: Returns a sub-string of source from before the character at inclusiveStart to after the character at inclusiveEnd
//   Example: string_inclusiveRange(U"0123456789", 2, 4) == U"234"
ReadableString string_inclusiveRange(const ReadableString& source, int64_t inclusiveStart, int64_t inclusiveEnd);
// Post-condition: Returns a sub-string of source from the start to before the character at exclusiveEnd
//   Example: string_before(U"0123456789", 5) == U"01234"
ReadableString string_before(const ReadableString& source, int64_t exclusiveEnd);
// Post-condition: Returns a sub-string of source from the start to after the character at inclusiveEnd
//   Example: string_until(U"0123456789", 5) == U"012345"
ReadableString string_until(const ReadableString& source, int64_t inclusiveEnd);
// Post-condition: Returns a sub-string of source from before the character at inclusiveStart to the end
//   Example: string_from(U"0123456789", 5) == U"56789"
ReadableString string_from(const ReadableString& source, int64_t inclusiveStart);
// Post-condition: Returns a sub-string of source from after the character at exclusiveStart to the end
//   Example: string_after(U"0123456789", 5) == U"6789"
ReadableString string_after(const ReadableString& source, int64_t exclusiveStart);

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
// Split a string without needing a list to store the result.
//   This allow filtering and processing before saving the results,
//   but makes debugging more difficult and prevents pattern detection.
// Side-effects:
//   Calls action for each sub-string divided by separator in source.
void string_split_callback(std::function<void(ReadableString)> action, const ReadableString& source, DsrChar separator);

// Post-condition: Returns true iff c is a digit.
//   Digit <- '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
bool character_isDigit(DsrChar c);
// Post-condition: Returns true iff c is an integer character.
//   IntegerCharacter <- '-' | '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
bool character_isIntegerCharacter(DsrChar c);
// Post-condition: Returns true iff c is a value character.
//   ValueCharacter <- '.' | '-' | '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
bool character_isValueCharacter(DsrChar c);
// Post-condition: Returns true iff c is a white-space character.
//   WhiteSpace <- ' ' | '\t' | '\v' | '\f' | '\n' | '\r'
//   Null terminators are excluded, because it's reserved for out of bound results.
bool character_isWhiteSpace(DsrChar c);
// Post-condition: Returns true iff source is a valid integer. IntegerAllowingWhiteSpace is also allowed iff allowWhiteSpace is true.
//   UnsignedInteger <- Digit+
//   Integer <- UnsignedInteger | '-' UnsignedInteger
//   IntegerAllowingWhiteSpace <- WhiteSpace* Integer WhiteSpace*
bool string_isInteger(const ReadableString& source, bool allowWhiteSpace = true);
// Post-condition: Returns true iff source is a valid integer or decimal number. DoubleAllowingWhiteSpace is also allowed iff allowWhiteSpace is true.
//   UnsignedDouble <- Digit+ | Digit* '.' Digit+
//   Double <- UnsignedDouble | '-' UnsignedDouble
//   DoubleAllowingWhiteSpace <- WhiteSpace* Double WhiteSpace*
// Only dots are allowed as decimals.
//   Because being able to read files from another country without crashes is a lot more important than a detail that most people don't even notice.
//   Automatic nationalization made sense when most applications were written in-house before the internet existed.
bool string_isDouble(const ReadableString& source, bool allowWhiteSpace = true);
// Pre-condition: source must be a valid integer according to string_isInteger. Otherwise unexpected characters are simply ignored.
// Post-condition: Returns the integer representation of source.
// The result is signed, because the input might unexpectedly have a negation sign.
// The result is large, so that one can easily check the range before assigning to a smaller integer type.
int64_t string_toInteger(const ReadableString& source);
// Pre-condition: source must be a valid double according to string_isDouble. Otherwise unexpected characters are simply ignored.
// Post-condition: Returns the double precision floating-point representation of source.
double string_toDouble(const ReadableString& source);

// Loading will try to find a byte order mark and can handle UTF-8 and UTF-16.
//   Failure to find a byte order mark will assume that the file's content is raw Latin-1,
//   because automatic detection would cause random behaviour.
// For portability, carriage return characters are removed,
//   but will be generated again using the default CrLf line encoding of string_save.
// Post-condition:
//   Returns the content of the file referred to be filename.
//   If mustExist is true, then failure to load will throw an exception.
//   If mustExist is false, then failure to load will return an empty string.
// If you want to handle files that are not found in a different way,
//   it is easy to use buffer_load and string_loadFromMemory separatelly.
String string_load(const ReadableString& filename, bool mustExist = true);
// Decode a text file from a buffer, which can be loaded using buffer_load.
String string_loadFromMemory(Buffer fileContent);

// Side-effect: Saves content to filename using the selected character and line encodings.
// Do not add carriage return characters yourself into strings, for these will be added automatically in the CrLf mode.
// The internal String type should only use UTF-32 with single line feeds for breaking lines.
//   This makes text processing algorithms a lot cleaner when a character or line break is always one element.
// UTF-8 with BOM is default by being both compact and capable of storing 21 bits of unicode.
void string_save(const ReadableString& filename, const ReadableString& content,
  CharacterEncoding characterEncoding = CharacterEncoding::BOM_UTF8,
  LineEncoding lineEncoding = LineEncoding::CrLf
);
// Encode the string and keep the raw buffer instead of saving it to a file.
Buffer string_saveToMemory(const ReadableString& content,
  CharacterEncoding characterEncoding = CharacterEncoding::BOM_UTF8,
  LineEncoding lineEncoding = LineEncoding::CrLf
);

// Post-condition: Returns true iff strings a and b are exactly equal.
bool string_match(const ReadableString& a, const ReadableString& b);
// Post-condition: Returns true iff strings a and b are roughly equal using a case insensitive match.
bool string_caseInsensitiveMatch(const ReadableString& a, const ReadableString& b);

// Post-condition: Returns text converted to upper case.
String string_upperCase(const ReadableString &text);
// Post-condition: Returns text converted to lower case.
String string_lowerCase(const ReadableString &text);

// Post-condition: Returns a clone of text without any white-space (space, tab, carriage-return, null terminator, et cetera).
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

// Ensures safely that at least minimumLength characters can he held in the buffer
inline void string_reserve(String& target, int64_t minimumLength) {
	target.reserve(minimumLength);
}

// Append/push one character (to avoid integer to string conversion)
inline void string_appendChar(String& target, DsrChar value) {
	target.appendChar(value);
}

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

}

#endif

