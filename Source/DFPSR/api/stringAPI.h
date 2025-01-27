// zlib open source license
//
// Copyright (c) 2017 to 2025 David Forsgren Piuva
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

#ifndef DFPSR_API_STRING
#define DFPSR_API_STRING

#include <cstdint>
#include <functional>
#include "bufferAPI.h"
#include "../base/SafePointer.h"
#include "../base/DsrTraits.h"
#include "../collection/List.h"

// Define DSR_INTERNAL_ACCESS before any include to get internal access to exposed types
#ifdef DSR_INTERNAL_ACCESS
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

class String;

// Helper type for strings.
struct Impl_CharacterView {
	DsrChar *data = nullptr;
	intptr_t length = 0;
	Impl_CharacterView() {}
	Impl_CharacterView(Handle<DsrChar> characters)
	: data(characters.getUnsafe()), length(characters.getElementCount()) {}
	Impl_CharacterView(const DsrChar *data, intptr_t length)
	: data(const_cast<DsrChar *>(data)), length(length) {
		if (data == nullptr) this->length = 0;
	}
	inline DsrChar *getUnchecked() const {
		return const_cast<DsrChar*>(this->data);
	}
	inline DsrChar operator [] (intptr_t index) const {
		if (index < 0 || index >= this->length) {
			return U'\0';
		} else {
			return this->data[index];
		}
	}
	inline void writeCharacter(intptr_t index, DsrChar character) {
		if (index < 0 || index >= this->length) {
			// TODO: Throw an error without causing bottomless recursion.
		} else {
			this->data[index] = character;
		}
	}
	inline SafePointer<DsrChar> getSafe(const char *name) const {
		return SafePointer<DsrChar>(name, this->getUnchecked(), this->length * sizeof(DsrChar));
	}
};

// Replacing String with a ReadableString reference for input arguments can make passing of U"" literals faster,
//   because String is not allowed to assume anything about how long the literal will be available.
// Unlike String, it cannot be constructed from a "" literal, because it is not allowed to heap allocate new memory
//   for the conversion, only hold existing buffers alive with reference counting when casted from String.
class ReadableString {
IMPL_ACCESS:
	// A reference counted pointer to the buffer to allow passing strings around without having to clone the buffer each time
	// ReadableString only uses it for reference counting but String use it for reallocating
	Handle<DsrChar> characters;
	// Pointing to a subset of the buffer or memory that is not shared.
	Impl_CharacterView view;
	// TODO: Merge the pointer and length into a new View type for unified bound checks. Then remove the writer pointer.
	//SafePointer<const DsrChar> reader;
	//intptr_t length = 0;
public:
	// TODO: Inline the [] operator for faster reading of characters.
	//       Use the padded read internally, because the old version was hard-coded for buffers padded to default alignment.
	// Returning the character by value prevents writing to memory that might be a constant literal or shared with other strings
	inline DsrChar operator[] (intptr_t index) const {
		return this->view[index];
	}
public:
	// Empty string U""
	ReadableString() {}
	// Implicit casting from U"text"
	ReadableString(const DsrChar *content);
	ReadableString(Handle<DsrChar> characters, Impl_CharacterView view)
	: characters(characters), view(view) {}
	// Create from String by sharing the buffer
	ReadableString(const String& source);
	// Destructor
	~ReadableString() {} // Do not override the non-virtual destructor.
};

// A safe and simple string type
//   Can be constructed from ascii literals "", but U"" will preserve unicode characters.
//   Can be used without ReadableString, but ReadableString can be wrapped over U"" literals without allocation
//   UTF-32
//     Endianness is native
//     No combined characters allowed, use precomposed instead, so that the strings can guarantee a fixed character size
class String : public ReadableString {
//IMPL_ACCESS:
	// TODO: Have a single pointer to the data in ReadableString and let the API be responsible for type safety.
	//SafePointer<DsrChar> writer;
public:
	// Constructors
	String();
	String(const char* source);
	String(const DsrChar* source);
	String(const ReadableString& source);
	String(const String& source);
};

// Used as format tags around numbers passed to string_append or string_combine
// New types can implement printing to String by making wrappers from this class
class Printable {
public:
	// The method for appending the printable object into the target string
	virtual String& toStreamIndented(String& target, const ReadableString& indentation) const = 0;
	String& toStream(String& target) const;
	String toStringIndented(const ReadableString& indentation) const;
	String toString() const;
	virtual ~Printable();
};

// Helper functions to resolve ambiguity without constexpr if statements in C++ 14.
String& impl_toStreamIndented_ascii(String& target, const char *value, const ReadableString& indentation);
String& impl_toStreamIndented_utf32(String& target, const char32_t *value, const ReadableString& indentation);
String& impl_toStreamIndented_readable(String& target, const ReadableString &value, const ReadableString& indentation);
String& impl_toStreamIndented_double(String& target, const double &value, const ReadableString& indentation);
String& impl_toStreamIndented_int64(String& target, const int64_t &value, const ReadableString& indentation);
String& impl_toStreamIndented_uint64(String& target, const uint64_t &value, const ReadableString& indentation);

// Resolving ambiguity without access to constexpr in if statements by disabling type safety with unsafeCast.
template <typename T, DSR_ENABLE_IF(
    DSR_UTF32_LITERAL(T)
 || DSR_ASCII_LITERAL(T)
 || DSR_INHERITS_FROM(T, Printable)
 || DSR_SAME_TYPE(T, String)
 || DSR_SAME_TYPE(T, ReadableString)
 || DSR_SAME_TYPE(T, float)
 || DSR_SAME_TYPE(T, double)
 || DSR_SAME_TYPE(T, char)
 || DSR_SAME_TYPE(T, char32_t)
 || DSR_SAME_TYPE(T, bool)
 || DSR_SAME_TYPE(T, short)
 || DSR_SAME_TYPE(T, int)
 || DSR_SAME_TYPE(T, long)
 || DSR_SAME_TYPE(T, long long)
 || DSR_SAME_TYPE(T, unsigned short)
 || DSR_SAME_TYPE(T, unsigned int)
 || DSR_SAME_TYPE(T, unsigned long)
 || DSR_SAME_TYPE(T, unsigned long long)
 || DSR_SAME_TYPE(T, uint8_t)
 || DSR_SAME_TYPE(T, uint16_t)
 || DSR_SAME_TYPE(T, uint32_t)
 || DSR_SAME_TYPE(T, uint64_t)
 || DSR_SAME_TYPE(T, int8_t)
 || DSR_SAME_TYPE(T, int16_t)
 || DSR_SAME_TYPE(T, int32_t)
 || DSR_SAME_TYPE(T, int64_t))>
inline String& string_toStreamIndented(String& target, const T &value, const ReadableString& indentation) {
	if (DSR_UTF32_LITERAL(T)) {
		impl_toStreamIndented_utf32(target, unsafeCast<char32_t*>(value), indentation);
	} else if (DSR_ASCII_LITERAL(T)) {
		impl_toStreamIndented_ascii(target, unsafeCast<char*>(value), indentation);
	} else if (DSR_INHERITS_FROM(T, Printable)) {
		unsafeCast<Printable>(value).toStreamIndented(target, indentation);
	} else if (DSR_SAME_TYPE(T, String)) {
		impl_toStreamIndented_readable(target, unsafeCast<String>(value), indentation);
	} else if (DSR_SAME_TYPE(T, ReadableString)) {
		impl_toStreamIndented_readable(target, unsafeCast<ReadableString>(value), indentation);
	} else if (DSR_SAME_TYPE(T, float)) {
		impl_toStreamIndented_double(target, (double)unsafeCast<float>(value), indentation);
	} else if (DSR_SAME_TYPE(T, double)) {
		impl_toStreamIndented_double(target, unsafeCast<double>(value), indentation);
	} else if (DSR_SAME_TYPE(T, char)) {
		impl_toStreamIndented_readable(target, indentation, U"");
		string_appendChar(target, unsafeCast<char>(value));
	} else if (DSR_SAME_TYPE(T, char32_t)) {
		impl_toStreamIndented_readable(target, indentation, U"");
		string_appendChar(target, unsafeCast<char32_t>(value));
	} else if (DSR_SAME_TYPE(T, bool)) {
		impl_toStreamIndented_utf32(target, unsafeCast<bool>(value) ? U"true" : U"false", indentation);
	} else if (DSR_SAME_TYPE(T, uint8_t)) {
		impl_toStreamIndented_uint64(target, (uint64_t)unsafeCast<uint8_t>(value), indentation);
	} else if (DSR_SAME_TYPE(T, uint16_t)) {
		impl_toStreamIndented_uint64(target, (uint64_t)unsafeCast<uint16_t>(value), indentation);
	} else if (DSR_SAME_TYPE(T, uint32_t)) {
		impl_toStreamIndented_uint64(target, (uint64_t)unsafeCast<uint32_t>(value), indentation);
	} else if (DSR_SAME_TYPE(T, uint64_t)) {
		impl_toStreamIndented_uint64(target, unsafeCast<uint64_t>(value), indentation);
	} else if (DSR_SAME_TYPE(T, int8_t)) {
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<int8_t>(value), indentation);
	} else if (DSR_SAME_TYPE(T, int16_t)) {
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<int16_t>(value), indentation);
	} else if (DSR_SAME_TYPE(T, int32_t)) {
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<int32_t>(value), indentation);
	} else if (DSR_SAME_TYPE(T, int64_t)) {
		impl_toStreamIndented_int64(target, unsafeCast<int64_t>(value), indentation);
	} else if (DSR_SAME_TYPE(T, short)) {
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<short>(value), indentation);
	} else if (DSR_SAME_TYPE(T, int)) {
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<int>(value), indentation);
	} else if (DSR_SAME_TYPE(T, long)) {
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<long>(value), indentation);
	} else if (DSR_SAME_TYPE(T, long long)) {
		static_assert(sizeof(long long) == 8, U"You need to implement integer printing for integers larger than 64 bits, or printing long long will be truncated!");
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<long long>(value), indentation);
	} else if (DSR_SAME_TYPE(T, unsigned short)) {
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<short>(value), indentation);
	} else if (DSR_SAME_TYPE(T, unsigned int)) {
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<int>(value), indentation);
	} else if (DSR_SAME_TYPE(T, unsigned long)) {
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<long>(value), indentation);
	} else if (DSR_SAME_TYPE(T, unsigned long long)) {
		static_assert(sizeof(unsigned long long) == 8, U"You need to implement integer printing for integers larger than 64 bits, or printing unsigned long long will be truncated!");
		impl_toStreamIndented_int64(target, (int64_t)unsafeCast<unsigned long long>(value), indentation);
	}
	return target;
}

template<typename T>
String string_toStringIndented(const T& source, const ReadableString& indentation) {
	String result;
	string_toStreamIndented(result, source, indentation);
	return result;
}

template<typename T>
String string_toString(const T& source) {
	String result;
	string_toStreamIndented(result, source, U"");
	return result;
}


// ---------------- Procedural API ----------------


// Sets the target string's length to zero.
// Because this opens up to appending new text where sub-string may already share the buffer,
//   this operation will reallocate the buffer if shared with other strings.
void string_clear(String& target);
// Post-condition: Returns the length of source.
//   Example: string_length(U"ABC") == 3
intptr_t string_length(const ReadableString& source);
// Post-condition: Returns the base-zero index of source's first occurence of toFind, starting from startIndex. Returns -1 if not found.
//   Example: string_findFirst(U"ABCABCABC", U'A') == 0
//   Example: string_findFirst(U"ABCABCABC", U'B') == 1
//   Example: string_findFirst(U"ABCABCABC", U'C') == 2
//   Example: string_findFirst(U"ABCABCABC", U'D') == -1
intptr_t string_findFirst(const ReadableString& source, DsrChar toFind, intptr_t startIndex = 0);
// Post-condition: Returns the base-zero index of source's last occurence of toFind.  Returns -1 if not found.
//   Example: string_findLast(U"ABCABCABC", U'A') == 6
//   Example: string_findLast(U"ABCABCABC", U'B') == 7
//   Example: string_findLast(U"ABCABCABC", U'C') == 8
//   Example: string_findLast(U"ABCABCABC", U'D') == -1
intptr_t string_findLast(const ReadableString& source, DsrChar toFind);
// Post-condition: Returns a sub-string of source from before the character at inclusiveStart to before the character at exclusiveEnd
//   Example: string_exclusiveRange(U"0123456789", 2, 4) == U"23"
ReadableString string_exclusiveRange(const ReadableString& source, intptr_t inclusiveStart, intptr_t exclusiveEnd);
// Post-condition: Returns a sub-string of source from before the character at inclusiveStart to after the character at inclusiveEnd
//   Example: string_inclusiveRange(U"0123456789", 2, 4) == U"234"
ReadableString string_inclusiveRange(const ReadableString& source, intptr_t inclusiveStart, intptr_t inclusiveEnd);
// Post-condition: Returns a sub-string of source from the start to before the character at exclusiveEnd
//   Example: string_before(U"0123456789", 5) == U"01234"
ReadableString string_before(const ReadableString& source, intptr_t exclusiveEnd);
// Post-condition: Returns a sub-string of source from the start to after the character at inclusiveEnd
//   Example: string_until(U"0123456789", 5) == U"012345"
ReadableString string_until(const ReadableString& source, intptr_t inclusiveEnd);
// Post-condition: Returns a sub-string of source from before the character at inclusiveStart to the end
//   Example: string_from(U"0123456789", 5) == U"56789"
ReadableString string_from(const ReadableString& source, intptr_t inclusiveStart);
// Post-condition: Returns a sub-string of source from after the character at exclusiveStart to the end
//   Example: string_after(U"0123456789", 5) == U"6789"
ReadableString string_after(const ReadableString& source, intptr_t exclusiveStart);

// Split source into a list of strings.
// Post-condition:
//   Returns a list of strings from source by splitting along separator.
//   If removeWhiteSpace is true then surrounding white-space will be removed, otherwise all white-space is kept.
// The separating characters are excluded from the resulting strings.
// The number of strings returned in the list will equal the number of separating characters plus one, so the result may contain empty strings.
// Each string in the list clones content to its own dynamic buffer. Use string_split_callback if you don't need long term storage.
List<String> string_split(const ReadableString& source, DsrChar separator, bool removeWhiteSpace = false);
// Split a string without needing a list to store the result.
// Use string_splitCount on the same source and separator if you need to know the element count in advance.
// Side-effects:
//   Calls action for each sub-string divided by separator in source given as the separatedText argument.
void string_split_callback(std::function<void(ReadableString separatedText)> action, const ReadableString& source, DsrChar separator, bool removeWhiteSpace = false);
// An alternative overload for having a very long lambda at the end.
inline void string_split_callback(const ReadableString& source, DsrChar separator, bool removeWhiteSpace, std::function<void(ReadableString separatedText)> action) {
	string_split_callback(action, source, separator, removeWhiteSpace);
}
// Split source using separator, only to return the number of splits.
// Useful for pre-allocation.
intptr_t string_splitCount(const ReadableString& source, DsrChar separator);

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
// Side-effect: Appends value as a base ten integer at the end of target.
void string_fromUnsigned(String& target, uint64_t value);
// Post-condition: Returns value written as a base ten integer.
inline String string_fromUnsigned(int64_t value) {
	String result; string_fromUnsigned(result, value); return result;
}
// Side-effect: Appends value as a base ten integer at the end of target.
void string_fromSigned(String& target, int64_t value, DsrChar negationCharacter = U'-');
// Post-condition: Returns value written as a base ten integer.
inline String string_fromSigned(int64_t value, DsrChar negationCharacter = U'-') {
	String result; string_fromSigned(result, value, negationCharacter); return result;
}
// Pre-condition: source must be a valid double according to string_isDouble. Otherwise unexpected characters are simply ignored.
// Post-condition: Returns the double precision floating-point representation of source.
double string_toDouble(const ReadableString& source);
// Side-effect: Appends value as a base ten decimal number at the end of target.
void string_fromDouble(String& target, double value, int decimalCount = 6, bool removeTrailingZeroes = true, DsrChar decimalCharacter = U'.', DsrChar negationCharacter = U'-');
// Post-condition: Returns value written as a base ten decimal number.
inline String string_fromDouble(double value, int decimalCount = 6, bool removeTrailingZeroes = true, DsrChar decimalCharacter = U'.', DsrChar negationCharacter = U'-') {
	String result; string_fromDouble(result, value, decimalCount, removeTrailingZeroes, decimalCharacter, negationCharacter); return result;
}
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
// Decode a null terminated string without BOM, by specifying which format it was encoded in.
// Pre-conditions:
//   data does not start with any byte-order-mark (BOM).
//   data must be null terminated with '\0' in whatever format is being used. Otherwise you may have random crashes
// Post-condition:
//   Returns a string decoded from the raw data.
String string_dangerous_decodeFromData(const void* data, CharacterEncoding encoding);

// Side-effect: Saves content to filename using the selected character and line encodings.
// Post-condition: Returns true on success and false on failure.
// Do not add carriage return characters yourself into strings, for these will be added automatically in the CrLf mode.
// The internal String type should only use UTF-32 with single line feeds for breaking lines.
//   This makes text processing algorithms a lot cleaner when a character or line break is always one element.
// UTF-8 with BOM is default by being both compact and capable of storing 21 bits of unicode.
bool string_save(const ReadableString& filename, const ReadableString& content,
  CharacterEncoding characterEncoding = CharacterEncoding::BOM_UTF8,
  LineEncoding lineEncoding = LineEncoding::CrLf
);
// Encode the string and keep the raw buffer instead of saving it to a file.
// Disabling writeByteOrderMark can be done when the result is casted to a native string for platform specific APIs, where a BOM is not allowed.
// Enabling writeNullTerminator should be done when using the result as a pointer, so that the length is known when the buffer does not have padding.
Buffer string_saveToMemory(const ReadableString& content,
  CharacterEncoding characterEncoding = CharacterEncoding::BOM_UTF8,
  LineEncoding lineEncoding = LineEncoding::CrLf,
  bool writeByteOrderMark = true,
  bool writeNullTerminator = false
);

// Post-condition: Returns true iff strings a and b are exactly equal.
bool string_match(const ReadableString& a, const ReadableString& b);
// Post-condition: Returns true iff strings a and b are roughly equal using a case insensitive match.
bool string_caseInsensitiveMatch(const ReadableString& a, const ReadableString& b);
// While string_match should be preferred over == for code readability and consistency with string_caseInsensitiveMatch,
//   the equality operator might be called automatically from template methods when a template type is a string.
inline bool operator==(const ReadableString& a, const ReadableString& b) { return string_match(a, b); }
inline bool operator!=(const ReadableString& a, const ReadableString& b) { return !string_match(a, b); }

// Post-condition: Returns text converted to upper case.
String string_upperCase(const ReadableString &text);
// Post-condition: Returns text converted to lower case.
String string_lowerCase(const ReadableString &text);

// Post-condition: Returns a sub-set of text without surrounding white-space (space, tab and carriage-return).
ReadableString string_removeOuterWhiteSpace(const ReadableString &text);

// Post-condition: Returns rawText wrapped in a quote.
// Special characters are included using escape characters, so that one can quote multiple lines but store it easily.
String string_mangleQuote(const ReadableString &rawText);
// Pre-condition: mangledText must be enclosed in double quotes and special characters must use escape characters (tabs in quotes are okay though).
// Post-condition: Returns mangledText with quotes removed and excape tokens interpreted.
String string_unmangleQuote(const ReadableString& mangledText);

// Post-condition: Returns the number of strings using the same buffer, including itself.
uintptr_t string_getBufferUseCount(const ReadableString& text);

// Ensures safely that at least minimumLength characters can he held in the buffer
void string_reserve(String& target, intptr_t minimumLength);

// Append/push one character (to avoid integer to string conversion)
void string_appendChar(String& target, DsrChar value);

// Append elements
inline void string_append(String& target) {}
template<typename HEAD, typename... TAIL>
inline void string_append(String& target, HEAD head, TAIL&&... tail) {
	string_toStreamIndented(target, head, U"");
	string_append(target, tail...);
}
// Combine a number of strings, characters and numbers
//   If an input type is rejected, create a Printable object to wrap around it
template<typename... ARGS>
inline String string_combine(ARGS&&... args) {
	String result;
	string_append(result, args...);
	return result;
}


// ---------------- Infix syntax ----------------


// Operations
inline String operator+ (const ReadableString& a, const ReadableString& b) { return string_combine(a, b); }
inline String operator+ (const DsrChar* a, const ReadableString& b) { return string_combine(a, b); }
inline String operator+ (const ReadableString& a, const DsrChar* b) { return string_combine(a, b); }
inline String operator+ (const String& a, const String& b) { return string_combine(a, b); }
inline String operator+ (const DsrChar* a, const String& b) { return string_combine(a, b); }
inline String operator+ (const String& a, const DsrChar* b) { return string_combine(a, b); }
inline String operator+ (const String& a, const ReadableString& b) { return string_combine(a, b); }
inline String operator+ (const ReadableString& a, const String& b) { return string_combine(a, b); }


// ---------------- Message handling ----------------


enum class MessageType {
	Error, // Terminate as quickly as possible after saving and informing the user.
	Warning, // Inform the user but let the caller continue.
	StandardPrinting, // Print text to the terminal.
	DebugPrinting // Print debug information to the terminal, if debug mode is active.
};

// Get a reference to the thread-local buffer used for printing messages.
//   Can be combined with string_clear, string_append and string_sendMessage to send long messages in a thread-safe way.
//   Clear, fill and send.
String &string_getPrintBuffer();

// Send a message
void string_sendMessage(const ReadableString &message, MessageType type);
// Send a message directly to the default message handler, ignoring string_assignMessageHandler.
void string_sendMessage_default(const ReadableString &message, MessageType type);

// Get a message
// Pre-condition:
//   The action function must throw an exception or terminate the program when given an error, otherwise string_sendMessage will throw an exception about failing to do so.
//   Do not call string_sendMessage directly or indirectly from within action, use string_sendMessage_default instead to avoid infinite recursion.
// Terminating the program as soon as possible is ideal, but one might want to save a backup or show what went wrong in a graphical interface before terminating.
// Do not throw and catch errors as if they were warnings, because throwing and catching creates a partial transaction, potentially violating type invariants.
//   Better to use warnings and let the sender of the warning figure out how to abort the action safely.
void string_assignMessageHandler(std::function<void(const ReadableString &message, MessageType type)> action);

// Undo string_assignMessageHandler, so that any messages will be handled the default way again.
void string_unassignMessageHandler();

// Throw an error, which must terminate the application or throw an error
template<typename... ARGS>
void throwError(ARGS... args) {
	String *target = &(string_getPrintBuffer());
	string_clear(*target);
	string_append(*target, args...);
	string_sendMessage(*target, MessageType::Error);
}

// Send a warning, which might throw an exception, terminate the application or anything else that the application requests using string_handleMessages
template<typename... ARGS>
void sendWarning(ARGS... args) {
	String *target = &(string_getPrintBuffer());
	string_clear(*target);
	string_append(*target, args...);
	string_sendMessage(*target, MessageType::Warning);
}

// Print information to the terminal or something else listening for messages using string_handleMessages
template<typename... ARGS>
void printText(ARGS... args) {
	String *target = &(string_getPrintBuffer());
	string_clear(*target);
	string_append(*target, args...);
	string_sendMessage(*target, MessageType::StandardPrinting);
}

// Debug messages are automatically disabled in release mode, so that you don't have to worry about accidentally releasing a program with poor performance from constantly printing to the terminal
//   Useful for selectively printing the most important information accumulated over time
//   Less useful for profiling, because the debug mode is slower than the release mode
#ifdef NDEBUG
	// Supress debugText in release mode
	template<typename... ARGS>
	void debugText(ARGS... args) {}
#else
	// Print debugText in debug mode
	template<typename... ARGS>
	void debugText(ARGS... args) {
	String *target = &(string_getPrintBuffer());
		string_clear(*target);
		string_append(*target, args...);
		string_sendMessage(*target, MessageType::DebugPrinting);
	}
#endif

// Used to generate fixed size ascii strings, which is useful when heap allocations are not possible
//   or you need a safe format until you know which encoding a system call needs to support Unicode.
template <intptr_t SIZE>
struct FixedAscii {
	char characters[SIZE];
	// Create a fixed size ascii string from a null terminated ascii string.
	// Crops if text is too long.
	FixedAscii(const char *text) {
		bool terminated = false;
		for (intptr_t i = 0; i < SIZE - 1; i++) {
			char c = text[i];
			if (c == '\0') {
				terminated = true;
			}
			if (terminated) {
				this->characters[i] = '\0';
			} else if (c > 127) {
				this->characters[i] = '?';
			} else {
				this->characters[i] = c;
			}
		}
		this->characters[SIZE - 1] = '\0';
	}
	FixedAscii(const ReadableString &text) {
		bool terminated = false;
		for (intptr_t i = 0; i < SIZE - 1; i++) {
			char c = text[i];
			if (c == '\0') {
				terminated = true;
			}
			if (terminated) {
				this->characters[i] = '\0';
			} else if (c > 127) {
				this->characters[i] = '?';
			} else {
				this->characters[i] = c;
			}
		}
		this->characters[SIZE - 1] = '\0';
	}
	operator const char *() const {
		return characters;
	}
};

}

#endif

