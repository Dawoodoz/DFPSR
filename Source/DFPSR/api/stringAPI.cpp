// zlib open source license
//
// Copyright (c) 2017 to 2026 David Forsgren Piuva
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
#define DSR_INTERNAL_ACCESS

#include <iostream>
#include <sstream>
#include <fstream>
#include <streambuf>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <cmath>
#include "stringAPI.h"
#include "../api/fileAPI.h"
#include "../settings.h"

using namespace dsr;

// The print buffer keeps its buffer size from previous printing to avoid reallocating memory every time something is printed.
//   It is stored separatelly for each calling thread to avoid conflicts.
static thread_local String printBuffer;
String &dsr::string_getPrintBuffer() {
	return printBuffer;
}

static void atomic_append_ascii(String &target, const char* source);
static void atomic_append_readable(String &target, const ReadableString& source);
static void atomic_append_utf32(String &target, const DsrChar* source);

static intptr_t strlen_utf32(const DsrChar *content) {
	intptr_t length = 0;
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

ReadableString::ReadableString(const DsrChar *content)
: view(content, strlen_utf32(content)) {}

String::String() {}
#ifndef BAN_IMPLICIT_ASCII_CONVERSION
	String::String(const char* source) { atomic_append_ascii(*this, source); }
#endif
String::String(const DsrChar* source) { atomic_append_utf32(*this, source); }

String dsr::string_fromAscii(const char *text) {
	String result;
	atomic_append_ascii(result, text);
	return result;
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

Printable::~Printable() {}

/*
Code generator used to create character transforming functions from arbitrary reference functions.
Paste the result into functions that provide character, odd and even before the returning if statement begins.

static void generateCharacterRange(String &result, DsrChar firstIn, DsrChar lastIn, int64_t stride, int64_t offset) {
	DsrChar firstOut = firstIn + offset;
	DsrChar lastOut = lastIn + offset;
	if (string_length(result) == 0) {
		string_append(result, U"	");
	} else {
		string_append(result, U"	} else ");
	}
	if (firstIn == lastIn) {
		string_append(result, U"if (character == U'", firstIn, "') { // ", firstIn, U" (", (uint32_t)firstIn, U")\n");
		string_append(result, U"		return U'", firstOut, "'; // ", firstOut, U" (", (uint32_t)firstOut, U")\n");
	} else {
		string_append(result, U"if (U'", firstIn, "' <= character && character <= U'", lastIn, U"'");
		if (stride == 2) {
			if (firstIn & DsrChar(1)) {
				// Odd interval
				string_append(result, U" && odd");
			} else {
				// Even interval
				string_append(result, U" && even");
			}
		} else if (stride != 1) {
			throwError(U"Unsupported stride ", stride, U"!\n");
		}
		string_append(result, U") { // ", firstIn, U" (", (uint32_t)firstIn, U") to ", lastIn, U" (", (uint32_t)lastIn, U")\n");
		if (firstOut > firstIn) {
			string_append(result, U"		return character + ", offset, ";");
		} else if (firstOut < firstIn) {
			string_append(result, U"		return character - ", -offset, ";");
		}
		string_append(result, U"// ", firstOut, U" (", (uint32_t)firstOut, U") to ", lastOut, U" (", (uint32_t)lastOut, U")\n");
	}
}
// Pre-condition: The transform function must change at least one character.
static String generateCharacterMapping(std::function<DsrChar(const DsrChar character)> transform, DsrChar first, DsrChar last) {
	String result;
	int64_t rangeStart = -1;
	int64_t rangeEnd = -1;
	int64_t lastOffset = -1;
	int64_t currentStride = -1;
	for (int64_t c = first; c <= last; c++) {
		int64_t t = transform(c);
		if (c != t) {
			int64_t offset = int64_t(t) - int64_t(c);
			int64_t step = c - rangeEnd;
			// Check if we should break apart the previous range.
			if ((currentStride != -1 && step != currentStride)
			 || step > 2
			 || (lastOffset != -1 && offset != lastOffset)) {
				if (rangeStart != -1) {
					generateCharacterRange(result, rangeStart, rangeEnd, currentStride, lastOffset);
				}
				rangeStart = c;
				rangeEnd = c;
				lastOffset = offset;
				currentStride = -1;
			} else {
				rangeEnd = c;
				lastOffset = offset;
				currentStride = step;
			}
		}
	}
	// Generate the last range, while assuming that we have at least one character to modify.
	if (rangeStart != -1) {
		generateCharacterRange(result, rangeStart, rangeEnd, currentStride, lastOffset);
	}
	string_append(result, U"	} else {\n");
	string_append(result, U"		return character;\n");
	string_append(result, U"	}\n");
	return result;
}
*/

DsrChar dsr::character_upperCase(DsrChar character) {
	if (character < 256) {
		if (U'a' <= character && character <= U'z') { // a (97) to z (122)
			return character - 32; // A (65) to Z (90)
		} else if (character == U'ß') { // ß (223)
			return U'ẞ'; // ẞ (7838)
		} else if (U'à' <= character && character <= U'ö') { // à (224) to ö (246)
			return character - 32; // À (192) to Ö (214)
		} else if (U'ø' <= character && character <= U'þ') { // ø (248) to þ (254)
			return character - 32; // Ø (216) to Þ (222)
		} else if (character == U'ÿ') { // ÿ (255)
			return U'Ÿ'; // Ÿ (376)
		} else {
			return character;
		}
	} else {
		bool odd = character & DsrChar(1);
		bool even = !odd;
		if (U'ā' <= character && character <= U'ķ' && odd) { // ā (257) to ķ (311)
			return character - 1; // Ā (256) to Ķ (310)
		} else if (U'ĺ' <= character && character <= U'ň' && even) { // ĺ (314) to ň (328)
			return character - 1; // Ĺ (313) to Ň (327)
		} else if (U'ŋ' <= character && character <= U'ŷ' && odd) { // ŋ (331) to ŷ (375)
			return character - 1; // Ŋ (330) toŶ (374)
		} else if (U'ź' <= character && character <= U'ž' && even) { // ź (378) to ž (382)
			return character - 1; // Ź (377) to Ž (381)
		} else if (character == U'ƀ') { // ƀ (384)
			return U'Ƀ'; // Ƀ (579)
		} else if (character == U'ƃ') { // ƃ (387)
			return U'Ƃ'; // Ƃ (386)
		} else if (character == U'ƅ') { // ƅ (389)
			return U'Ƅ'; // Ƅ (388)
		} else if (character == U'ƈ') { // ƈ (392)
			return U'Ƈ'; // Ƈ (391)
		} else if (character == U'ƌ') { // ƌ (396)
			return U'Ƌ'; // Ƌ (395)
		} else if (character == U'ƒ') { // ƒ (402)
			return U'Ƒ'; // Ƒ (401)
		} else if (character == U'ƙ') { // ƙ (409)
			return U'Ƙ'; // Ƙ (408)
		} else if (character == U'ƚ') { // ƚ (410)
			return U'Ƚ'; // Ƚ (573)
		} else if (character == U'ƞ') { // ƞ (414)
			return U'Ƞ'; // Ƞ (544)
		} else if (character == U'ơ') { // ơ (417)
			return U'Ơ'; // Ơ (416)
		} else if (character == U'ƣ') { // ƣ (419)
			return U'Ƣ'; // Ƣ (418)
		} else if (character == U'ƥ') { // ƥ (421)
			return U'Ƥ'; // Ƥ (420)
		} else if (character == U'ƨ') { // ƨ (424)
			return U'Ƨ'; // Ƨ (423)
		} else if (character == U'Ʃ') { // Ʃ (425)
			return U'ʃ'; // ʃ (643)
		} else if (character == U'ƭ') { // ƭ (429)
			return U'Ƭ'; // Ƭ (428)
		} else if (character == U'ư') { // ư (432)
			return U'Ư'; // Ư (431)
		} else if (character == U'ƴ') { // ƴ (436)
			return U'Ƴ'; // Ƴ (435)
		} else if (character == U'ƶ') { // ƶ (438)
			return U'Ƶ'; // Ƶ (437)
		} else if (character == U'ƹ') { // ƹ (441)
			return U'Ƹ'; // Ƹ (440)
		} else if (character == U'ƽ') { // ƽ (445)
			return U'Ƽ'; // Ƽ (444)
		} else if (character == U'ƿ') { // ƿ (447)
			return U'Ƿ'; // Ƿ (503)
		} else if (character == U'ǅ') { // ǅ (453)
			return U'Ǆ'; // Ǆ (452)
		} else if (character == U'ǆ') { // ǆ (454)
			return U'Ǆ'; // Ǆ (452)
		} else if (character == U'ǈ') { // ǈ (456)
			return U'Ǉ'; // Ǉ (455)
		} else if (character == U'ǉ') { // ǉ (457)
			return U'Ǉ'; // Ǉ (455)
		} else if (character == U'ǋ') { // ǋ (459)
			return U'Ǌ'; // Ǌ (458)
		} else if (character == U'ǌ') { // ǌ (460)
			return U'Ǌ'; // Ǌ (458)
		} else if (U'ǎ' <= character && character <= U'ǜ' && even) { // ǎ (462) to ǜ (476)
			return character - 1; // Ǎ (461) to Ǜ (475)
		} else if (U'ǟ' <= character && character <= U'ǯ' && odd) { // ǟ (479) to ǯ (495)
			return character - 1; // Ǟ (478) to Ǯ (494)
		} else if (character == U'ǲ') { // ǲ (498)
			return U'Ǳ'; // Ǳ (497)
		} else if (character == U'ǳ') { // ǳ (499)
			return U'Ǳ'; // Ǳ (497)
		} else if (character == U'ǵ') { // ǵ (501)
			return U'Ǵ'; // Ǵ (500)
		} else if (U'ǹ' <= character && character <= U'ȟ' && odd) { // ǹ (505) to ȟ (543)
			return character - 1;// Ǹ (504) to Ȟ (542)
		} else if (U'ȣ' <= character && character <= U'ȳ' && odd) { // ȣ (547) to ȳ (563)
			return character - 1;// Ȣ (546) to Ȳ (562)
		} else if (character == U'ȼ') { // ȼ (572)
			return U'Ȼ'; // Ȼ (571)
		} else if (U'ȿ' <= character && character <= U'ɀ') { // ȿ (575) to ɀ (576)
			return character + 10815;// Ȿ (11390) to Ɀ (11391)
		} else if (character == U'ɂ') { // ɂ (578)
			return U'Ɂ'; // Ɂ (577)
		} else if (U'ɇ' <= character && character <= U'ɏ' && odd) { // ɇ (583) to ɏ (591)
			return character - 1;// Ɇ (582) to Ɏ (590)
		} else if (character == U'ɐ') { // ɐ (592)
			return U'Ɐ'; // Ɐ (11375)
		} else if (character == U'ɑ') { // ɑ (593)
			return U'Ɑ'; // Ɑ (11373)
		} else if (character == U'ɒ') { // ɒ (594)
			return U'Ɒ'; // Ɒ (11376)
		} else if (character == U'ɓ') { // ɓ (595)
			return U'Ɓ'; // Ɓ (385)
		} else if (character == U'ɔ') { // ɔ (596)
			return U'Ɔ'; // Ɔ (390)
		} else if (U'ɖ' <= character && character <= U'ɗ') { // ɖ (598) to ɗ (599)
			return character - 205;// Ɖ (393) to Ɗ (394)
		} else if (U'ɘ' <= character && character <= U'ə') { // ɘ (600) to ə (601)
			return character - 202;// Ǝ (398) to Ə (399)
		} else if (character == U'ɛ') { // ɛ (603)
			return U'Ɛ'; // Ɛ (400)
		} else if (character == U'ɠ') { // ɠ (608)
			return U'Ɠ'; // Ɠ (403)
		} else if (character == U'ɣ') { // ɣ (611)
			return U'Ɣ'; // Ɣ (404)
		} else if (character == U'ɥ') { // ɥ (613)
			return U'Ɥ'; // Ɥ (42893)
		} else if (character == U'ɨ') { // ɨ (616)
			return U'Ɨ'; // Ɨ (407)
		} else if (character == U'ɩ') { // ɩ (617)
			return U'Ɩ'; // Ɩ (406)
		} else if (character == U'ɪ') { // ɪ (618)
			return U'Ɪ'; // Ɪ (42926)
		} else if (character == U'ɯ') { // ɯ (623)
			return U'Ɯ'; // Ɯ (412)
		} else if (character == U'ɱ') { // ɱ (625)
			return U'Ɱ'; // Ɱ (11374)
		} else if (character == U'ɲ') { // ɲ (626)
			return U'Ɲ'; // Ɲ (413)
		} else if (character == U'ɵ') { // ɵ (629)
			return U'Ɵ'; // Ɵ (415)
		} else if (character == U'ɽ') { // ɽ (637)
			return U'Ɽ'; // Ɽ (11364)
		} else if (character == U'ʀ') { // ʀ (640)
			return U'Ʀ'; // Ʀ (422)
		} else if (character == U'ʈ') { // ʈ (648)
			return U'Ʈ'; // Ʈ (430)
		} else if (character == U'ʉ') { // ʉ (649)
			return U'Ʉ'; // Ʉ (580)
		} else if (U'ʊ' <= character && character <= U'ʋ') { // ʊ (650) to ʋ (651)
			return character - 217;// Ʊ (433) to Ʋ (434)
		} else if (character == U'ʌ') { // ʌ (652)
			return U'Ʌ'; // Ʌ (581)
		} else if (character == U'ʒ') { // ʒ (658)
			return U'Ʒ'; // Ʒ (439)
		} else if (character == U'ʔ') { // ʔ (660)
			return U'ˀ'; // ˀ (704)
		} else if (character == U'ά') { // ά (940)
			return U'Ά'; // Ά (902)
		} else if (U'έ' <= character && character <= U'ί') { // έ (941) to ί (943)
			return character - 37;// Έ (904) to Ί (906)
		} else if (U'α' <= character && character <= U'ρ') { // α (945) to ρ (961)
			return character - 32;// Α (913) to Ρ (929)
		} else if (U'σ' <= character && character <= U'ϋ') { // σ (963) to ϋ (971)
			return character - 32;// Σ (931) to Ϋ (939)
		} else if (character == U'ό') { // ό (972)
			return U'Ό'; // Ό (908)
		} else if (U'ύ' <= character && character <= U'ώ') { // ύ (973) to ώ (974)
			return character - 63;// Ύ (910) to Ώ (911)
		} else if (U'ϣ' <= character && character <= U'ϯ' && odd) { // ϣ (995) to ϯ (1007)
			return character - 1;// Ϣ (994) to Ϯ (1006)
		} else if (U'а' <= character && character <= U'я') { // а (1072) to я (1103)
			return character - 32;// А (1040) to Я (1071)
		} else if (U'ё' <= character && character <= U'ќ') { // ё (1105) to ќ (1116)
			return character - 80;// Ё (1025) to Ќ (1036)
		} else if (U'ў' <= character && character <= U'џ') { // ў (1118) to џ (1119)
			return character - 80;// Ў (1038) to Џ (1039)
		} else if (U'ѡ' <= character && character <= U'ҁ' && odd) { // ѡ (1121) to ҁ (1153)
			return character - 1;// Ѡ (1120) to Ҁ (1152)
		} else if (U'ґ' <= character && character <= U'ҿ' && odd) { // ґ (1169) to ҿ (1215)
			return character - 1;// Ґ (1168) to Ҿ (1214)
		} else if (U'ӂ' <= character && character <= U'ӄ' && even) { // ӂ (1218) to ӄ (1220)
			return character - 1;// Ӂ (1217) to Ӄ (1219)
		} else if (character == U'ӈ') { // ӈ (1224)
			return U'Ӈ'; // Ӈ (1223)
		} else if (character == U'ӌ') { // ӌ (1228)
			return U'Ӌ'; // Ӌ (1227)
		} else if (U'ӑ' <= character && character <= U'ӫ' && odd) { // ӑ (1233) to ӫ (1259)
			return character - 1;// Ӑ (1232) to Ӫ (1258)
		} else if (U'ӯ' <= character && character <= U'ӵ' && odd) { // ӯ (1263) to ӵ (1269)
			return character - 1;// Ӯ (1262) to Ӵ (1268)
		} else if (character == U'ӹ') { // ӹ (1273)
			return U'Ӹ'; // Ӹ (1272)
		} else if (U'ա' <= character && character <= U'ֆ') { // ա (1377) to ֆ (1414)
			return character - 48;// Ա (1329) to Ֆ (1366)
		} else if (U'ა' <= character && character <= U'ჵ') { // ა (4304) to ჵ (4341)
			return character - 48;// Ⴀ (4256) to Ⴥ (4293)
		} else if (U'ḁ' <= character && character <= U'ẕ' && odd) { // ḁ (7681) to ẕ (7829)
			return character - 1;// Ḁ (7680) to Ẕ (7828)
		} else if (U'ạ' <= character && character <= U'ỹ' && odd) { // ạ (7841) to ỹ (7929)
			return character - 1;// Ạ (7840) to Ỹ (7928)
		} else if (U'ἀ' <= character && character <= U'ἇ') { // ἀ (7936) to ἇ (7943)
			return character + 8;// Ἀ (7944) to Ἇ (7951)
		} else if (U'ἐ' <= character && character <= U'ἕ') { // ἐ (7952) to ἕ (7957)
			return character + 8;// Ἐ (7960) to Ἕ (7965)
		} else if (U'ἠ' <= character && character <= U'ἧ') { // ἠ (7968) to ἧ (7975)
			return character + 8;// Ἠ (7976) to Ἧ (7983)
		} else if (U'ἰ' <= character && character <= U'ἷ') { // ἰ (7984) to ἷ (7991)
			return character + 8;// Ἰ (7992) to Ἷ (7999)
		} else if (U'ὀ' <= character && character <= U'ὅ') { // ὀ (8000) to ὅ (8005)
			return character + 8;// Ὀ (8008) to Ὅ (8013)
		} else if (U'ὑ' <= character && character <= U'ὗ' && odd) { // ὑ (8017) to ὗ (8023)
			return character + 8;// Ὑ (8025) to Ὗ (8031)
		} else if (U'ὠ' <= character && character <= U'ὧ') { // ὠ (8032) to ὧ (8039)
			return character + 8;// Ὠ (8040) to Ὧ (8047)
		} else if (U'ᾀ' <= character && character <= U'ᾇ') { // ᾀ (8064) to ᾇ (8071)
			return character + 8;// ᾈ (8072) to ᾏ (8079)
		} else if (U'ᾐ' <= character && character <= U'ᾗ') { // ᾐ (8080) to ᾗ (8087)
			return character + 8;// ᾘ (8088) to ᾟ (8095)
		} else if (U'ᾠ' <= character && character <= U'ᾧ') { // ᾠ (8096) to ᾧ (8103)
			return character + 8;// ᾨ (8104) to ᾯ (8111)
		} else if (U'ᾰ' <= character && character <= U'ᾱ') { // ᾰ (8112) to ᾱ (8113)
			return character + 8;// Ᾰ (8120) to Ᾱ (8121)
		} else if (U'ῐ' <= character && character <= U'ῑ') { // ῐ (8144) to ῑ (8145)
			return character + 8;// Ῐ (8152) to Ῑ (8153)
		} else if (U'ῠ' <= character && character <= U'ῡ') { // ῠ (8160) to ῡ (8161)
			return character + 8;// Ῠ (8168) to Ῡ (8169)
		} else if (U'ⓐ' <= character && character <= U'ⓩ') { // ⓐ (9424) to ⓩ (9449)
			return character - 26;// Ⓐ (9398) to Ⓩ (9423)
		} else if (U'ａ' <= character && character <= U'ｚ') { // ａ (65345) to ｚ (65370)
			return character - 32;// Ａ (65313) to Ｚ (65338)
		} else {
			return character;
		}
	}
}

DsrChar dsr::character_lowerCase(DsrChar character) {
	if (character < 256) {
		if (U'A' <= character && character <= U'Z') { // A (65) to Z (90)
			return character + 32; // a (97) to z (122)
		} else if (U'À' <= character && character <= U'Ö') { // À (192) to Ö (214)
			return character + 32; // à (224) to ö (246)
		} else if (U'Ø' <= character && character <= U'Þ') { // Ø (216) to Þ (222)
			return character + 32; // ø (248) to þ (254)
		} else {
			return character;
		}
	} else {
		bool odd = character & DsrChar(1);
		bool even = !odd;
		if (U'Ā' <= character && character <= U'Ķ' && even) { // Ā (256) to Ķ (310)
			return character + 1; // ā (257) to ķ (311)
		} else if (U'Ĺ' <= character && character <= U'Ň' && odd) { // Ĺ (313) to Ň (327)
			return character + 1; // ĺ (314) to ň (328)
		} else if (U'Ŋ' <= character && character <= U'Ŷ' && even) { // Ŋ (330) to Ŷ (374)
			return character + 1; // ŋ (331) to ŷ (375)
		} else if (character == U'Ÿ') { // Ÿ (376)
			return U'ÿ'; // ÿ (255)
		} else if (character == U'Ź') { // Ź (377)
			return U'ź'; // ź (378)
		} else if (character == U'Ż') { // Ż (379)
			return U'ż'; // ż (380)
		} else if (character == U'Ž') { // Ž (381)
			return U'ž'; // ž (382)
		} else if (character == U'Ɓ') { // Ɓ (385)
			return U'ɓ'; // ɓ (595)
		} else if (character == U'Ƃ') { // Ƃ (386)
			return U'ƃ'; // ƃ (387)
		} else if (character == U'Ƅ') { // Ƅ (388)
			return U'ƅ'; // ƅ (389)
		} else if (character == U'Ɔ') { // Ɔ (390)
			return U'ɔ'; // ɔ (596)
		} else if (character == U'Ƈ') { // Ƈ (391)
			return U'ƈ'; // ƈ (392)
		} else if (character == U'Ɖ') { // Ɖ (393)
			return U'ɖ'; // ɖ (598)
		} else if (character == U'Ɗ') { // Ɗ (394)
			return U'ɗ'; // ɗ (599)
		} else if (character == U'Ƌ') { // Ƌ (395)
			return U'ƌ'; // ƌ (396)
		} else if (character == U'Ǝ') { // Ǝ (398)
			return U'ɘ'; // ɘ (600)
		} else if (character == U'Ə') { // Ə (399)
			return U'ə'; // ə (601)
		} else if (character == U'Ɛ') { // Ɛ (400)
			return U'ɛ'; // ɛ (603)
		} else if (character == U'Ƒ') { // Ƒ (401)
			return U'ƒ'; // ƒ (402)
		} else if (character == U'Ɠ') { // Ɠ (403)
			return U'ɠ'; // ɠ (608)
		} else if (character == U'Ɣ') { // Ɣ (404)
			return U'ɣ'; // ɣ (611)
		} else if (character == U'Ɩ') { // Ɩ (406)
			return U'ɩ'; // ɩ (617)
		} else if (character == U'Ɨ') { // Ɨ (407)
			return U'ɨ'; // ɨ (616)
		} else if (character == U'Ƙ') { // Ƙ (408)
			return U'ƙ'; // ƙ (409)
		} else if (character == U'Ɯ') { // Ɯ (412)
			return U'ɯ'; // ɯ (623)
		} else if (character == U'Ɲ') { // Ɲ (413)
			return U'ɲ'; // ɲ (626)
		} else if (character == U'Ɵ') { // Ɵ (415)
			return U'ɵ'; // ɵ (629)
		} else if (character == U'Ơ') { // Ơ (416)
			return U'ơ'; // ơ (417)
		} else if (character == U'Ƣ') { // Ƣ (418)
			return U'ƣ'; // ƣ (419)
		} else if (character == U'Ƥ') { // Ƥ (420)
			return U'ƥ'; // ƥ (421)
		} else if (character == U'Ʀ') { // Ʀ (422)
			return U'ʀ'; // ʀ (640)
		} else if (character == U'Ƨ') { // Ƨ (423)
			return U'ƨ'; // ƨ (424)
		} else if (character == U'Ƭ') { // Ƭ (428)
			return U'ƭ'; // ƭ (429)
		} else if (character == U'Ʈ') { // Ʈ (430)
			return U'ʈ'; // ʈ (648)
		} else if (character == U'Ư') { // Ư (431)
			return U'ư'; // ư (432)
		} else if (character == U'Ʊ') { // Ʊ (433)
			return U'ʊ'; // ʊ (650)
		} else if (character == U'Ʋ') { // Ʋ (434)
			return U'ʋ'; // ʋ (651)
		} else if (character == U'Ƴ') { // Ƴ (435)
			return U'ƴ'; // ƴ (436)
		} else if (character == U'Ƶ') { // Ƶ (437)
			return U'ƶ'; // ƶ (438)
		} else if (character == U'Ʒ') { // Ʒ (439)
			return U'ʒ'; // ʒ (658)
		} else if (character == U'Ƹ') { // Ƹ (440)
			return U'ƹ'; // ƹ (441)
		} else if (character == U'Ƽ') { // Ƽ (444)
			return U'ƽ'; // ƽ (445)
		} else if (character == U'Ǆ') { // Ǆ (452)
			return U'ǆ'; // ǆ (454)
		} else if (character == U'ǅ') { // ǅ (453)
			return U'ǆ'; // ǆ (454)
		} else if (character == U'Ǉ') { // Ǉ (455)
			return U'ǉ'; // ǉ (457)
		} else if (character == U'ǈ') { // ǈ (456)
			return U'ǉ'; // ǉ (457)
		} else if (character == U'Ǌ') { // Ǌ (458)
			return U'ǌ'; // ǌ (460)
		} else if (U'ǋ' <= character && character <= U'Ǜ' && odd) { // ǋ (459) to Ǜ (475)
			return character + 1; // ǌ (460) to ǜ (476)
		} else if (U'Ǟ' <= character && character <= U'Ǯ' && even) { // Ǟ (478) to Ǯ (494)
			return character + 1; // ǟ (479) to ǯ (495)
		} else if (character == U'Ǳ') { // Ǳ (497)
			return U'ǳ'; // ǳ (499)
		} else if (character == U'ǲ') { // ǲ (498)
			return U'ǳ'; // ǳ (499)
		} else if (character == U'Ǵ') { // Ǵ (500)
			return U'ǵ'; // ǵ (501)
		} else if (character == U'Ƿ') { // Ƿ (503)
			return U'ƿ'; // ƿ (447)
		} else if (U'Ǹ' <= character && character <= U'Ȟ' && even) { // Ǹ (504) to Ȟ (542)
			return character + 1; // ǹ (505) to ȟ (543)
		} else if (character == U'Ƞ') { // Ƞ (544)
			return U'ƞ'; // ƞ (414)
		} else if (U'Ȣ' <= character && character <= U'Ȳ' && even) { // Ȣ (546) to Ȳ (562)
			return character + 1; // ȣ (547) to ȳ (563)
		} else if (character == U'Ȼ') { // Ȼ (571)
			return U'ȼ'; // ȼ (572)
		} else if (character == U'Ƚ') { // Ƚ (573)
			return U'ƚ'; // ƚ (410)
		} else if (character == U'Ɂ') { // Ɂ (577)
			return U'ɂ'; // ɂ (578)
		} else if (character == U'Ƀ') { // Ƀ (579)
			return U'ƀ'; // ƀ (384)
		} else if (character == U'Ʉ') { // Ʉ (580)
			return U'ʉ'; // ʉ (649)
		} else if (character == U'Ʌ') { // Ʌ (581)
			return U'ʌ'; // ʌ (652)
		} else if (U'Ɇ' <= character && character <= U'Ɏ' && even) { // Ɇ (582) to Ɏ (590)
			return character + 1;// ɇ (583) to ɏ (591)
		} else if (character == U'ʃ') { // ʃ (643)
			return U'Ʃ'; // Ʃ (425)
		} else if (character == U'ˀ') { // ˀ (704)
			return U'ʔ'; // ʔ (660)
		} else if (character == U'Ά') { // Ά (902)
			return U'ά'; // ά (940)
		} else if (U'Έ' <= character && character <= U'Ί') { // Έ (904) to Ί (906)
			return character + 37;// έ (941) to ί (943)
		} else if (character == U'Ό') { // Ό (908)
			return U'ό'; // ό (972)
		} else if (U'Ύ' <= character && character <= U'Ώ') { // Ύ (910) to Ώ (911)
			return character + 63;// ύ (973) to ώ (974)
		} else if (U'Α' <= character && character <= U'Ρ') { // Α (913) to Ρ (929)
			return character + 32;// α (945) to ρ (961)
		} else if (U'Σ' <= character && character <= U'Ϋ') { // Σ (931) to Ϋ (939)
			return character + 32;// σ (963) to ϋ (971)
		} else if (U'Ϣ' <= character && character <= U'Ϯ' && even) { // Ϣ (994) to Ϯ (1006)
			return character + 1;// ϣ (995) to ϯ (1007)
		} else if (U'Ё' <= character && character <= U'Ќ') { // Ё (1025) to Ќ (1036)
			return character + 80;// ё (1105) to ќ (1116)
		} else if (U'Ў' <= character && character <= U'Џ') { // Ў (1038) to Џ (1039)
			return character + 80;// ў (1118) to џ (1119)
		} else if (U'А' <= character && character <= U'Я') { // А (1040) to Я (1071)
			return character + 32;// а (1072) to я (1103)
		} else if (U'Ѡ' <= character && character <= U'Ҁ' && even) { // Ѡ (1120) to Ҁ (1152)
			return character + 1;// ѡ (1121) to ҁ (1153)
		} else if (U'Ґ' <= character && character <= U'Ҿ' && even) { // Ґ (1168) to Ҿ (1214)
			return character + 1;// ґ (1169) to ҿ (1215)
		} else if (U'Ӂ' <= character && character <= U'Ӄ' && odd) { // Ӂ (1217) to Ӄ (1219)
			return character + 1;// ӂ (1218) to ӄ (1220)
		} else if (character == U'Ӈ') { // Ӈ (1223)
			return U'ӈ'; // ӈ (1224)
		} else if (character == U'Ӌ') { // Ӌ (1227)
			return U'ӌ'; // ӌ (1228)
		} else if (U'Ӑ' <= character && character <= U'Ӫ' && even) { // Ӑ (1232) to Ӫ (1258)
			return character + 1;// ӑ (1233) to ӫ (1259)
		} else if (U'Ӯ' <= character && character <= U'Ӵ' && even) { // Ӯ (1262) to Ӵ (1268)
			return character + 1;// ӯ (1263) to ӵ (1269)
		} else if (character == U'Ӹ') { // Ӹ (1272)
			return U'ӹ'; // ӹ (1273)
		} else if (U'Ա' <= character && character <= U'Ֆ') { // Ա (1329) to Ֆ (1366)
			return character + 48;// ա (1377) to ֆ (1414)
		} else if (U'Ⴀ' <= character && character <= U'Ⴥ') { // Ⴀ (4256) to Ⴥ (4293)
			return character + 48;// ა (4304) to ჵ (4341)
		} else if (U'Ḁ' <= character && character <= U'Ẕ' && even) { // Ḁ (7680) to Ẕ (7828)
			return character + 1;// ḁ (7681) to ẕ (7829)
		} else if (character == U'ẞ') { // ẞ (7838)
			return U'ß'; // ß (223)
		} else if (U'Ạ' <= character && character <= U'Ỹ' && even) { // Ạ (7840) to Ỹ (7928)
			return character + 1;// ạ (7841) to ỹ (7929)
		} else if (U'Ἀ' <= character && character <= U'Ἇ') { // Ἀ (7944) to Ἇ (7951)
			return character - 8;// ἀ (7936) to ἇ (7943)
		} else if (U'Ἐ' <= character && character <= U'Ἕ') { // Ἐ (7960) to Ἕ (7965)
			return character - 8;// ἐ (7952) to ἕ (7957)
		} else if (U'Ἠ' <= character && character <= U'Ἧ') { // Ἠ (7976) to Ἧ (7983)
			return character - 8;// ἠ (7968) to ἧ (7975)
		} else if (U'Ἰ' <= character && character <= U'Ἷ') { // Ἰ (7992) to Ἷ (7999)
			return character - 8;// ἰ (7984) to ἷ (7991)
		} else if (U'Ὀ' <= character && character <= U'Ὅ') { // Ὀ (8008) to Ὅ (8013)
			return character - 8;// ὀ (8000) to ὅ (8005)
		} else if (U'Ὑ' <= character && character <= U'Ὗ' && odd) { // Ὑ (8025) to Ὗ (8031)
			return character - 8;// ὑ (8017) to ὗ (8023)
		} else if (U'Ὠ' <= character && character <= U'Ὧ') { // Ὠ (8040) to Ὧ (8047)
			return character - 8;// ὠ (8032) to ὧ (8039)
		} else if (U'ᾈ' <= character && character <= U'ᾏ') { // ᾈ (8072) to ᾏ (8079)
			return character - 8;// ᾀ (8064) to ᾇ (8071)
		} else if (U'ᾘ' <= character && character <= U'ᾟ') { // ᾘ (8088) to ᾟ (8095)
			return character - 8;// ᾐ (8080) to ᾗ (8087)
		} else if (U'ᾨ' <= character && character <= U'ᾯ') { // ᾨ (8104) to ᾯ (8111)
			return character - 8;// ᾠ (8096) to ᾧ (8103)
		} else if (U'Ᾰ' <= character && character <= U'Ᾱ') { // Ᾰ (8120) to Ᾱ (8121)
			return character - 8;// ᾰ (8112) to ᾱ (8113)
		} else if (U'Ῐ' <= character && character <= U'Ῑ') { // Ῐ (8152) to Ῑ (8153)
			return character - 8;// ῐ (8144) to ῑ (8145)
		} else if (U'Ῠ' <= character && character <= U'Ῡ') { // Ῠ (8168) to Ῡ (8169)
			return character - 8;// ῠ (8160) to ῡ (8161)
		} else if (U'Ⓐ' <= character && character <= U'Ⓩ') { // Ⓐ (9398) to Ⓩ (9423)
			return character + 26;// ⓐ (9424) to ⓩ (9449)
		} else if (character == U'Ɽ') { // Ɽ (11364)
			return U'ɽ'; // ɽ (637)
		} else if (character == U'Ɑ') { // Ɑ (11373)
			return U'ɑ'; // ɑ (593)
		} else if (character == U'Ɱ') { // Ɱ (11374)
			return U'ɱ'; // ɱ (625)
		} else if (character == U'Ɐ') { // Ɐ (11375)
			return U'ɐ'; // ɐ (592)
		} else if (character == U'Ɒ') { // Ɒ (11376)
			return U'ɒ'; // ɒ (594)
		} else if (U'Ȿ' <= character && character <= U'Ɀ') { // Ȿ (11390) to Ɀ (11391)
			return character - 10815;// ȿ (575) to ɀ (576)
		} else if (character == U'Ɥ') { // Ɥ (42893)
			return U'ɥ'; // ɥ (613)
		} else if (character == U'Ɪ') { // Ɪ (42926)
			return U'ɪ'; // ɪ (618)
		} else if (U'Ａ' <= character && character <= U'Ｚ') { // Ａ (65313) to Ｚ (65338)
			return character + 32;// ａ (65345) to ｚ (65370)
		} else {
			return character;
		}
	}
}

String dsr::string_upperCase(const ReadableString &text) {
	String result;
	string_reserve(result, text.view.length);
	for (intptr_t i = 0; i < text.view.length; i++) {
		string_appendChar(result, character_upperCase(text[i]));
	}
	return result;
}

String dsr::string_lowerCase(const ReadableString &text) {
	String result;
	string_reserve(result, text.view.length);
	for (intptr_t i = 0; i < text.view.length; i++) {
		string_appendChar(result, character_lowerCase(text[i]));
	}
	return result;
}

bool dsr::string_match(const ReadableString& a, const ReadableString& b) {
	if (a.view.length != b.view.length) {
		return false;
	} else {
		for (intptr_t i = 0; i < a.view.length; i++) {
			if (a[i] != b[i]) {
				return false;
			}
		}
		return true;
	}
}

bool dsr::string_caseInsensitiveMatch(const ReadableString& a, const ReadableString& b) {
	if (a.view.length != b.view.length) {
		return false;
	} else {
		for (intptr_t i = 0; i < a.view.length; i++) {
			if (character_upperCase(a[i]) != character_upperCase(b[i])) {
				return false;
			}
		}
		return true;
	}
}

static intptr_t findFirstNonWhite(const ReadableString &text) {
	for (intptr_t i = 0; i < text.view.length; i++) {
		DsrChar c = text[i];
		if (!character_isWhiteSpace(c)) {
			return i;
		}
	}
	return -1;
}

static intptr_t findLastNonWhite(const ReadableString &text) {
	for (intptr_t i = text.view.length - 1; i >= 0; i--) {
		DsrChar c = text[i];
		if (!character_isWhiteSpace(c)) {
			return i;
		}
	}
	return -1;
}

// Allow passing literals without allocating heap memory for the result
ReadableString dsr::string_removeOuterWhiteSpace(const ReadableString &text) {
	intptr_t first = findFirstNonWhite(text);
	intptr_t last = findLastNonWhite(text);
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
	string_reserve(result, rawText.view.length + 2);
	string_appendChar(result, U'\"'); // Begin quote
	for (intptr_t i = 0; i < rawText.view.length; i++) {
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
	intptr_t firstQuote = string_findFirst(mangledText, '\"');
	intptr_t lastQuote = string_findLast(mangledText, '\"');
	String result;
	if (firstQuote == -1 || lastQuote == -1 || firstQuote == lastQuote) {
		throwError(U"Cannot unmangle using string_unmangleQuote without beginning and ending with quote signs!\n", mangledText, U"\n");
	} else {
		for (intptr_t i = firstQuote + 1; i < lastQuote; i++) {
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
					 throwError(U"Unmangled double quote sign detected in string_unmangleQuote!\n", mangledText, U"\n");
				} else if (c == U'\a') { // Audible bell
					 throwError(U"Unmangled audible bell detected in string_unmangleQuote!\n", mangledText, U"\n");
				} else if (c == U'\b') { // Backspace
					 throwError(U"Unmangled backspace detected in string_unmangleQuote!\n", mangledText, U"\n");
				} else if (c == U'\f') { // Form feed
					 throwError(U"Unmangled form feed detected in string_unmangleQuote!\n", mangledText, U"\n");
				} else if (c == U'\n') { // Line feed
					 throwError(U"Unmangled line feed detected in string_unmangleQuote!\n", mangledText, U"\n");
				} else if (c == U'\r') { // Carriage return
					 throwError(U"Unmangled carriage return detected in string_unmangleQuote!\n", mangledText, U"\n");
				} else if (c == U'\0') { // Null terminator
					 throwError(U"Unmangled null terminator detected in string_unmangleQuote!\n", mangledText, U"\n");
				} else {
					string_appendChar(result, c);
				}
			}
		}
	}
	return result;
}

void dsr::string_fromUnsigned(String& target, uint64_t value) {
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

void dsr::string_fromSigned(String& target, int64_t value, DsrChar negationCharacter) {
	if (value >= 0) {
		string_fromUnsigned(target, (uint64_t)value);
	} else {
		string_appendChar(target, negationCharacter);
		string_fromUnsigned(target, (uint64_t)(-value));
	}
}

static const int MAX_DECIMALS = 16;
static double decimalMultipliers[MAX_DECIMALS] = {
	10.0,
	100.0,
	1000.0,
	10000.0,
	100000.0,
	1000000.0,
	10000000.0,
	100000000.0,
	1000000000.0,
	10000000000.0,
	100000000000.0,
	1000000000000.0,
	10000000000000.0,
	100000000000000.0,
	1000000000000000.0,
	10000000000000000.0
};
static double roundingOffsets[MAX_DECIMALS] = {
	0.05,
	0.005,
	0.0005,
	0.00005,
	0.000005,
	0.0000005,
	0.00000005,
	0.000000005,
	0.0000000005,
	0.00000000005,
	0.000000000005,
	0.0000000000005,
	0.00000000000005,
	0.000000000000005,
	0.0000000000000005,
	0.00000000000000005
};
static uint64_t decimalLimits[MAX_DECIMALS] = {
	9,
	99,
	999,
	9999,
	99999,
	999999,
	9999999,
	99999999,
	999999999,
	9999999999,
	99999999999,
	999999999999,
	9999999999999,
	99999999999999,
	999999999999999,
	9999999999999999
};
void dsr::string_fromDouble(String& target, double value, int decimalCount, bool removeTrailingZeroes, DsrChar decimalCharacter, DsrChar negationCharacter) {
	if (decimalCount < 1) decimalCount = 1;
	if (decimalCount > MAX_DECIMALS) decimalCount = MAX_DECIMALS;
	double remainder = value;
	// Get negation
	if (remainder < 0.0) {
		string_appendChar(target, negationCharacter);
		remainder = -remainder;
	}
	// Apply an offset to make the following truncation round to the closest printable decimal.
	int offsetIndex = decimalCount - 1;
	remainder += roundingOffsets[offsetIndex];
	// Get whole part
	uint64_t whole = (uint64_t)remainder;
	string_fromUnsigned(target, whole);
	// Remove the whole part from the remainder.
	remainder = remainder - whole;
	// Print the decimal
	string_appendChar(target, decimalCharacter);
	// Get decimals
	uint64_t scaledDecimals = uint64_t(remainder * decimalMultipliers[offsetIndex]);
	// Limit decimals to all nines prevent losing a whole unit from fraction overflow.
	uint64_t limit = decimalLimits[offsetIndex];
	if (scaledDecimals > limit) scaledDecimals = limit;
	DsrChar digits[MAX_DECIMALS]; // Using 0 to decimalCount - 1
	int writeIndex = decimalCount - 1;
	for (int d = 0; d < decimalCount; d++) {
		int digit = scaledDecimals % 10;
		digits[writeIndex] = U'0' + digit;
		scaledDecimals = scaledDecimals / 10;
		writeIndex--;
	}
	if (removeTrailingZeroes) {
		// Find the last non-zero decimal, but keep at least one zero.
		int lastValue = 0;
		for (int d = 0; d < decimalCount; d++) {
			if (digits[d] != U'0') lastValue = d;
		}
		// Print until the last value or the only zero.
		for (int d = 0; d <= lastValue; d++) {
			string_appendChar(target, digits[d]);
		}
	} else {
		// Print fixed decimals.
		for (int d = 0; d < decimalCount; d++) {
			string_appendChar(target, digits[d]);
		}
	}
}

#define TO_RAW_ASCII(TARGET, SOURCE) \
	char TARGET[SOURCE.view.length + 1]; \
	for (intptr_t i = 0; i < SOURCE.view.length; i++) { \
		TARGET[i] = toAscii(SOURCE[i]); \
	} \
	TARGET[SOURCE.view.length] = '\0';

// A function definition for receiving a stream of bytes
//   Instead of using std's messy inheritance
using ByteWriterFunction = std::function<void(uint8_t value)>;

// A function definition for receiving a stream of UTF-32 characters
//   Instead of using std's messy inheritance
using UTF32WriterFunction = std::function<void(DsrChar character)>;

// Filter out unwanted characters for improved portability
static void feedCharacter(const UTF32WriterFunction &receiver, DsrChar character) {
	if (character != U'\0' && character != U'\r') {
		receiver(character);
	}
}

// Appends the content of buffer as a BOM-free Latin-1 file into target
// fileLength is ignored when nullTerminated is true
template <bool nullTerminated>
static void feedStringFromFileBuffer_Latin1(const UTF32WriterFunction &receiver, const uint8_t* buffer, intptr_t fileLength = 0) {
	for (intptr_t i = 0; i < fileLength || nullTerminated; i++) {
		DsrChar character = (DsrChar)(buffer[i]);
		if (nullTerminated && character == 0) { return; }
		feedCharacter(receiver, character);
	}
}
// Appends the content of buffer as a BOM-free UTF-8 file into target
// fileLength is ignored when nullTerminated is true
template <bool nullTerminated>
static void feedStringFromFileBuffer_UTF8(const UTF32WriterFunction &receiver, const uint8_t* buffer, intptr_t fileLength = 0) {
	for (intptr_t i = 0; i < fileLength || nullTerminated; i++) {
		uint8_t byteA = buffer[i];
		if (byteA < (uint32_t)0b10000000) {
			// Single byte (1xxxxxxx)
			if (nullTerminated && byteA == 0) { return; }
			feedCharacter(receiver, (DsrChar)byteA);
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
			feedCharacter(receiver, (DsrChar)character);
		}
	}
}

template <bool LittleEndian>
uint16_t read16bits(const uint8_t* buffer, intptr_t startOffset) {
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
static void feedStringFromFileBuffer_UTF16(const UTF32WriterFunction &receiver, const uint8_t* buffer, intptr_t fileLength = 0) {
	for (intptr_t i = 0; i < fileLength || nullTerminated; i += 2) {
		// Read the first 16-bit word
		uint16_t wordA = read16bits<LittleEndian>(buffer, i);
		// Check if another word is needed
		//   Assuming that wordA >= 0x0000 and wordA <= 0xFFFF as uint16_t,
		//   we can just check if it's within the range reserved for 32-bit encoding
		if (wordA <= 0xD7FF || wordA >= 0xE000) {
			// Not in the reserved range, just a single 16-bit character
			if (nullTerminated && wordA == 0) { return; }
			feedCharacter(receiver, (DsrChar)wordA);
		} else {
			// The given range was reserved and therefore using 32 bits
			i += 2;
			uint16_t wordB = read16bits<LittleEndian>(buffer, i);
			uint32_t higher10Bits = wordA & (uint32_t)0b1111111111;
			uint32_t lower10Bits  = wordB & (uint32_t)0b1111111111;
			DsrChar finalChar = (DsrChar)(((higher10Bits << 10) | lower10Bits) + (uint32_t)0x10000);
			feedCharacter(receiver, finalChar);
		}
	}
}
// Sends the decoded UTF-32 characters from the encoded buffer into target.
// The text encoding should be specified using a BOM at the start of buffer, otherwise Latin-1 is assumed.
static void feedStringFromFileBuffer(const UTF32WriterFunction &receiver, const uint8_t* buffer, intptr_t fileLength) {
	// After removing the BOM bytes, the rest can be seen as a BOM-free text file with a known format
	if (fileLength >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) { // UTF-8
		feedStringFromFileBuffer_UTF8<false>(receiver, buffer + 3, fileLength - 3);
	} else if (fileLength >= 2 && buffer[0] == 0xFE && buffer[1] == 0xFF) { // UTF-16 BE
		feedStringFromFileBuffer_UTF16<false, false>(receiver, buffer + 2, fileLength - 2);
	} else if (fileLength >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE) { // UTF-16 LE
		feedStringFromFileBuffer_UTF16<true, false>(receiver, buffer + 2, fileLength - 2);
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
		feedStringFromFileBuffer_Latin1<false>(receiver, buffer, fileLength);
	}
}

// Sends the decoded UTF-32 characters from the encoded null terminated buffer into target.
// buffer may not contain any BOM, and must be null terminated in the specified encoding.
static void feedStringFromRawData(const UTF32WriterFunction &receiver, const uint8_t* buffer, CharacterEncoding encoding) {
	if (encoding == CharacterEncoding::Raw_Latin1) {
		feedStringFromFileBuffer_Latin1<true>(receiver, buffer);
	} else if (encoding == CharacterEncoding::BOM_UTF8) {
		feedStringFromFileBuffer_UTF8<true>(receiver, buffer);
	} else if (encoding == CharacterEncoding::BOM_UTF16BE) {
		feedStringFromFileBuffer_UTF16<false, true>(receiver, buffer);
	} else if (encoding == CharacterEncoding::BOM_UTF16LE) {
		feedStringFromFileBuffer_UTF16<true, true>(receiver, buffer);
	} else {
		throwError(U"Unhandled encoding in feedStringFromRawData!\n");
	}
}

String dsr::string_dangerous_decodeFromData(const void* data, CharacterEncoding encoding) {
	String result;
	// Measure the size of the result by scanning the content in advance
	intptr_t characterCount = 0;
	UTF32WriterFunction measurer = [&characterCount](DsrChar character) {
		characterCount++;
	};
	feedStringFromRawData(measurer, (const uint8_t*)data, encoding);
	// Pre-allocate the correct amount of memory based on the simulation
	string_reserve(result, characterCount);
	// Stream output to the result string
	UTF32WriterFunction receiver = [&result](DsrChar character) {
		string_appendChar(result, character);
	};
	feedStringFromRawData(receiver, (const uint8_t*)data, encoding);
	return result;
}

String dsr::string_loadFromMemory(Buffer fileContent) {
	String result;
	// Measure the size of the result by scanning the content in advance
	intptr_t characterCount = 0;
	UTF32WriterFunction measurer = [&characterCount](DsrChar character) {
		characterCount++;
	};
	feedStringFromFileBuffer(measurer, fileContent.getUnsafe(), fileContent.getUsedSize());
	// Pre-allocate the correct amount of memory based on the simulation
	string_reserve(result, characterCount);
	// Stream output to the result string
	UTF32WriterFunction receiver = [&result](DsrChar character) {
		string_appendChar(result, character);
	};
	feedStringFromFileBuffer(receiver, fileContent.getUnsafe(), fileContent.getUsedSize());
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
	for (intptr_t i = 0; i < string_length(content); i++) {
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
	intptr_t byteCount = 0;
	ByteWriterFunction counter = [&byteCount](uint8_t value) {
		byteCount++;
	};
	ENCODE_TEXT(counter, content, characterEncoding, lineEncoding, writeByteOrderMark, writeNullTerminator);
	Buffer result = buffer_create(byteCount).setName("Buffer holding an encoded string");
	SafePointer<uint8_t> byteWriter = buffer_getSafeData<uint8_t>(result, "Buffer for string encoding");
	ByteWriterFunction receiver = [&byteWriter](uint8_t value) {
		*byteWriter = value;
		byteWriter += 1;
	};
	ENCODE_TEXT(receiver, content, characterEncoding, lineEncoding, writeByteOrderMark, writeNullTerminator);
	return result;
}

static uintptr_t getStartOffset(const ReadableString &source) {
	// Get the allocation
	const uint8_t* origin = (uint8_t*)(source.characters.getUnsafe());
	const uint8_t* start = (uint8_t*)(source.view.getUnchecked());
	assert(start <= origin);
	// Get the offset from the parent
	return (start - origin) / sizeof(DsrChar);
}

#ifdef SAFE_POINTER_CHECKS
	static void serializeCharacterBuffer(PrintCharacter target, void const * const allocation, uintptr_t maxLength) {
		uintptr_t characterCount = heap_getUsedSize(allocation) / sizeof(DsrChar);
		target(U'\"');
		for (uintptr_t c = 0; c < characterCount; c++) {
			if (c == maxLength) {
				target(U'\"');
				target(U'.');
				target(U'.');
				target(U'.');
				return;
			}
			target(((DsrChar *)allocation)[c]);
		}
		target(U'\"');
	}
#endif

static Handle<DsrChar> allocateCharacters(intptr_t minimumLength) {
	// Allocate memory.
	Handle<DsrChar> result = handle_createArray<DsrChar>(AllocationInitialization::Uninitialized, minimumLength).setName("String characters");
	#ifdef SAFE_POINTER_CHECKS
		setAllocationSerialization(result.getUnsafe(), &serializeCharacterBuffer);
	#endif
	// Check how much space we got.
	uintptr_t availableSpace = heap_getAllocationSize(result.getUnsafe());
	// Expand to use all available memory in the allocation.
	uintptr_t newSize = heap_setUsedSize(result.getUnsafe(), availableSpace);
	// Clear the memory to zeroes, just to be safe against non-deterministic bugs.
	safeMemorySet(result.getSafe("Cleared String pointer"), 0, newSize);
	return result;
}

// Replaces the buffer with a new buffer holding at least minimumLength characters
// Guarantees that the new buffer is not shared by other strings, so that it may be written to freely
static void reallocateBuffer(String &target, intptr_t minimumLength, bool preserve) {
	// Holding oldData alive while copying to the new buffer
	Handle<DsrChar> oldBuffer = target.characters; // Kept for reference counting only, do not remove.
	Impl_CharacterView oldData = target.view;
	target.characters = allocateCharacters(minimumLength);
	target.view = Impl_CharacterView(target.characters.getUnsafe(), oldData.length);
	if (preserve && oldData.length > 0) {
		safeMemoryCopy(target.view.getSafe("New characters being copied from an old buffer"), oldData.getSafe("Old characters being copied to a new buffer"), oldData.length * sizeof(DsrChar));
	}
}
// Call before writing to the buffer.
//   This hides that Strings share buffers when assigning by value or taking partial strings.
static void cloneIfNeeded(String &target) {
	// If there is no buffer or the buffer is shared, it needs to allocate its own buffer.
	if (target.characters.isNull() || target.characters.getUseCount() > 1) {
		reallocateBuffer(target, target.view.length, true);
	}
}

void dsr::string_clear(String& target) {
	// We we start writing from the beginning, then we must have our own allocation to avoid overwriting the characters in other strings.
	cloneIfNeeded(target);
	target.view.length = 0;
}

// The number of DsrChar characters that can be contained in the allocation before reaching the buffer's end
//   This doesn't imply that it's always okay to write to the remaining space, because the buffer may be shared
static intptr_t getCapacity(const ReadableString &source) {
	if (source.characters.isNotNull()) {
		uintptr_t bufferElements = source.characters.getElementCount();
		// Subtract offset from the buffer size to get the remaining space
		return bufferElements - getStartOffset(source);
	} else {
		return 0;
	}
}

static void expand(String &target, intptr_t newLength, bool affectUsedLength) {
	cloneIfNeeded(target);
	if (newLength > target.view.length) {
		if (newLength > getCapacity(target)) {
			reallocateBuffer(target, newLength, true);
		}
		if (affectUsedLength) {
			target.view.length = newLength;
		}
	}
}

void dsr::string_reserve(String& target, intptr_t minimumLength) {
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
	intptr_t oldLength = (TARGET).view.length; \
	expand((TARGET), oldLength + (intptr_t)(LENGTH), true); \
	for (intptr_t i = 0; i < (intptr_t)(LENGTH); i++) { \
		(TARGET).view.writeCharacter(oldLength + i, ((SOURCE)[i]) & MASK); \
	} \
}
// TODO: See if ascii litterals can be checked for values above 127 in compile-time
static void atomic_append_ascii(String &target, const char* source) { APPEND(target, source, strlen(source), 0xFF); }
// TODO: Use memcpy when appending input of the same format
static void atomic_append_readable(String &target, const ReadableString& source) { APPEND(target, source, source.view.length, 0xFFFFFFFF); }
static void atomic_append_utf32(String &target, const DsrChar* source) { APPEND(target, source, strlen_utf32(source), 0xFFFFFFFF); }
void dsr::string_appendChar(String& target, DsrChar value) { APPEND(target, &value, 1, 0xFFFFFFFF); }
String& dsr::impl_toStreamIndented_ascii(String& target, const char *value, const ReadableString& indentation) {
	atomic_append_readable(target, indentation);
	atomic_append_ascii(target, value);
	return target;
}
String& dsr::impl_toStreamIndented_utf32(String& target, const char32_t *value, const ReadableString& indentation) {
	atomic_append_readable(target, indentation);
	atomic_append_utf32(target, value);
	return target;
}
String& dsr::impl_toStreamIndented_readable(String& target, const ReadableString& value, const ReadableString& indentation) {
	atomic_append_readable(target, indentation);
	atomic_append_readable(target, value);
	return target;
}
String& dsr::impl_toStreamIndented_double(String& target, const double &value, const ReadableString& indentation) {
	atomic_append_readable(target, indentation);
	string_fromDouble(target, (double)value);
	return target;
}
String& dsr::impl_toStreamIndented_int64(String& target, const int64_t &value, const ReadableString& indentation) {
	atomic_append_readable(target, indentation);
	string_fromSigned(target, value);
	return target;
}
String& dsr::impl_toStreamIndented_uint64(String& target, const uint64_t &value, const ReadableString& indentation) {
	atomic_append_readable(target, indentation);
	string_fromUnsigned(target, value);
	return target;
}

// The print mutex makes sure that messages from multiple threads don't get mixed up.
static std::mutex printMutex;

static std::ostream& toStream(std::ostream& out, const ReadableString &source) {
	for (intptr_t i = 0; i < source.view.length; i++) {
		out.put(toAscii(source.view[i]));
	}
	return out;
}

static const std::function<void(const ReadableString &message, MessageType type)> defaultMessageAction = [](const ReadableString &message, MessageType type) {
	if (type == MessageType::Error) {
		#ifdef DSR_HARD_EXIT_ON_ERROR
			// Print the error.
			toStream(std::cerr, message);
			// Free all heap allocations.
			heap_hardExitCleaning();
			// Terminate with a non-zero value to indicate failure.
			std::exit(1);
		#else
			Buffer ascii = string_saveToMemory(message, CharacterEncoding::Raw_Latin1, LineEncoding::CrLf, false, true);
			throw std::runtime_error((char*)ascii.getUnsafe());
		#endif
	} else {
		printMutex.lock();
			toStream(std::cout, message);
		printMutex.unlock();
	}
};

static std::function<void(const ReadableString &message, MessageType type)> globalMessageAction = defaultMessageAction;

void dsr::string_sendMessage(const ReadableString &message, MessageType type) {
	globalMessageAction(message, type);
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
	intptr_t sectionStart = 0;
	for (intptr_t i = 0; i < source.view.length; i++) {
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
	if (source.view.length > sectionStart) {
		if (removeWhiteSpace) {
			action(string_removeOuterWhiteSpace(string_exclusiveRange(source, sectionStart, source.view.length)));
		} else {
			action(string_exclusiveRange(source, sectionStart, source.view.length));
		}
	}
}

static String createSubString(const Handle<DsrChar> &characters, const Impl_CharacterView &view) {
	String result;
	result.characters = characters;
	result.view = view;
	return result;
}

List<String> dsr::string_split(const ReadableString& source, DsrChar separator, bool removeWhiteSpace) {
	List<String> result;
	if (source.view.length > 0) {
		// Re-use the existing buffer
		String commonBuffer = createSubString(source.characters, source.view);
		// Source is allocated as String
		string_split_callback([&result, removeWhiteSpace](String element) {
			if (removeWhiteSpace) {
				result.push(string_removeOuterWhiteSpace(element));
			} else {
				result.push(element);
			}
		}, commonBuffer, separator, removeWhiteSpace);
	}
	return result;
}

intptr_t dsr::string_splitCount(const ReadableString& source, DsrChar separator) {
	intptr_t result = 0;
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
	for (intptr_t i = 0; i < source.view.length; i++) {
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
	for (intptr_t i = 0; i < source.view.length; i++) {
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
		} else if (c == 'e' || c == 'E') {
			// Apply the exponent after 'e'.
			result *= std::pow(10.0, string_toInteger(string_after(source, i)));
			// Skip remaining characters.
			i = source.view.length;
		}
	}
	if (negated) {
		return -result;
	} else {
		return result;
	}
}

intptr_t dsr::string_length(const ReadableString& source) {
	return source.view.length;
}

intptr_t dsr::string_findFirst(const ReadableString& source, DsrChar toFind, intptr_t startIndex) {
	for (intptr_t i = startIndex; i < source.view.length; i++) {
		if (source[i] == toFind) {
			return i;
		}
	}
	return -1;
}

intptr_t dsr::string_findLast(const ReadableString& source, DsrChar toFind) {
	for (intptr_t i = source.view.length - 1; i >= 0; i--) {
		if (source[i] == toFind) {
			return i;
		}
	}
	return -1;
}

ReadableString dsr::string_exclusiveRange(const ReadableString& source, intptr_t inclusiveStart, intptr_t exclusiveEnd) {
	// Return empty string for each complete miss
	if (inclusiveStart >= source.view.length || exclusiveEnd <= 0) { return ReadableString(); }
	// Automatically clamping to valid range
	if (inclusiveStart < 0) { inclusiveStart = 0; }
	if (exclusiveEnd > source.view.length) { exclusiveEnd = source.view.length; }
	// Return the overlapping interval
	return createSubString(source.characters, Impl_CharacterView(source.view.getUnchecked() + inclusiveStart, exclusiveEnd - inclusiveStart));
}

ReadableString dsr::string_inclusiveRange(const ReadableString& source, intptr_t inclusiveStart, intptr_t inclusiveEnd) {
	return string_exclusiveRange(source, inclusiveStart, inclusiveEnd + 1);
}

ReadableString dsr::string_before(const ReadableString& source, intptr_t exclusiveEnd) {
	return string_exclusiveRange(source, 0, exclusiveEnd);
}

ReadableString dsr::string_until(const ReadableString& source, intptr_t inclusiveEnd) {
	return string_inclusiveRange(source, 0, inclusiveEnd);
}

ReadableString dsr::string_from(const ReadableString& source, intptr_t inclusiveStart) {
	return string_exclusiveRange(source, inclusiveStart, source.view.length);
}

ReadableString dsr::string_after(const ReadableString& source, intptr_t exclusiveStart) {
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
	intptr_t readIndex = 0;
	if (allowWhiteSpace) {
		PATTERN_STAR(WhiteSpace);
	}
	CHARACTER_OPTIONAL(U'-');
	// At least one digit required
	PATTERN_PLUS(IntegerCharacter);
	if (allowWhiteSpace) {
		PATTERN_STAR(WhiteSpace);
	}
	return readIndex == source.view.length;
}

// To avoid consuming the all digits on Digit* before reaching Digit+ when there is no decimal, whole integers are judged by string_isInteger
bool dsr::string_isDouble(const ReadableString& source, bool allowWhiteSpace) {
	// Solving the UnsignedDouble <- Digit+ | Digit* '.' Digit+ ambiguity is done easiest by checking if there's a decimal before handling the white-space and negation
	if (string_findFirst(source, U'.') == -1) {
		// No decimal detected
		return string_isInteger(source, allowWhiteSpace);
	} else {
		intptr_t readIndex = 0;
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
		return readIndex == source.view.length;
	}
}

uintptr_t dsr::string_getBufferUseCount(const ReadableString& text) {
	return text.characters.getUseCount();
}
