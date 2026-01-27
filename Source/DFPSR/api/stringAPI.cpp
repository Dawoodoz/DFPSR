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
String::String(const char* source) { atomic_append_ascii(*this, source); }
String::String(const DsrChar* source) { atomic_append_utf32(*this, source); }

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
		} else if (U'ǹ' <= character && character <= U'ȇ' && odd) { // ǹ (505) to ȇ (519)
			return character - 1; // Ǹ (504) to Ȇ (518)
		} else if (U'ȉ' <= character && character <= U'ȟ' && odd) { // ȉ (521) to ȟ (543)
			return character - 1; // Ȉ (520) to Ȟ (542)
		} else if (U'ȣ' <= character && character <= U'ȳ' && odd) { // ȣ (547) to ȳ (563)
			return character - 1; // Ȣ (546) to Ȳ (562)
		} else if (character == U'ȼ') { // ȼ (572)
			return U'Ȼ'; // Ȼ (571)
		} else if (character == U'ȿ') { // ȿ (575)
			return U'Ȿ'; // Ȿ (11390)
		} else if (character == U'ɀ') { // ɀ (576)
			return U'Ɀ'; // Ɀ (11391)

					// TODO: Compress into ranges.
					} else if (character == U'ɂ') { // ɂ (578)
						return U'Ɂ'; // Ɂ (577)
						return character - 1;
					} else if (character == U'ɇ') { // ɇ (583)
						return U'Ɇ'; // Ɇ (582)
						return character - 1;
					} else if (character == U'ɉ') { // ɉ (585)
						return U'Ɉ'; // Ɉ (584)
						return character - 1;
					} else if (character == U'ɋ') { // ɋ (587)
						return U'Ɋ'; // Ɋ (586)
						return character - 1;
					} else if (character == U'ɍ') { // ɍ (589)
						return U'Ɍ'; // Ɍ (588)
						return character - 1;
					} else if (character == U'ɏ') { // ɏ (591)
						return U'Ɏ'; // Ɏ (590)
						return character - 1;
					} else if (character == U'ɐ') { // ɐ (592)
						return U'Ɐ'; // Ɐ (11375)
						return character + 10783;
					} else if (character == U'ɑ') { // ɑ (593)
						return U'Ɑ'; // Ɑ (11373)
						return character + 10780;
					} else if (character == U'ɒ') { // ɒ (594)
						return U'Ɒ'; // Ɒ (11376)
						return character + 10782;
					} else if (character == U'ɓ') { // ɓ (595)
						return U'Ɓ'; // Ɓ (385)
						return character - 210;
					} else if (character == U'ɔ') { // ɔ (596)
						return U'Ɔ'; // Ɔ (390)
						return character - 206;
					} else if (character == U'ɖ') { // ɖ (598)
						return U'Ɖ'; // Ɖ (393)
						return character - 205;
					} else if (character == U'ɗ') { // ɗ (599)
						return U'Ɗ'; // Ɗ (394)
						return character - 205;
					} else if (character == U'ɘ') { // ɘ (600)
						return U'Ǝ'; // Ǝ (398)
						return character - 202;
					} else if (character == U'ə') { // ə (601)
						return U'Ə'; // Ə (399)
						return character - 202;
					} else if (character == U'ɛ') { // ɛ (603)
						return U'Ɛ'; // Ɛ (400)
						return character - 203;
					} else if (character == U'ɠ') { // ɠ (608)
						return U'Ɠ'; // Ɠ (403)
						return character - 205;
					} else if (character == U'ɣ') { // ɣ (611)
						return U'Ɣ'; // Ɣ (404)
						return character - 207;
					} else if (character == U'ɥ') { // ɥ (613)
						return U'Ɥ'; // Ɥ (42893)
						return character + 42280;
					} else if (character == U'ɨ') { // ɨ (616)
						return U'Ɨ'; // Ɨ (407)
						return character - 209;
					} else if (character == U'ɩ') { // ɩ (617)
						return U'Ɩ'; // Ɩ (406)
						return character - 211;
					} else if (character == U'ɪ') { // ɪ (618)
						return U'Ɪ'; // Ɪ (42926)
						return character + 42308;
					} else if (character == U'ɯ') { // ɯ (623)
						return U'Ɯ'; // Ɯ (412)
						return character - 211;
					} else if (character == U'ɱ') { // ɱ (625)
						return U'Ɱ'; // Ɱ (11374)
						return character + 10749;
					} else if (character == U'ɲ') { // ɲ (626)
						return U'Ɲ'; // Ɲ (413)
						return character - 213;
					} else if (character == U'ɵ') { // ɵ (629)
						return U'Ɵ'; // Ɵ (415)
						return character - 214;
					} else if (character == U'ɽ') { // ɽ (637)
						return U'Ɽ'; // Ɽ (11364)
						return character + 10727;
					} else if (character == U'ʀ') { // ʀ (640)
						return U'Ʀ'; // Ʀ (422)
						return character - 218;
					} else if (character == U'ʈ') { // ʈ (648)
						return U'Ʈ'; // Ʈ (430)
						return character - 218;
					} else if (character == U'ʉ') { // ʉ (649)
						return U'Ʉ'; // Ʉ (580)
						return character - 69;
					} else if (character == U'ʊ') { // ʊ (650)
						return U'Ʊ'; // Ʊ (433)
						return character - 217;
					} else if (character == U'ʋ') { // ʋ (651)
						return U'Ʋ'; // Ʋ (434)
						return character - 217;
					} else if (character == U'ʌ') { // ʌ (652)
						return U'Ʌ'; // Ʌ (581)
						return character - 71;
					} else if (character == U'ʒ') { // ʒ (658)
						return U'Ʒ'; // Ʒ (439)
						return character - 219;
					} else if (character == U'ʔ') { // ʔ (660)
						return U'ˀ'; // ˀ (704)
						return character + 44;
					} else if (character == U'ά') { // ά (940)
						return U'Ά'; // Ά (902)
						return character - 38;
					} else if (character == U'έ') { // έ (941)
						return U'Έ'; // Έ (904)
						return character - 37;
					} else if (character == U'ή') { // ή (942)
						return U'Ή'; // Ή (905)
						return character - 37;
					} else if (character == U'ί') { // ί (943)
						return U'Ί'; // Ί (906)
						return character - 37;
					} else if (character == U'α') { // α (945)
						return U'Α'; // Α (913)
						return character - 32;
					} else if (character == U'β') { // β (946)
						return U'Β'; // Β (914)
						return character - 32;
					} else if (character == U'γ') { // γ (947)
						return U'Γ'; // Γ (915)
						return character - 32;
					} else if (character == U'δ') { // δ (948)
						return U'Δ'; // Δ (916)
						return character - 32;
					} else if (character == U'ε') { // ε (949)
						return U'Ε'; // Ε (917)
						return character - 32;
					} else if (character == U'ζ') { // ζ (950)
						return U'Ζ'; // Ζ (918)
						return character - 32;
					} else if (character == U'η') { // η (951)
						return U'Η'; // Η (919)
						return character - 32;
					} else if (character == U'θ') { // θ (952)
						return U'Θ'; // Θ (920)
						return character - 32;
					} else if (character == U'ι') { // ι (953)
						return U'Ι'; // Ι (921)
						return character - 32;
					} else if (character == U'κ') { // κ (954)
						return U'Κ'; // Κ (922)
						return character - 32;
					} else if (character == U'λ') { // λ (955)
						return U'Λ'; // Λ (923)
						return character - 32;
					} else if (character == U'μ') { // μ (956)
						return U'Μ'; // Μ (924)
						return character - 32;
					} else if (character == U'ν') { // ν (957)
						return U'Ν'; // Ν (925)
						return character - 32;
					} else if (character == U'ξ') { // ξ (958)
						return U'Ξ'; // Ξ (926)
						return character - 32;
					} else if (character == U'ο') { // ο (959)
						return U'Ο'; // Ο (927)
						return character - 32;
					} else if (character == U'π') { // π (960)
						return U'Π'; // Π (928)
						return character - 32;
					} else if (character == U'ρ') { // ρ (961)
						return U'Ρ'; // Ρ (929)
						return character - 32;
					} else if (character == U'σ') { // σ (963)
						return U'Σ'; // Σ (931)
						return character - 32;
					} else if (character == U'τ') { // τ (964)
						return U'Τ'; // Τ (932)
						return character - 32;
					} else if (character == U'υ') { // υ (965)
						return U'Υ'; // Υ (933)
						return character - 32;
					} else if (character == U'φ') { // φ (966)
						return U'Φ'; // Φ (934)
						return character - 32;
					} else if (character == U'χ') { // χ (967)
						return U'Χ'; // Χ (935)
						return character - 32;
					} else if (character == U'ψ') { // ψ (968)
						return U'Ψ'; // Ψ (936)
						return character - 32;
					} else if (character == U'ω') { // ω (969)
						return U'Ω'; // Ω (937)
						return character - 32;
					} else if (character == U'ϊ') { // ϊ (970)
						return U'Ϊ'; // Ϊ (938)
						return character - 32;
					} else if (character == U'ϋ') { // ϋ (971)
						return U'Ϋ'; // Ϋ (939)
						return character - 32;
					} else if (character == U'ό') { // ό (972)
						return U'Ό'; // Ό (908)
						return character - 64;
					} else if (character == U'ύ') { // ύ (973)
						return U'Ύ'; // Ύ (910)
						return character - 63;
					} else if (character == U'ώ') { // ώ (974)
						return U'Ώ'; // Ώ (911)
						return character - 63;
					} else if (character == U'ϣ') { // ϣ (995)
						return U'Ϣ'; // Ϣ (994)
						return character - 1;
					} else if (character == U'ϥ') { // ϥ (997)
						return U'Ϥ'; // Ϥ (996)
						return character - 1;
					} else if (character == U'ϧ') { // ϧ (999)
						return U'Ϧ'; // Ϧ (998)
						return character - 1;
					} else if (character == U'ϩ') { // ϩ (1001)
						return U'Ϩ'; // Ϩ (1000)
						return character - 1;
					} else if (character == U'ϫ') { // ϫ (1003)
						return U'Ϫ'; // Ϫ (1002)
						return character - 1;
					} else if (character == U'ϭ') { // ϭ (1005)
						return U'Ϭ'; // Ϭ (1004)
						return character - 1;
					} else if (character == U'ϯ') { // ϯ (1007)
						return U'Ϯ'; // Ϯ (1006)
						return character - 1;
					} else if (character == U'а') { // а (1072)
						return U'А'; // А (1040)
						return character - 32;
					} else if (character == U'б') { // б (1073)
						return U'Б'; // Б (1041)
						return character - 32;
					} else if (character == U'в') { // в (1074)
						return U'В'; // В (1042)
						return character - 32;
					} else if (character == U'г') { // г (1075)
						return U'Г'; // Г (1043)
						return character - 32;
					} else if (character == U'д') { // д (1076)
						return U'Д'; // Д (1044)
						return character - 32;
					} else if (character == U'е') { // е (1077)
						return U'Е'; // Е (1045)
						return character - 32;
					} else if (character == U'ж') { // ж (1078)
						return U'Ж'; // Ж (1046)
						return character - 32;
					} else if (character == U'з') { // з (1079)
						return U'З'; // З (1047)
						return character - 32;
					} else if (character == U'и') { // и (1080)
						return U'И'; // И (1048)
						return character - 32;
					} else if (character == U'й') { // й (1081)
						return U'Й'; // Й (1049)
						return character - 32;
					} else if (character == U'к') { // к (1082)
						return U'К'; // К (1050)
						return character - 32;
					} else if (character == U'л') { // л (1083)
						return U'Л'; // Л (1051)
						return character - 32;
					} else if (character == U'м') { // м (1084)
						return U'М'; // М (1052)
						return character - 32;
					} else if (character == U'н') { // н (1085)
						return U'Н'; // Н (1053)
						return character - 32;
					} else if (character == U'о') { // о (1086)
						return U'О'; // О (1054)
						return character - 32;
					} else if (character == U'п') { // п (1087)
						return U'П'; // П (1055)
						return character - 32;
					} else if (character == U'р') { // р (1088)
						return U'Р'; // Р (1056)
						return character - 32;
					} else if (character == U'с') { // с (1089)
						return U'С'; // С (1057)
						return character - 32;
					} else if (character == U'т') { // т (1090)
						return U'Т'; // Т (1058)
						return character - 32;
					} else if (character == U'у') { // у (1091)
						return U'У'; // У (1059)
						return character - 32;
					} else if (character == U'ф') { // ф (1092)
						return U'Ф'; // Ф (1060)
						return character - 32;
					} else if (character == U'х') { // х (1093)
						return U'Х'; // Х (1061)
						return character - 32;
					} else if (character == U'ц') { // ц (1094)
						return U'Ц'; // Ц (1062)
						return character - 32;
					} else if (character == U'ч') { // ч (1095)
						return U'Ч'; // Ч (1063)
						return character - 32;
					} else if (character == U'ш') { // ш (1096)
						return U'Ш'; // Ш (1064)
						return character - 32;
					} else if (character == U'щ') { // щ (1097)
						return U'Щ'; // Щ (1065)
						return character - 32;
					} else if (character == U'ъ') { // ъ (1098)
						return U'Ъ'; // Ъ (1066)
						return character - 32;
					} else if (character == U'ы') { // ы (1099)
						return U'Ы'; // Ы (1067)
						return character - 32;
					} else if (character == U'ь') { // ь (1100)
						return U'Ь'; // Ь (1068)
						return character - 32;
					} else if (character == U'э') { // э (1101)
						return U'Э'; // Э (1069)
						return character - 32;
					} else if (character == U'ю') { // ю (1102)
						return U'Ю'; // Ю (1070)
						return character - 32;
					} else if (character == U'я') { // я (1103)
						return U'Я'; // Я (1071)
						return character - 32;
					} else if (character == U'ё') { // ё (1105)
						return U'Ё'; // Ё (1025)
						return character - 80;
					} else if (character == U'ђ') { // ђ (1106)
						return U'Ђ'; // Ђ (1026)
						return character - 80;
					} else if (character == U'ѓ') { // ѓ (1107)
						return U'Ѓ'; // Ѓ (1027)
						return character - 80;
					} else if (character == U'є') { // є (1108)
						return U'Є'; // Є (1028)
						return character - 80;
					} else if (character == U'ѕ') { // ѕ (1109)
						return U'Ѕ'; // Ѕ (1029)
						return character - 80;
					} else if (character == U'і') { // і (1110)
						return U'І'; // І (1030)
						return character - 80;
					} else if (character == U'ї') { // ї (1111)
						return U'Ї'; // Ї (1031)
						return character - 80;
					} else if (character == U'ј') { // ј (1112)
						return U'Ј'; // Ј (1032)
						return character - 80;
					} else if (character == U'љ') { // љ (1113)
						return U'Љ'; // Љ (1033)
						return character - 80;
					} else if (character == U'њ') { // њ (1114)
						return U'Њ'; // Њ (1034)
						return character - 80;
					} else if (character == U'ћ') { // ћ (1115)
						return U'Ћ'; // Ћ (1035)
						return character - 80;
					} else if (character == U'ќ') { // ќ (1116)
						return U'Ќ'; // Ќ (1036)
						return character - 80;
					} else if (character == U'ў') { // ў (1118)
						return U'Ў'; // Ў (1038)
						return character - 80;
					} else if (character == U'џ') { // џ (1119)
						return U'Џ'; // Џ (1039)
						return character - 80;
					} else if (character == U'ѡ') { // ѡ (1121)
						return U'Ѡ'; // Ѡ (1120)
						return character - 1;
					} else if (character == U'ѣ') { // ѣ (1123)
						return U'Ѣ'; // Ѣ (1122)
						return character - 1;
					} else if (character == U'ѥ') { // ѥ (1125)
						return U'Ѥ'; // Ѥ (1124)
						return character - 1;
					} else if (character == U'ѧ') { // ѧ (1127)
						return U'Ѧ'; // Ѧ (1126)
						return character - 1;
					} else if (character == U'ѩ') { // ѩ (1129)
						return U'Ѩ'; // Ѩ (1128)
						return character - 1;
					} else if (character == U'ѫ') { // ѫ (1131)
						return U'Ѫ'; // Ѫ (1130)
						return character - 1;
					} else if (character == U'ѭ') { // ѭ (1133)
						return U'Ѭ'; // Ѭ (1132)
						return character - 1;
					} else if (character == U'ѯ') { // ѯ (1135)
						return U'Ѯ'; // Ѯ (1134)
						return character - 1;
					} else if (character == U'ѱ') { // ѱ (1137)
						return U'Ѱ'; // Ѱ (1136)
						return character - 1;
					} else if (character == U'ѳ') { // ѳ (1139)
						return U'Ѳ'; // Ѳ (1138)
						return character - 1;
					} else if (character == U'ѵ') { // ѵ (1141)
						return U'Ѵ'; // Ѵ (1140)
						return character - 1;
					} else if (character == U'ѷ') { // ѷ (1143)
						return U'Ѷ'; // Ѷ (1142)
						return character - 1;
					} else if (character == U'ѹ') { // ѹ (1145)
						return U'Ѹ'; // Ѹ (1144)
						return character - 1;
					} else if (character == U'ѻ') { // ѻ (1147)
						return U'Ѻ'; // Ѻ (1146)
						return character - 1;
					} else if (character == U'ѽ') { // ѽ (1149)
						return U'Ѽ'; // Ѽ (1148)
						return character - 1;
					} else if (character == U'ѿ') { // ѿ (1151)
						return U'Ѿ'; // Ѿ (1150)
						return character - 1;
					} else if (character == U'ҁ') { // ҁ (1153)
						return U'Ҁ'; // Ҁ (1152)
						return character - 1;
					} else if (character == U'ґ') { // ґ (1169)
						return U'Ґ'; // Ґ (1168)
						return character - 1;
					} else if (character == U'ғ') { // ғ (1171)
						return U'Ғ'; // Ғ (1170)
						return character - 1;
					} else if (character == U'ҕ') { // ҕ (1173)
						return U'Ҕ'; // Ҕ (1172)
						return character - 1;
					} else if (character == U'җ') { // җ (1175)
						return U'Җ'; // Җ (1174)
						return character - 1;
					} else if (character == U'ҙ') { // ҙ (1177)
						return U'Ҙ'; // Ҙ (1176)
						return character - 1;
					} else if (character == U'қ') { // қ (1179)
						return U'Қ'; // Қ (1178)
						return character - 1;
					} else if (character == U'ҝ') { // ҝ (1181)
						return U'Ҝ'; // Ҝ (1180)
						return character - 1;
					} else if (character == U'ҟ') { // ҟ (1183)
						return U'Ҟ'; // Ҟ (1182)
						return character - 1;
					} else if (character == U'ҡ') { // ҡ (1185)
						return U'Ҡ'; // Ҡ (1184)
						return character - 1;
					} else if (character == U'ң') { // ң (1187)
						return U'Ң'; // Ң (1186)
						return character - 1;
					} else if (character == U'ҥ') { // ҥ (1189)
						return U'Ҥ'; // Ҥ (1188)
						return character - 1;
					} else if (character == U'ҧ') { // ҧ (1191)
						return U'Ҧ'; // Ҧ (1190)
						return character - 1;
					} else if (character == U'ҩ') { // ҩ (1193)
						return U'Ҩ'; // Ҩ (1192)
						return character - 1;
					} else if (character == U'ҫ') { // ҫ (1195)
						return U'Ҫ'; // Ҫ (1194)
						return character - 1;
					} else if (character == U'ҭ') { // ҭ (1197)
						return U'Ҭ'; // Ҭ (1196)
						return character - 1;
					} else if (character == U'ү') { // ү (1199)
						return U'Ү'; // Ү (1198)
						return character - 1;
					} else if (character == U'ұ') { // ұ (1201)
						return U'Ұ'; // Ұ (1200)
						return character - 1;
					} else if (character == U'ҳ') { // ҳ (1203)
						return U'Ҳ'; // Ҳ (1202)
						return character - 1;
					} else if (character == U'ҵ') { // ҵ (1205)
						return U'Ҵ'; // Ҵ (1204)
						return character - 1;
					} else if (character == U'ҷ') { // ҷ (1207)
						return U'Ҷ'; // Ҷ (1206)
						return character - 1;
					} else if (character == U'ҹ') { // ҹ (1209)
						return U'Ҹ'; // Ҹ (1208)
						return character - 1;
					} else if (character == U'һ') { // һ (1211)
						return U'Һ'; // Һ (1210)
						return character - 1;
					} else if (character == U'ҽ') { // ҽ (1213)
						return U'Ҽ'; // Ҽ (1212)
						return character - 1;
					} else if (character == U'ҿ') { // ҿ (1215)
						return U'Ҿ'; // Ҿ (1214)
						return character - 1;
					} else if (character == U'ӂ') { // ӂ (1218)
						return U'Ӂ'; // Ӂ (1217)
						return character - 1;
					} else if (character == U'ӄ') { // ӄ (1220)
						return U'Ӄ'; // Ӄ (1219)
						return character - 1;
					} else if (character == U'ӈ') { // ӈ (1224)
						return U'Ӈ'; // Ӈ (1223)
						return character - 1;
					} else if (character == U'ӌ') { // ӌ (1228)
						return U'Ӌ'; // Ӌ (1227)
						return character - 1;
					} else if (character == U'ӑ') { // ӑ (1233)
						return U'Ӑ'; // Ӑ (1232)
						return character - 1;
					} else if (character == U'ӓ') { // ӓ (1235)
						return U'Ӓ'; // Ӓ (1234)
						return character - 1;
					} else if (character == U'ӕ') { // ӕ (1237)
						return U'Ӕ'; // Ӕ (1236)
						return character - 1;
					} else if (character == U'ӗ') { // ӗ (1239)
						return U'Ӗ'; // Ӗ (1238)
						return character - 1;
					} else if (character == U'ә') { // ә (1241)
						return U'Ә'; // Ә (1240)
						return character - 1;
					} else if (character == U'ӛ') { // ӛ (1243)
						return U'Ӛ'; // Ӛ (1242)
						return character - 1;
					} else if (character == U'ӝ') { // ӝ (1245)
						return U'Ӝ'; // Ӝ (1244)
						return character - 1;
					} else if (character == U'ӟ') { // ӟ (1247)
						return U'Ӟ'; // Ӟ (1246)
						return character - 1;
					} else if (character == U'ӡ') { // ӡ (1249)
						return U'Ӡ'; // Ӡ (1248)
						return character - 1;
					} else if (character == U'ӣ') { // ӣ (1251)
						return U'Ӣ'; // Ӣ (1250)
						return character - 1;
					} else if (character == U'ӥ') { // ӥ (1253)
						return U'Ӥ'; // Ӥ (1252)
						return character - 1;
					} else if (character == U'ӧ') { // ӧ (1255)
						return U'Ӧ'; // Ӧ (1254)
						return character - 1;
					} else if (character == U'ө') { // ө (1257)
						return U'Ө'; // Ө (1256)
						return character - 1;
					} else if (character == U'ӫ') { // ӫ (1259)
						return U'Ӫ'; // Ӫ (1258)
						return character - 1;
					} else if (character == U'ӯ') { // ӯ (1263)
						return U'Ӯ'; // Ӯ (1262)
						return character - 1;
					} else if (character == U'ӱ') { // ӱ (1265)
						return U'Ӱ'; // Ӱ (1264)
						return character - 1;
					} else if (character == U'ӳ') { // ӳ (1267)
						return U'Ӳ'; // Ӳ (1266)
						return character - 1;
					} else if (character == U'ӵ') { // ӵ (1269)
						return U'Ӵ'; // Ӵ (1268)
						return character - 1;
					} else if (character == U'ӹ') { // ӹ (1273)
						return U'Ӹ'; // Ӹ (1272)
						return character - 1;
					} else if (character == U'ա') { // ա (1377)
						return U'Ա'; // Ա (1329)
						return character - 48;
					} else if (character == U'բ') { // բ (1378)
						return U'Բ'; // Բ (1330)
						return character - 48;
					} else if (character == U'գ') { // գ (1379)
						return U'Գ'; // Գ (1331)
						return character - 48;
					} else if (character == U'դ') { // դ (1380)
						return U'Դ'; // Դ (1332)
						return character - 48;
					} else if (character == U'ե') { // ե (1381)
						return U'Ե'; // Ե (1333)
						return character - 48;
					} else if (character == U'զ') { // զ (1382)
						return U'Զ'; // Զ (1334)
						return character - 48;
					} else if (character == U'է') { // է (1383)
						return U'Է'; // Է (1335)
						return character - 48;
					} else if (character == U'ը') { // ը (1384)
						return U'Ը'; // Ը (1336)
						return character - 48;
					} else if (character == U'թ') { // թ (1385)
						return U'Թ'; // Թ (1337)
						return character - 48;
					} else if (character == U'ժ') { // ժ (1386)
						return U'Ժ'; // Ժ (1338)
						return character - 48;
					} else if (character == U'ի') { // ի (1387)
						return U'Ի'; // Ի (1339)
						return character - 48;
					} else if (character == U'լ') { // լ (1388)
						return U'Լ'; // Լ (1340)
						return character - 48;
					} else if (character == U'խ') { // խ (1389)
						return U'Խ'; // Խ (1341)
						return character - 48;
					} else if (character == U'ծ') { // ծ (1390)
						return U'Ծ'; // Ծ (1342)
						return character - 48;
					} else if (character == U'կ') { // կ (1391)
						return U'Կ'; // Կ (1343)
						return character - 48;
					} else if (character == U'հ') { // հ (1392)
						return U'Հ'; // Հ (1344)
						return character - 48;
					} else if (character == U'ձ') { // ձ (1393)
						return U'Ձ'; // Ձ (1345)
						return character - 48;
					} else if (character == U'ղ') { // ղ (1394)
						return U'Ղ'; // Ղ (1346)
						return character - 48;
					} else if (character == U'ճ') { // ճ (1395)
						return U'Ճ'; // Ճ (1347)
						return character - 48;
					} else if (character == U'մ') { // մ (1396)
						return U'Մ'; // Մ (1348)
						return character - 48;
					} else if (character == U'յ') { // յ (1397)
						return U'Յ'; // Յ (1349)
						return character - 48;
					} else if (character == U'ն') { // ն (1398)
						return U'Ն'; // Ն (1350)
						return character - 48;
					} else if (character == U'շ') { // շ (1399)
						return U'Շ'; // Շ (1351)
						return character - 48;
					} else if (character == U'ո') { // ո (1400)
						return U'Ո'; // Ո (1352)
						return character - 48;
					} else if (character == U'չ') { // չ (1401)
						return U'Չ'; // Չ (1353)
						return character - 48;
					} else if (character == U'պ') { // պ (1402)
						return U'Պ'; // Պ (1354)
						return character - 48;
					} else if (character == U'ջ') { // ջ (1403)
						return U'Ջ'; // Ջ (1355)
						return character - 48;
					} else if (character == U'ռ') { // ռ (1404)
						return U'Ռ'; // Ռ (1356)
						return character - 48;
					} else if (character == U'ս') { // ս (1405)
						return U'Ս'; // Ս (1357)
						return character - 48;
					} else if (character == U'վ') { // վ (1406)
						return U'Վ'; // Վ (1358)
						return character - 48;
					} else if (character == U'տ') { // տ (1407)
						return U'Տ'; // Տ (1359)
						return character - 48;
					} else if (character == U'ր') { // ր (1408)
						return U'Ր'; // Ր (1360)
						return character - 48;
					} else if (character == U'ց') { // ց (1409)
						return U'Ց'; // Ց (1361)
						return character - 48;
					} else if (character == U'ւ') { // ւ (1410)
						return U'Ւ'; // Ւ (1362)
						return character - 48;
					} else if (character == U'փ') { // փ (1411)
						return U'Փ'; // Փ (1363)
						return character - 48;
					} else if (character == U'ք') { // ք (1412)
						return U'Ք'; // Ք (1364)
						return character - 48;
					} else if (character == U'օ') { // օ (1413)
						return U'Օ'; // Օ (1365)
						return character - 48;
					} else if (character == U'ֆ') { // ֆ (1414)
						return U'Ֆ'; // Ֆ (1366)
						return character - 48;
					} else if (character == U'ა') { // ა (4304)
						return U'Ⴀ'; // Ⴀ (4256)
						return character - 48;
					} else if (character == U'ბ') { // ბ (4305)
						return U'Ⴁ'; // Ⴁ (4257)
						return character - 48;
					} else if (character == U'გ') { // გ (4306)
						return U'Ⴂ'; // Ⴂ (4258)
						return character - 48;
					} else if (character == U'დ') { // დ (4307)
						return U'Ⴃ'; // Ⴃ (4259)
						return character - 48;
					} else if (character == U'ე') { // ე (4308)
						return U'Ⴄ'; // Ⴄ (4260)
						return character - 48;
					} else if (character == U'ვ') { // ვ (4309)
						return U'Ⴅ'; // Ⴅ (4261)
						return character - 48;
					} else if (character == U'ზ') { // ზ (4310)
						return U'Ⴆ'; // Ⴆ (4262)
						return character - 48;
					} else if (character == U'თ') { // თ (4311)
						return U'Ⴇ'; // Ⴇ (4263)
						return character - 48;
					} else if (character == U'ი') { // ი (4312)
						return U'Ⴈ'; // Ⴈ (4264)
						return character - 48;
					} else if (character == U'კ') { // კ (4313)
						return U'Ⴉ'; // Ⴉ (4265)
						return character - 48;
					} else if (character == U'ლ') { // ლ (4314)
						return U'Ⴊ'; // Ⴊ (4266)
						return character - 48;
					} else if (character == U'მ') { // მ (4315)
						return U'Ⴋ'; // Ⴋ (4267)
						return character - 48;
					} else if (character == U'ნ') { // ნ (4316)
						return U'Ⴌ'; // Ⴌ (4268)
						return character - 48;
					} else if (character == U'ო') { // ო (4317)
						return U'Ⴍ'; // Ⴍ (4269)
						return character - 48;
					} else if (character == U'პ') { // პ (4318)
						return U'Ⴎ'; // Ⴎ (4270)
						return character - 48;
					} else if (character == U'ჟ') { // ჟ (4319)
						return U'Ⴏ'; // Ⴏ (4271)
						return character - 48;
					} else if (character == U'რ') { // რ (4320)
						return U'Ⴐ'; // Ⴐ (4272)
						return character - 48;
					} else if (character == U'ს') { // ს (4321)
						return U'Ⴑ'; // Ⴑ (4273)
						return character - 48;
					} else if (character == U'ტ') { // ტ (4322)
						return U'Ⴒ'; // Ⴒ (4274)
						return character - 48;
					} else if (character == U'უ') { // უ (4323)
						return U'Ⴓ'; // Ⴓ (4275)
						return character - 48;
					} else if (character == U'ფ') { // ფ (4324)
						return U'Ⴔ'; // Ⴔ (4276)
						return character - 48;
					} else if (character == U'ქ') { // ქ (4325)
						return U'Ⴕ'; // Ⴕ (4277)
						return character - 48;
					} else if (character == U'ღ') { // ღ (4326)
						return U'Ⴖ'; // Ⴖ (4278)
						return character - 48;
					} else if (character == U'ყ') { // ყ (4327)
						return U'Ⴗ'; // Ⴗ (4279)
						return character - 48;
					} else if (character == U'შ') { // შ (4328)
						return U'Ⴘ'; // Ⴘ (4280)
						return character - 48;
					} else if (character == U'ჩ') { // ჩ (4329)
						return U'Ⴙ'; // Ⴙ (4281)
						return character - 48;
					} else if (character == U'ც') { // ც (4330)
						return U'Ⴚ'; // Ⴚ (4282)
						return character - 48;
					} else if (character == U'ძ') { // ძ (4331)
						return U'Ⴛ'; // Ⴛ (4283)
						return character - 48;
					} else if (character == U'წ') { // წ (4332)
						return U'Ⴜ'; // Ⴜ (4284)
						return character - 48;
					} else if (character == U'ჭ') { // ჭ (4333)
						return U'Ⴝ'; // Ⴝ (4285)
						return character - 48;
					} else if (character == U'ხ') { // ხ (4334)
						return U'Ⴞ'; // Ⴞ (4286)
						return character - 48;
					} else if (character == U'ჯ') { // ჯ (4335)
						return U'Ⴟ'; // Ⴟ (4287)
						return character - 48;
					} else if (character == U'ჰ') { // ჰ (4336)
						return U'Ⴠ'; // Ⴠ (4288)
						return character - 48;
					} else if (character == U'ჱ') { // ჱ (4337)
						return U'Ⴡ'; // Ⴡ (4289)
						return character - 48;
					} else if (character == U'ჲ') { // ჲ (4338)
						return U'Ⴢ'; // Ⴢ (4290)
						return character - 48;
					} else if (character == U'ჳ') { // ჳ (4339)
						return U'Ⴣ'; // Ⴣ (4291)
						return character - 48;
					} else if (character == U'ჴ') { // ჴ (4340)
						return U'Ⴤ'; // Ⴤ (4292)
						return character - 48;
					} else if (character == U'ჵ') { // ჵ (4341)
						return U'Ⴥ'; // Ⴥ (4293)
						return character - 48;
					} else if (character == U'ḁ') { // ḁ (7681)
						return U'Ḁ'; // Ḁ (7680)
						return character - 1;
					} else if (character == U'ḃ') { // ḃ (7683)
						return U'Ḃ'; // Ḃ (7682)
						return character - 1;
					} else if (character == U'ḅ') { // ḅ (7685)
						return U'Ḅ'; // Ḅ (7684)
						return character - 1;
					} else if (character == U'ḇ') { // ḇ (7687)
						return U'Ḇ'; // Ḇ (7686)
						return character - 1;
					} else if (character == U'ḉ') { // ḉ (7689)
						return U'Ḉ'; // Ḉ (7688)
						return character - 1;
					} else if (character == U'ḋ') { // ḋ (7691)
						return U'Ḋ'; // Ḋ (7690)
						return character - 1;
					} else if (character == U'ḍ') { // ḍ (7693)
						return U'Ḍ'; // Ḍ (7692)
						return character - 1;
					} else if (character == U'ḏ') { // ḏ (7695)
						return U'Ḏ'; // Ḏ (7694)
						return character - 1;
					} else if (character == U'ḑ') { // ḑ (7697)
						return U'Ḑ'; // Ḑ (7696)
						return character - 1;
					} else if (character == U'ḓ') { // ḓ (7699)
						return U'Ḓ'; // Ḓ (7698)
						return character - 1;
					} else if (character == U'ḕ') { // ḕ (7701)
						return U'Ḕ'; // Ḕ (7700)
						return character - 1;
					} else if (character == U'ḗ') { // ḗ (7703)
						return U'Ḗ'; // Ḗ (7702)
						return character - 1;
					} else if (character == U'ḙ') { // ḙ (7705)
						return U'Ḙ'; // Ḙ (7704)
						return character - 1;
					} else if (character == U'ḛ') { // ḛ (7707)
						return U'Ḛ'; // Ḛ (7706)
						return character - 1;
					} else if (character == U'ḝ') { // ḝ (7709)
						return U'Ḝ'; // Ḝ (7708)
						return character - 1;
					} else if (character == U'ḟ') { // ḟ (7711)
						return U'Ḟ'; // Ḟ (7710)
						return character - 1;
					} else if (character == U'ḡ') { // ḡ (7713)
						return U'Ḡ'; // Ḡ (7712)
						return character - 1;
					} else if (character == U'ḣ') { // ḣ (7715)
						return U'Ḣ'; // Ḣ (7714)
						return character - 1;
					} else if (character == U'ḥ') { // ḥ (7717)
						return U'Ḥ'; // Ḥ (7716)
						return character - 1;
					} else if (character == U'ḧ') { // ḧ (7719)
						return U'Ḧ'; // Ḧ (7718)
						return character - 1;
					} else if (character == U'ḩ') { // ḩ (7721)
						return U'Ḩ'; // Ḩ (7720)
						return character - 1;
					} else if (character == U'ḫ') { // ḫ (7723)
						return U'Ḫ'; // Ḫ (7722)
						return character - 1;
					} else if (character == U'ḭ') { // ḭ (7725)
						return U'Ḭ'; // Ḭ (7724)
						return character - 1;
					} else if (character == U'ḯ') { // ḯ (7727)
						return U'Ḯ'; // Ḯ (7726)
						return character - 1;
					} else if (character == U'ḱ') { // ḱ (7729)
						return U'Ḱ'; // Ḱ (7728)
						return character - 1;
					} else if (character == U'ḳ') { // ḳ (7731)
						return U'Ḳ'; // Ḳ (7730)
						return character - 1;
					} else if (character == U'ḵ') { // ḵ (7733)
						return U'Ḵ'; // Ḵ (7732)
						return character - 1;
					} else if (character == U'ḷ') { // ḷ (7735)
						return U'Ḷ'; // Ḷ (7734)
						return character - 1;
					} else if (character == U'ḹ') { // ḹ (7737)
						return U'Ḹ'; // Ḹ (7736)
						return character - 1;
					} else if (character == U'ḻ') { // ḻ (7739)
						return U'Ḻ'; // Ḻ (7738)
						return character - 1;
					} else if (character == U'ḽ') { // ḽ (7741)
						return U'Ḽ'; // Ḽ (7740)
						return character - 1;
					} else if (character == U'ḿ') { // ḿ (7743)
						return U'Ḿ'; // Ḿ (7742)
						return character - 1;
					} else if (character == U'ṁ') { // ṁ (7745)
						return U'Ṁ'; // Ṁ (7744)
						return character - 1;
					} else if (character == U'ṃ') { // ṃ (7747)
						return U'Ṃ'; // Ṃ (7746)
						return character - 1;
					} else if (character == U'ṅ') { // ṅ (7749)
						return U'Ṅ'; // Ṅ (7748)
						return character - 1;
					} else if (character == U'ṇ') { // ṇ (7751)
						return U'Ṇ'; // Ṇ (7750)
						return character - 1;
					} else if (character == U'ṉ') { // ṉ (7753)
						return U'Ṉ'; // Ṉ (7752)
						return character - 1;
					} else if (character == U'ṋ') { // ṋ (7755)
						return U'Ṋ'; // Ṋ (7754)
						return character - 1;
					} else if (character == U'ṍ') { // ṍ (7757)
						return U'Ṍ'; // Ṍ (7756)
						return character - 1;
					} else if (character == U'ṏ') { // ṏ (7759)
						return U'Ṏ'; // Ṏ (7758)
						return character - 1;
					} else if (character == U'ṑ') { // ṑ (7761)
						return U'Ṑ'; // Ṑ (7760)
						return character - 1;
					} else if (character == U'ṓ') { // ṓ (7763)
						return U'Ṓ'; // Ṓ (7762)
						return character - 1;
					} else if (character == U'ṕ') { // ṕ (7765)
						return U'Ṕ'; // Ṕ (7764)
						return character - 1;
					} else if (character == U'ṗ') { // ṗ (7767)
						return U'Ṗ'; // Ṗ (7766)
						return character - 1;
					} else if (character == U'ṙ') { // ṙ (7769)
						return U'Ṙ'; // Ṙ (7768)
						return character - 1;
					} else if (character == U'ṛ') { // ṛ (7771)
						return U'Ṛ'; // Ṛ (7770)
						return character - 1;
					} else if (character == U'ṝ') { // ṝ (7773)
						return U'Ṝ'; // Ṝ (7772)
						return character - 1;
					} else if (character == U'ṟ') { // ṟ (7775)
						return U'Ṟ'; // Ṟ (7774)
						return character - 1;
					} else if (character == U'ṡ') { // ṡ (7777)
						return U'Ṡ'; // Ṡ (7776)
						return character - 1;
					} else if (character == U'ṣ') { // ṣ (7779)
						return U'Ṣ'; // Ṣ (7778)
						return character - 1;
					} else if (character == U'ṥ') { // ṥ (7781)
						return U'Ṥ'; // Ṥ (7780)
						return character - 1;
					} else if (character == U'ṧ') { // ṧ (7783)
						return U'Ṧ'; // Ṧ (7782)
						return character - 1;
					} else if (character == U'ṩ') { // ṩ (7785)
						return U'Ṩ'; // Ṩ (7784)
						return character - 1;
					} else if (character == U'ṫ') { // ṫ (7787)
						return U'Ṫ'; // Ṫ (7786)
						return character - 1;
					} else if (character == U'ṭ') { // ṭ (7789)
						return U'Ṭ'; // Ṭ (7788)
						return character - 1;
					} else if (character == U'ṯ') { // ṯ (7791)
						return U'Ṯ'; // Ṯ (7790)
						return character - 1;
					} else if (character == U'ṱ') { // ṱ (7793)
						return U'Ṱ'; // Ṱ (7792)
						return character - 1;
					} else if (character == U'ṳ') { // ṳ (7795)
						return U'Ṳ'; // Ṳ (7794)
						return character - 1;
					} else if (character == U'ṵ') { // ṵ (7797)
						return U'Ṵ'; // Ṵ (7796)
						return character - 1;
					} else if (character == U'ṷ') { // ṷ (7799)
						return U'Ṷ'; // Ṷ (7798)
						return character - 1;
					} else if (character == U'ṹ') { // ṹ (7801)
						return U'Ṹ'; // Ṹ (7800)
						return character - 1;
					} else if (character == U'ṻ') { // ṻ (7803)
						return U'Ṻ'; // Ṻ (7802)
						return character - 1;
					} else if (character == U'ṽ') { // ṽ (7805)
						return U'Ṽ'; // Ṽ (7804)
						return character - 1;
					} else if (character == U'ṿ') { // ṿ (7807)
						return U'Ṿ'; // Ṿ (7806)
						return character - 1;
					} else if (character == U'ẁ') { // ẁ (7809)
						return U'Ẁ'; // Ẁ (7808)
						return character - 1;
					} else if (character == U'ẃ') { // ẃ (7811)
						return U'Ẃ'; // Ẃ (7810)
						return character - 1;
					} else if (character == U'ẅ') { // ẅ (7813)
						return U'Ẅ'; // Ẅ (7812)
						return character - 1;
					} else if (character == U'ẇ') { // ẇ (7815)
						return U'Ẇ'; // Ẇ (7814)
						return character - 1;
					} else if (character == U'ẉ') { // ẉ (7817)
						return U'Ẉ'; // Ẉ (7816)
						return character - 1;
					} else if (character == U'ẋ') { // ẋ (7819)
						return U'Ẋ'; // Ẋ (7818)
						return character - 1;
					} else if (character == U'ẍ') { // ẍ (7821)
						return U'Ẍ'; // Ẍ (7820)
						return character - 1;
					} else if (character == U'ẏ') { // ẏ (7823)
						return U'Ẏ'; // Ẏ (7822)
						return character - 1;
					} else if (character == U'ẑ') { // ẑ (7825)
						return U'Ẑ'; // Ẑ (7824)
						return character - 1;
					} else if (character == U'ẓ') { // ẓ (7827)
						return U'Ẓ'; // Ẓ (7826)
						return character - 1;
					} else if (character == U'ẕ') { // ẕ (7829)
						return U'Ẕ'; // Ẕ (7828)
						return character - 1;
					} else if (character == U'ạ') { // ạ (7841)
						return U'Ạ'; // Ạ (7840)
						return character - 1;
					} else if (character == U'ả') { // ả (7843)
						return U'Ả'; // Ả (7842)
						return character - 1;
					} else if (character == U'ấ') { // ấ (7845)
						return U'Ấ'; // Ấ (7844)
						return character - 1;
					} else if (character == U'ầ') { // ầ (7847)
						return U'Ầ'; // Ầ (7846)
						return character - 1;
					} else if (character == U'ẩ') { // ẩ (7849)
						return U'Ẩ'; // Ẩ (7848)
						return character - 1;
					} else if (character == U'ẫ') { // ẫ (7851)
						return U'Ẫ'; // Ẫ (7850)
						return character - 1;
					} else if (character == U'ậ') { // ậ (7853)
						return U'Ậ'; // Ậ (7852)
						return character - 1;
					} else if (character == U'ắ') { // ắ (7855)
						return U'Ắ'; // Ắ (7854)
						return character - 1;
					} else if (character == U'ằ') { // ằ (7857)
						return U'Ằ'; // Ằ (7856)
						return character - 1;
					} else if (character == U'ẳ') { // ẳ (7859)
						return U'Ẳ'; // Ẳ (7858)
						return character - 1;
					} else if (character == U'ẵ') { // ẵ (7861)
						return U'Ẵ'; // Ẵ (7860)
						return character - 1;
					} else if (character == U'ặ') { // ặ (7863)
						return U'Ặ'; // Ặ (7862)
						return character - 1;
					} else if (character == U'ẹ') { // ẹ (7865)
						return U'Ẹ'; // Ẹ (7864)
						return character - 1;
					} else if (character == U'ẻ') { // ẻ (7867)
						return U'Ẻ'; // Ẻ (7866)
						return character - 1;
					} else if (character == U'ẽ') { // ẽ (7869)
						return U'Ẽ'; // Ẽ (7868)
						return character - 1;
					} else if (character == U'ế') { // ế (7871)
						return U'Ế'; // Ế (7870)
						return character - 1;
					} else if (character == U'ề') { // ề (7873)
						return U'Ề'; // Ề (7872)
						return character - 1;
					} else if (character == U'ể') { // ể (7875)
						return U'Ể'; // Ể (7874)
						return character - 1;
					} else if (character == U'ễ') { // ễ (7877)
						return U'Ễ'; // Ễ (7876)
						return character - 1;
					} else if (character == U'ệ') { // ệ (7879)
						return U'Ệ'; // Ệ (7878)
						return character - 1;
					} else if (character == U'ỉ') { // ỉ (7881)
						return U'Ỉ'; // Ỉ (7880)
						return character - 1;
					} else if (character == U'ị') { // ị (7883)
						return U'Ị'; // Ị (7882)
						return character - 1;
					} else if (character == U'ọ') { // ọ (7885)
						return U'Ọ'; // Ọ (7884)
						return character - 1;
					} else if (character == U'ỏ') { // ỏ (7887)
						return U'Ỏ'; // Ỏ (7886)
						return character - 1;
					} else if (character == U'ố') { // ố (7889)
						return U'Ố'; // Ố (7888)
						return character - 1;
					} else if (character == U'ồ') { // ồ (7891)
						return U'Ồ'; // Ồ (7890)
						return character - 1;
					} else if (character == U'ổ') { // ổ (7893)
						return U'Ổ'; // Ổ (7892)
						return character - 1;
					} else if (character == U'ỗ') { // ỗ (7895)
						return U'Ỗ'; // Ỗ (7894)
						return character - 1;
					} else if (character == U'ộ') { // ộ (7897)
						return U'Ộ'; // Ộ (7896)
						return character - 1;
					} else if (character == U'ớ') { // ớ (7899)
						return U'Ớ'; // Ớ (7898)
						return character - 1;
					} else if (character == U'ờ') { // ờ (7901)
						return U'Ờ'; // Ờ (7900)
						return character - 1;
					} else if (character == U'ở') { // ở (7903)
						return U'Ở'; // Ở (7902)
						return character - 1;
					} else if (character == U'ỡ') { // ỡ (7905)
						return U'Ỡ'; // Ỡ (7904)
						return character - 1;
					} else if (character == U'ợ') { // ợ (7907)
						return U'Ợ'; // Ợ (7906)
						return character - 1;
					} else if (character == U'ụ') { // ụ (7909)
						return U'Ụ'; // Ụ (7908)
						return character - 1;
					} else if (character == U'ủ') { // ủ (7911)
						return U'Ủ'; // Ủ (7910)
						return character - 1;
					} else if (character == U'ứ') { // ứ (7913)
						return U'Ứ'; // Ứ (7912)
						return character - 1;
					} else if (character == U'ừ') { // ừ (7915)
						return U'Ừ'; // Ừ (7914)
						return character - 1;
					} else if (character == U'ử') { // ử (7917)
						return U'Ử'; // Ử (7916)
						return character - 1;
					} else if (character == U'ữ') { // ữ (7919)
						return U'Ữ'; // Ữ (7918)
						return character - 1;
					} else if (character == U'ự') { // ự (7921)
						return U'Ự'; // Ự (7920)
						return character - 1;
					} else if (character == U'ỳ') { // ỳ (7923)
						return U'Ỳ'; // Ỳ (7922)
						return character - 1;
					} else if (character == U'ỵ') { // ỵ (7925)
						return U'Ỵ'; // Ỵ (7924)
						return character - 1;
					} else if (character == U'ỷ') { // ỷ (7927)
						return U'Ỷ'; // Ỷ (7926)
						return character - 1;
					} else if (character == U'ỹ') { // ỹ (7929)
						return U'Ỹ'; // Ỹ (7928)
						return character - 1;
					} else if (character == U'ἀ') { // ἀ (7936)
						return U'Ἀ'; // Ἀ (7944)
						return character + 8;
					} else if (character == U'ἁ') { // ἁ (7937)
						return U'Ἁ'; // Ἁ (7945)
						return character + 8;
					} else if (character == U'ἂ') { // ἂ (7938)
						return U'Ἂ'; // Ἂ (7946)
						return character + 8;
					} else if (character == U'ἃ') { // ἃ (7939)
						return U'Ἃ'; // Ἃ (7947)
						return character + 8;
					} else if (character == U'ἄ') { // ἄ (7940)
						return U'Ἄ'; // Ἄ (7948)
						return character + 8;
					} else if (character == U'ἅ') { // ἅ (7941)
						return U'Ἅ'; // Ἅ (7949)
						return character + 8;
					} else if (character == U'ἆ') { // ἆ (7942)
						return U'Ἆ'; // Ἆ (7950)
						return character + 8;
					} else if (character == U'ἇ') { // ἇ (7943)
						return U'Ἇ'; // Ἇ (7951)
						return character + 8;
					} else if (character == U'ἐ') { // ἐ (7952)
						return U'Ἐ'; // Ἐ (7960)
						return character + 8;
					} else if (character == U'ἑ') { // ἑ (7953)
						return U'Ἑ'; // Ἑ (7961)
						return character + 8;
					} else if (character == U'ἒ') { // ἒ (7954)
						return U'Ἒ'; // Ἒ (7962)
						return character + 8;
					} else if (character == U'ἓ') { // ἓ (7955)
						return U'Ἓ'; // Ἓ (7963)
						return character + 8;
					} else if (character == U'ἔ') { // ἔ (7956)
						return U'Ἔ'; // Ἔ (7964)
						return character + 8;
					} else if (character == U'ἕ') { // ἕ (7957)
						return U'Ἕ'; // Ἕ (7965)
						return character + 8;
					} else if (character == U'ἠ') { // ἠ (7968)
						return U'Ἠ'; // Ἠ (7976)
						return character + 8;
					} else if (character == U'ἡ') { // ἡ (7969)
						return U'Ἡ'; // Ἡ (7977)
						return character + 8;
					} else if (character == U'ἢ') { // ἢ (7970)
						return U'Ἢ'; // Ἢ (7978)
						return character + 8;
					} else if (character == U'ἣ') { // ἣ (7971)
						return U'Ἣ'; // Ἣ (7979)
						return character + 8;
					} else if (character == U'ἤ') { // ἤ (7972)
						return U'Ἤ'; // Ἤ (7980)
						return character + 8;
					} else if (character == U'ἥ') { // ἥ (7973)
						return U'Ἥ'; // Ἥ (7981)
						return character + 8;
					} else if (character == U'ἦ') { // ἦ (7974)
						return U'Ἦ'; // Ἦ (7982)
						return character + 8;
					} else if (character == U'ἧ') { // ἧ (7975)
						return U'Ἧ'; // Ἧ (7983)
						return character + 8;
					} else if (character == U'ἰ') { // ἰ (7984)
						return U'Ἰ'; // Ἰ (7992)
						return character + 8;
					} else if (character == U'ἱ') { // ἱ (7985)
						return U'Ἱ'; // Ἱ (7993)
						return character + 8;
					} else if (character == U'ἲ') { // ἲ (7986)
						return U'Ἲ'; // Ἲ (7994)
						return character + 8;
					} else if (character == U'ἳ') { // ἳ (7987)
						return U'Ἳ'; // Ἳ (7995)
						return character + 8;
					} else if (character == U'ἴ') { // ἴ (7988)
						return U'Ἴ'; // Ἴ (7996)
						return character + 8;
					} else if (character == U'ἵ') { // ἵ (7989)
						return U'Ἵ'; // Ἵ (7997)
						return character + 8;
					} else if (character == U'ἶ') { // ἶ (7990)
						return U'Ἶ'; // Ἶ (7998)
						return character + 8;
					} else if (character == U'ἷ') { // ἷ (7991)
						return U'Ἷ'; // Ἷ (7999)
						return character + 8;
					} else if (character == U'ὀ') { // ὀ (8000)
						return U'Ὀ'; // Ὀ (8008)
						return character + 8;
					} else if (character == U'ὁ') { // ὁ (8001)
						return U'Ὁ'; // Ὁ (8009)
						return character + 8;
					} else if (character == U'ὂ') { // ὂ (8002)
						return U'Ὂ'; // Ὂ (8010)
						return character + 8;
					} else if (character == U'ὃ') { // ὃ (8003)
						return U'Ὃ'; // Ὃ (8011)
						return character + 8;
					} else if (character == U'ὄ') { // ὄ (8004)
						return U'Ὄ'; // Ὄ (8012)
						return character + 8;
					} else if (character == U'ὅ') { // ὅ (8005)
						return U'Ὅ'; // Ὅ (8013)
						return character + 8;
					} else if (character == U'ὑ') { // ὑ (8017)
						return U'Ὑ'; // Ὑ (8025)
						return character + 8;
					} else if (character == U'ὓ') { // ὓ (8019)
						return U'Ὓ'; // Ὓ (8027)
						return character + 8;
					} else if (character == U'ὕ') { // ὕ (8021)
						return U'Ὕ'; // Ὕ (8029)
						return character + 8;
					} else if (character == U'ὗ') { // ὗ (8023)
						return U'Ὗ'; // Ὗ (8031)
						return character + 8;
					} else if (character == U'ὠ') { // ὠ (8032)
						return U'Ὠ'; // Ὠ (8040)
						return character + 8;
					} else if (character == U'ὡ') { // ὡ (8033)
						return U'Ὡ'; // Ὡ (8041)
						return character + 8;
					} else if (character == U'ὢ') { // ὢ (8034)
						return U'Ὢ'; // Ὢ (8042)
						return character + 8;
					} else if (character == U'ὣ') { // ὣ (8035)
						return U'Ὣ'; // Ὣ (8043)
						return character + 8;
					} else if (character == U'ὤ') { // ὤ (8036)
						return U'Ὤ'; // Ὤ (8044)
						return character + 8;
					} else if (character == U'ὥ') { // ὥ (8037)
						return U'Ὥ'; // Ὥ (8045)
						return character + 8;
					} else if (character == U'ὦ') { // ὦ (8038)
						return U'Ὦ'; // Ὦ (8046)
						return character + 8;
					} else if (character == U'ὧ') { // ὧ (8039)
						return U'Ὧ'; // Ὧ (8047)
						return character + 8;
					} else if (character == U'ᾀ') { // ᾀ (8064)
						return U'ᾈ'; // ᾈ (8072)
						return character + 8;
					} else if (character == U'ᾁ') { // ᾁ (8065)
						return U'ᾉ'; // ᾉ (8073)
						return character + 8;
					} else if (character == U'ᾂ') { // ᾂ (8066)
						return U'ᾊ'; // ᾊ (8074)
						return character + 8;
					} else if (character == U'ᾃ') { // ᾃ (8067)
						return U'ᾋ'; // ᾋ (8075)
						return character + 8;
					} else if (character == U'ᾄ') { // ᾄ (8068)
						return U'ᾌ'; // ᾌ (8076)
						return character + 8;
					} else if (character == U'ᾅ') { // ᾅ (8069)
						return U'ᾍ'; // ᾍ (8077)
						return character + 8;
					} else if (character == U'ᾆ') { // ᾆ (8070)
						return U'ᾎ'; // ᾎ (8078)
						return character + 8;
					} else if (character == U'ᾇ') { // ᾇ (8071)
						return U'ᾏ'; // ᾏ (8079)
						return character + 8;
					} else if (character == U'ᾐ') { // ᾐ (8080)
						return U'ᾘ'; // ᾘ (8088)
						return character + 8;
					} else if (character == U'ᾑ') { // ᾑ (8081)
						return U'ᾙ'; // ᾙ (8089)
						return character + 8;
					} else if (character == U'ᾒ') { // ᾒ (8082)
						return U'ᾚ'; // ᾚ (8090)
						return character + 8;
					} else if (character == U'ᾓ') { // ᾓ (8083)
						return U'ᾛ'; // ᾛ (8091)
						return character + 8;
					} else if (character == U'ᾔ') { // ᾔ (8084)
						return U'ᾜ'; // ᾜ (8092)
						return character + 8;
					} else if (character == U'ᾕ') { // ᾕ (8085)
						return U'ᾝ'; // ᾝ (8093)
						return character + 8;
					} else if (character == U'ᾖ') { // ᾖ (8086)
						return U'ᾞ'; // ᾞ (8094)
						return character + 8;
					} else if (character == U'ᾗ') { // ᾗ (8087)
						return U'ᾟ'; // ᾟ (8095)
						return character + 8;
					} else if (character == U'ᾠ') { // ᾠ (8096)
						return U'ᾨ'; // ᾨ (8104)
						return character + 8;
					} else if (character == U'ᾡ') { // ᾡ (8097)
						return U'ᾩ'; // ᾩ (8105)
						return character + 8;
					} else if (character == U'ᾢ') { // ᾢ (8098)
						return U'ᾪ'; // ᾪ (8106)
						return character + 8;
					} else if (character == U'ᾣ') { // ᾣ (8099)
						return U'ᾫ'; // ᾫ (8107)
						return character + 8;
					} else if (character == U'ᾤ') { // ᾤ (8100)
						return U'ᾬ'; // ᾬ (8108)
						return character + 8;
					} else if (character == U'ᾥ') { // ᾥ (8101)
						return U'ᾭ'; // ᾭ (8109)
						return character + 8;
					} else if (character == U'ᾦ') { // ᾦ (8102)
						return U'ᾮ'; // ᾮ (8110)
						return character + 8;
					} else if (character == U'ᾧ') { // ᾧ (8103)
						return U'ᾯ'; // ᾯ (8111)
						return character + 8;
					} else if (character == U'ᾰ') { // ᾰ (8112)
						return U'Ᾰ'; // Ᾰ (8120)
						return character + 8;
					} else if (character == U'ᾱ') { // ᾱ (8113)
						return U'Ᾱ'; // Ᾱ (8121)
						return character + 8;
					} else if (character == U'ῐ') { // ῐ (8144)
						return U'Ῐ'; // Ῐ (8152)
						return character + 8;
					} else if (character == U'ῑ') { // ῑ (8145)
						return U'Ῑ'; // Ῑ (8153)
						return character + 8;
					} else if (character == U'ῠ') { // ῠ (8160)
						return U'Ῠ'; // Ῠ (8168)
						return character + 8;
					} else if (character == U'ῡ') { // ῡ (8161)
						return U'Ῡ'; // Ῡ (8169)
						return character + 8;
					} else if (character == U'ⓐ') { // ⓐ (9424)
						return U'Ⓐ'; // Ⓐ (9398)
						return character - 26;
					} else if (character == U'ⓑ') { // ⓑ (9425)
						return U'Ⓑ'; // Ⓑ (9399)
						return character - 26;
					} else if (character == U'ⓒ') { // ⓒ (9426)
						return U'Ⓒ'; // Ⓒ (9400)
						return character - 26;
					} else if (character == U'ⓓ') { // ⓓ (9427)
						return U'Ⓓ'; // Ⓓ (9401)
						return character - 26;
					} else if (character == U'ⓔ') { // ⓔ (9428)
						return U'Ⓔ'; // Ⓔ (9402)
						return character - 26;
					} else if (character == U'ⓕ') { // ⓕ (9429)
						return U'Ⓕ'; // Ⓕ (9403)
						return character - 26;
					} else if (character == U'ⓖ') { // ⓖ (9430)
						return U'Ⓖ'; // Ⓖ (9404)
						return character - 26;
					} else if (character == U'ⓗ') { // ⓗ (9431)
						return U'Ⓗ'; // Ⓗ (9405)
						return character - 26;
					} else if (character == U'ⓘ') { // ⓘ (9432)
						return U'Ⓘ'; // Ⓘ (9406)
						return character - 26;
					} else if (character == U'ⓙ') { // ⓙ (9433)
						return U'Ⓙ'; // Ⓙ (9407)
						return character - 26;
					} else if (character == U'ⓚ') { // ⓚ (9434)
						return U'Ⓚ'; // Ⓚ (9408)
						return character - 26;
					} else if (character == U'ⓛ') { // ⓛ (9435)
						return U'Ⓛ'; // Ⓛ (9409)
						return character - 26;
					} else if (character == U'ⓜ') { // ⓜ (9436)
						return U'Ⓜ'; // Ⓜ (9410)
						return character - 26;
					} else if (character == U'ⓝ') { // ⓝ (9437)
						return U'Ⓝ'; // Ⓝ (9411)
						return character - 26;
					} else if (character == U'ⓞ') { // ⓞ (9438)
						return U'Ⓞ'; // Ⓞ (9412)
						return character - 26;
					} else if (character == U'ⓟ') { // ⓟ (9439)
						return U'Ⓟ'; // Ⓟ (9413)
						return character - 26;
					} else if (character == U'ⓠ') { // ⓠ (9440)
						return U'Ⓠ'; // Ⓠ (9414)
						return character - 26;
					} else if (character == U'ⓡ') { // ⓡ (9441)
						return U'Ⓡ'; // Ⓡ (9415)
						return character - 26;
					} else if (character == U'ⓢ') { // ⓢ (9442)
						return U'Ⓢ'; // Ⓢ (9416)
						return character - 26;
					} else if (character == U'ⓣ') { // ⓣ (9443)
						return U'Ⓣ'; // Ⓣ (9417)
						return character - 26;
					} else if (character == U'ⓤ') { // ⓤ (9444)
						return U'Ⓤ'; // Ⓤ (9418)
						return character - 26;
					} else if (character == U'ⓥ') { // ⓥ (9445)
						return U'Ⓥ'; // Ⓥ (9419)
						return character - 26;
					} else if (character == U'ⓦ') { // ⓦ (9446)
						return U'Ⓦ'; // Ⓦ (9420)
						return character - 26;
					} else if (character == U'ⓧ') { // ⓧ (9447)
						return U'Ⓧ'; // Ⓧ (9421)
						return character - 26;
					} else if (character == U'ⓨ') { // ⓨ (9448)
						return U'Ⓨ'; // Ⓨ (9422)
						return character - 26;
					} else if (character == U'ⓩ') { // ⓩ (9449)
						return U'Ⓩ'; // Ⓩ (9423)
						return character - 26;
					} else if (character == U'ａ') { // ａ (65345)
						return U'Ａ'; // Ａ (65313)
						return character - 32;
					} else if (character == U'ｂ') { // ｂ (65346)
						return U'Ｂ'; // Ｂ (65314)
						return character - 32;
					} else if (character == U'ｃ') { // ｃ (65347)
						return U'Ｃ'; // Ｃ (65315)
						return character - 32;
					} else if (character == U'ｄ') { // ｄ (65348)
						return U'Ｄ'; // Ｄ (65316)
						return character - 32;
					} else if (character == U'ｅ') { // ｅ (65349)
						return U'Ｅ'; // Ｅ (65317)
						return character - 32;
					} else if (character == U'ｆ') { // ｆ (65350)
						return U'Ｆ'; // Ｆ (65318)
						return character - 32;
					} else if (character == U'ｇ') { // ｇ (65351)
						return U'Ｇ'; // Ｇ (65319)
						return character - 32;
					} else if (character == U'ｈ') { // ｈ (65352)
						return U'Ｈ'; // Ｈ (65320)
						return character - 32;
					} else if (character == U'ｉ') { // ｉ (65353)
						return U'Ｉ'; // Ｉ (65321)
						return character - 32;
					} else if (character == U'ｊ') { // ｊ (65354)
						return U'Ｊ'; // Ｊ (65322)
						return character - 32;
					} else if (character == U'ｋ') { // ｋ (65355)
						return U'Ｋ'; // Ｋ (65323)
						return character - 32;
					} else if (character == U'ｌ') { // ｌ (65356)
						return U'Ｌ'; // Ｌ (65324)
						return character - 32;
					} else if (character == U'ｍ') { // ｍ (65357)
						return U'Ｍ'; // Ｍ (65325)
						return character - 32;
					} else if (character == U'ｎ') { // ｎ (65358)
						return U'Ｎ'; // Ｎ (65326)
						return character - 32;
					} else if (character == U'ｏ') { // ｏ (65359)
						return U'Ｏ'; // Ｏ (65327)
						return character - 32;
					} else if (character == U'ｐ') { // ｐ (65360)
						return U'Ｐ'; // Ｐ (65328)
						return character - 32;
					} else if (character == U'ｑ') { // ｑ (65361)
						return U'Ｑ'; // Ｑ (65329)
						return character - 32;
					} else if (character == U'ｒ') { // ｒ (65362)
						return U'Ｒ'; // Ｒ (65330)
						return character - 32;
					} else if (character == U'ｓ') { // ｓ (65363)
						return U'Ｓ'; // Ｓ (65331)
						return character - 32;
					} else if (character == U'ｔ') { // ｔ (65364)
						return U'Ｔ'; // Ｔ (65332)
						return character - 32;
					} else if (character == U'ｕ') { // ｕ (65365)
						return U'Ｕ'; // Ｕ (65333)
						return character - 32;
					} else if (character == U'ｖ') { // ｖ (65366)
						return U'Ｖ'; // Ｖ (65334)
						return character - 32;
					} else if (character == U'ｗ') { // ｗ (65367)
						return U'Ｗ'; // Ｗ (65335)
						return character - 32;
					} else if (character == U'ｘ') { // ｘ (65368)
						return U'Ｘ'; // Ｘ (65336)
						return character - 32;
					} else if (character == U'ｙ') { // ｙ (65369)
						return U'Ｙ'; // Ｙ (65337)
						return character - 32;
					} else if (character == U'ｚ') { // ｚ (65370)
						return U'Ｚ'; // Ｚ (65338)
						return character - 32;
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

						// TODO: Compress into ranges.
						} else if (character == U'Ɂ') { // Ɂ (577)
							return U'ɂ'; // ɂ (578)
							return character + 1;
						} else if (character == U'Ƀ') { // Ƀ (579)
							return U'ƀ'; // ƀ (384)
							return character + 195;
						} else if (character == U'Ʉ') { // Ʉ (580)
							return U'ʉ'; // ʉ (649)
							return character + 69;
						} else if (character == U'Ʌ') { // Ʌ (581)
							return U'ʌ'; // ʌ (652)
							return character + 71;
						} else if (character == U'Ɇ') { // Ɇ (582)
							return U'ɇ'; // ɇ (583)
							return character + 1;
						} else if (character == U'Ɉ') { // Ɉ (584)
							return U'ɉ'; // ɉ (585)
							return character + 1;
						} else if (character == U'Ɋ') { // Ɋ (586)
							return U'ɋ'; // ɋ (587)
							return character + 1;
						} else if (character == U'Ɍ') { // Ɍ (588)
							return U'ɍ'; // ɍ (589)
							return character + 1;
						} else if (character == U'Ɏ') { // Ɏ (590)
							return U'ɏ'; // ɏ (591)
							return character + 1;
						} else if (character == U'ʃ') { // ʃ (643)
							return U'Ʃ'; // Ʃ (425)
							return character + 218;
						} else if (character == U'ˀ') { // ˀ (704)
							return U'ʔ'; // ʔ (660)
							return character + 44;
						} else if (character == U'Ά') { // Ά (902)
							return U'ά'; // ά (940)
							return character + 38;
						} else if (character == U'Έ') { // Έ (904)
							return U'έ'; // έ (941)
							return character + 37;
						} else if (character == U'Ή') { // Ή (905)
							return U'ή'; // ή (942)
							return character + 37;
						} else if (character == U'Ί') { // Ί (906)
							return U'ί'; // ί (943)
							return character + 37;
						} else if (character == U'Ό') { // Ό (908)
							return U'ό'; // ό (972)
							return character + 64;
						} else if (character == U'Ύ') { // Ύ (910)
							return U'ύ'; // ύ (973)
							return character + 63;
						} else if (character == U'Ώ') { // Ώ (911)
							return U'ώ'; // ώ (974)
							return character + 63;
						} else if (character == U'Α') { // Α (913)
							return U'α'; // α (945)
							return character + 32;
						} else if (character == U'Β') { // Β (914)
							return U'β'; // β (946)
							return character + 32;
						} else if (character == U'Γ') { // Γ (915)
							return U'γ'; // γ (947)
							return character + 32;
						} else if (character == U'Δ') { // Δ (916)
							return U'δ'; // δ (948)
							return character + 32;
						} else if (character == U'Ε') { // Ε (917)
							return U'ε'; // ε (949)
							return character + 32;
						} else if (character == U'Ζ') { // Ζ (918)
							return U'ζ'; // ζ (950)
							return character + 32;
						} else if (character == U'Η') { // Η (919)
							return U'η'; // η (951)
							return character + 32;
						} else if (character == U'Θ') { // Θ (920)
							return U'θ'; // θ (952)
							return character + 32;
						} else if (character == U'Ι') { // Ι (921)
							return U'ι'; // ι (953)
							return character + 32;
						} else if (character == U'Κ') { // Κ (922)
							return U'κ'; // κ (954)
							return character + 32;
						} else if (character == U'Λ') { // Λ (923)
							return U'λ'; // λ (955)
							return character + 32;
						} else if (character == U'Μ') { // Μ (924)
							return U'μ'; // μ (956)
							return character + 32;
						} else if (character == U'Ν') { // Ν (925)
							return U'ν'; // ν (957)
							return character + 32;
						} else if (character == U'Ξ') { // Ξ (926)
							return U'ξ'; // ξ (958)
							return character + 32;
						} else if (character == U'Ο') { // Ο (927)
							return U'ο'; // ο (959)
							return character + 32;
						} else if (character == U'Π') { // Π (928)
							return U'π'; // π (960)
							return character + 32;
						} else if (character == U'Ρ') { // Ρ (929)
							return U'ρ'; // ρ (961)
							return character + 32;
						} else if (character == U'Σ') { // Σ (931)
							return U'σ'; // σ (963)
							return character + 32;
						} else if (character == U'Τ') { // Τ (932)
							return U'τ'; // τ (964)
							return character + 32;
						} else if (character == U'Υ') { // Υ (933)
							return U'υ'; // υ (965)
							return character + 32;
						} else if (character == U'Φ') { // Φ (934)
							return U'φ'; // φ (966)
							return character + 32;
						} else if (character == U'Χ') { // Χ (935)
							return U'χ'; // χ (967)
							return character + 32;
						} else if (character == U'Ψ') { // Ψ (936)
							return U'ψ'; // ψ (968)
							return character + 32;
						} else if (character == U'Ω') { // Ω (937)
							return U'ω'; // ω (969)
							return character + 32;
						} else if (character == U'Ϊ') { // Ϊ (938)
							return U'ϊ'; // ϊ (970)
							return character + 32;
						} else if (character == U'Ϋ') { // Ϋ (939)
							return U'ϋ'; // ϋ (971)
							return character + 32;
						} else if (character == U'Ϣ') { // Ϣ (994)
							return U'ϣ'; // ϣ (995)
							return character + 1;
						} else if (character == U'Ϥ') { // Ϥ (996)
							return U'ϥ'; // ϥ (997)
							return character + 1;
						} else if (character == U'Ϧ') { // Ϧ (998)
							return U'ϧ'; // ϧ (999)
							return character + 1;
						} else if (character == U'Ϩ') { // Ϩ (1000)
							return U'ϩ'; // ϩ (1001)
							return character + 1;
						} else if (character == U'Ϫ') { // Ϫ (1002)
							return U'ϫ'; // ϫ (1003)
							return character + 1;
						} else if (character == U'Ϭ') { // Ϭ (1004)
							return U'ϭ'; // ϭ (1005)
							return character + 1;
						} else if (character == U'Ϯ') { // Ϯ (1006)
							return U'ϯ'; // ϯ (1007)
							return character + 1;
						} else if (character == U'Ё') { // Ё (1025)
							return U'ё'; // ё (1105)
							return character + 80;
						} else if (character == U'Ђ') { // Ђ (1026)
							return U'ђ'; // ђ (1106)
							return character + 80;
						} else if (character == U'Ѓ') { // Ѓ (1027)
							return U'ѓ'; // ѓ (1107)
							return character + 80;
						} else if (character == U'Є') { // Є (1028)
							return U'є'; // є (1108)
							return character + 80;
						} else if (character == U'Ѕ') { // Ѕ (1029)
							return U'ѕ'; // ѕ (1109)
							return character + 80;
						} else if (character == U'І') { // І (1030)
							return U'і'; // і (1110)
							return character + 80;
						} else if (character == U'Ї') { // Ї (1031)
							return U'ї'; // ї (1111)
							return character + 80;
						} else if (character == U'Ј') { // Ј (1032)
							return U'ј'; // ј (1112)
							return character + 80;
						} else if (character == U'Љ') { // Љ (1033)
							return U'љ'; // љ (1113)
							return character + 80;
						} else if (character == U'Њ') { // Њ (1034)
							return U'њ'; // њ (1114)
							return character + 80;
						} else if (character == U'Ћ') { // Ћ (1035)
							return U'ћ'; // ћ (1115)
							return character + 80;
						} else if (character == U'Ќ') { // Ќ (1036)
							return U'ќ'; // ќ (1116)
							return character + 80;
						} else if (character == U'Ў') { // Ў (1038)
							return U'ў'; // ў (1118)
							return character + 80;
						} else if (character == U'Џ') { // Џ (1039)
							return U'џ'; // џ (1119)
							return character + 80;
						} else if (character == U'А') { // А (1040)
							return U'а'; // а (1072)
							return character + 32;
						} else if (character == U'Б') { // Б (1041)
							return U'б'; // б (1073)
							return character + 32;
						} else if (character == U'В') { // В (1042)
							return U'в'; // в (1074)
							return character + 32;
						} else if (character == U'Г') { // Г (1043)
							return U'г'; // г (1075)
							return character + 32;
						} else if (character == U'Д') { // Д (1044)
							return U'д'; // д (1076)
							return character + 32;
						} else if (character == U'Е') { // Е (1045)
							return U'е'; // е (1077)
							return character + 32;
						} else if (character == U'Ж') { // Ж (1046)
							return U'ж'; // ж (1078)
							return character + 32;
						} else if (character == U'З') { // З (1047)
							return U'з'; // з (1079)
							return character + 32;
						} else if (character == U'И') { // И (1048)
							return U'и'; // и (1080)
							return character + 32;
						} else if (character == U'Й') { // Й (1049)
							return U'й'; // й (1081)
							return character + 32;
						} else if (character == U'К') { // К (1050)
							return U'к'; // к (1082)
							return character + 32;
						} else if (character == U'Л') { // Л (1051)
							return U'л'; // л (1083)
							return character + 32;
						} else if (character == U'М') { // М (1052)
							return U'м'; // м (1084)
							return character + 32;
						} else if (character == U'Н') { // Н (1053)
							return U'н'; // н (1085)
							return character + 32;
						} else if (character == U'О') { // О (1054)
							return U'о'; // о (1086)
							return character + 32;
						} else if (character == U'П') { // П (1055)
							return U'п'; // п (1087)
							return character + 32;
						} else if (character == U'Р') { // Р (1056)
							return U'р'; // р (1088)
							return character + 32;
						} else if (character == U'С') { // С (1057)
							return U'с'; // с (1089)
							return character + 32;
						} else if (character == U'Т') { // Т (1058)
							return U'т'; // т (1090)
							return character + 32;
						} else if (character == U'У') { // У (1059)
							return U'у'; // у (1091)
							return character + 32;
						} else if (character == U'Ф') { // Ф (1060)
							return U'ф'; // ф (1092)
							return character + 32;
						} else if (character == U'Х') { // Х (1061)
							return U'х'; // х (1093)
							return character + 32;
						} else if (character == U'Ц') { // Ц (1062)
							return U'ц'; // ц (1094)
							return character + 32;
						} else if (character == U'Ч') { // Ч (1063)
							return U'ч'; // ч (1095)
							return character + 32;
						} else if (character == U'Ш') { // Ш (1064)
							return U'ш'; // ш (1096)
							return character + 32;
						} else if (character == U'Щ') { // Щ (1065)
							return U'щ'; // щ (1097)
							return character + 32;
						} else if (character == U'Ъ') { // Ъ (1066)
							return U'ъ'; // ъ (1098)
							return character + 32;
						} else if (character == U'Ы') { // Ы (1067)
							return U'ы'; // ы (1099)
							return character + 32;
						} else if (character == U'Ь') { // Ь (1068)
							return U'ь'; // ь (1100)
							return character + 32;
						} else if (character == U'Э') { // Э (1069)
							return U'э'; // э (1101)
							return character + 32;
						} else if (character == U'Ю') { // Ю (1070)
							return U'ю'; // ю (1102)
							return character + 32;
						} else if (character == U'Я') { // Я (1071)
							return U'я'; // я (1103)
							return character + 32;
						} else if (character == U'Ѡ') { // Ѡ (1120)
							return U'ѡ'; // ѡ (1121)
							return character + 1;
						} else if (character == U'Ѣ') { // Ѣ (1122)
							return U'ѣ'; // ѣ (1123)
							return character + 1;
						} else if (character == U'Ѥ') { // Ѥ (1124)
							return U'ѥ'; // ѥ (1125)
							return character + 1;
						} else if (character == U'Ѧ') { // Ѧ (1126)
							return U'ѧ'; // ѧ (1127)
							return character + 1;
						} else if (character == U'Ѩ') { // Ѩ (1128)
							return U'ѩ'; // ѩ (1129)
							return character + 1;
						} else if (character == U'Ѫ') { // Ѫ (1130)
							return U'ѫ'; // ѫ (1131)
							return character + 1;
						} else if (character == U'Ѭ') { // Ѭ (1132)
							return U'ѭ'; // ѭ (1133)
							return character + 1;
						} else if (character == U'Ѯ') { // Ѯ (1134)
							return U'ѯ'; // ѯ (1135)
							return character + 1;
						} else if (character == U'Ѱ') { // Ѱ (1136)
							return U'ѱ'; // ѱ (1137)
							return character + 1;
						} else if (character == U'Ѳ') { // Ѳ (1138)
							return U'ѳ'; // ѳ (1139)
							return character + 1;
						} else if (character == U'Ѵ') { // Ѵ (1140)
							return U'ѵ'; // ѵ (1141)
							return character + 1;
						} else if (character == U'Ѷ') { // Ѷ (1142)
							return U'ѷ'; // ѷ (1143)
							return character + 1;
						} else if (character == U'Ѹ') { // Ѹ (1144)
							return U'ѹ'; // ѹ (1145)
							return character + 1;
						} else if (character == U'Ѻ') { // Ѻ (1146)
							return U'ѻ'; // ѻ (1147)
							return character + 1;
						} else if (character == U'Ѽ') { // Ѽ (1148)
							return U'ѽ'; // ѽ (1149)
							return character + 1;
						} else if (character == U'Ѿ') { // Ѿ (1150)
							return U'ѿ'; // ѿ (1151)
							return character + 1;
						} else if (character == U'Ҁ') { // Ҁ (1152)
							return U'ҁ'; // ҁ (1153)
							return character + 1;
						} else if (character == U'Ґ') { // Ґ (1168)
							return U'ґ'; // ґ (1169)
							return character + 1;
						} else if (character == U'Ғ') { // Ғ (1170)
							return U'ғ'; // ғ (1171)
							return character + 1;
						} else if (character == U'Ҕ') { // Ҕ (1172)
							return U'ҕ'; // ҕ (1173)
							return character + 1;
						} else if (character == U'Җ') { // Җ (1174)
							return U'җ'; // җ (1175)
							return character + 1;
						} else if (character == U'Ҙ') { // Ҙ (1176)
							return U'ҙ'; // ҙ (1177)
							return character + 1;
						} else if (character == U'Қ') { // Қ (1178)
							return U'қ'; // қ (1179)
							return character + 1;
						} else if (character == U'Ҝ') { // Ҝ (1180)
							return U'ҝ'; // ҝ (1181)
							return character + 1;
						} else if (character == U'Ҟ') { // Ҟ (1182)
							return U'ҟ'; // ҟ (1183)
							return character + 1;
						} else if (character == U'Ҡ') { // Ҡ (1184)
							return U'ҡ'; // ҡ (1185)
							return character + 1;
						} else if (character == U'Ң') { // Ң (1186)
							return U'ң'; // ң (1187)
							return character + 1;
						} else if (character == U'Ҥ') { // Ҥ (1188)
							return U'ҥ'; // ҥ (1189)
							return character + 1;
						} else if (character == U'Ҧ') { // Ҧ (1190)
							return U'ҧ'; // ҧ (1191)
							return character + 1;
						} else if (character == U'Ҩ') { // Ҩ (1192)
							return U'ҩ'; // ҩ (1193)
							return character + 1;
						} else if (character == U'Ҫ') { // Ҫ (1194)
							return U'ҫ'; // ҫ (1195)
							return character + 1;
						} else if (character == U'Ҭ') { // Ҭ (1196)
							return U'ҭ'; // ҭ (1197)
							return character + 1;
						} else if (character == U'Ү') { // Ү (1198)
							return U'ү'; // ү (1199)
							return character + 1;
						} else if (character == U'Ұ') { // Ұ (1200)
							return U'ұ'; // ұ (1201)
							return character + 1;
						} else if (character == U'Ҳ') { // Ҳ (1202)
							return U'ҳ'; // ҳ (1203)
							return character + 1;
						} else if (character == U'Ҵ') { // Ҵ (1204)
							return U'ҵ'; // ҵ (1205)
							return character + 1;
						} else if (character == U'Ҷ') { // Ҷ (1206)
							return U'ҷ'; // ҷ (1207)
							return character + 1;
						} else if (character == U'Ҹ') { // Ҹ (1208)
							return U'ҹ'; // ҹ (1209)
							return character + 1;
						} else if (character == U'Һ') { // Һ (1210)
							return U'һ'; // һ (1211)
							return character + 1;
						} else if (character == U'Ҽ') { // Ҽ (1212)
							return U'ҽ'; // ҽ (1213)
							return character + 1;
						} else if (character == U'Ҿ') { // Ҿ (1214)
							return U'ҿ'; // ҿ (1215)
							return character + 1;
						} else if (character == U'Ӂ') { // Ӂ (1217)
							return U'ӂ'; // ӂ (1218)
							return character + 1;
						} else if (character == U'Ӄ') { // Ӄ (1219)
							return U'ӄ'; // ӄ (1220)
							return character + 1;
						} else if (character == U'Ӈ') { // Ӈ (1223)
							return U'ӈ'; // ӈ (1224)
							return character + 1;
						} else if (character == U'Ӌ') { // Ӌ (1227)
							return U'ӌ'; // ӌ (1228)
							return character + 1;
						} else if (character == U'Ӑ') { // Ӑ (1232)
							return U'ӑ'; // ӑ (1233)
							return character + 1;
						} else if (character == U'Ӓ') { // Ӓ (1234)
							return U'ӓ'; // ӓ (1235)
							return character + 1;
						} else if (character == U'Ӕ') { // Ӕ (1236)
							return U'ӕ'; // ӕ (1237)
							return character + 1;
						} else if (character == U'Ӗ') { // Ӗ (1238)
							return U'ӗ'; // ӗ (1239)
							return character + 1;
						} else if (character == U'Ә') { // Ә (1240)
							return U'ә'; // ә (1241)
							return character + 1;
						} else if (character == U'Ӛ') { // Ӛ (1242)
							return U'ӛ'; // ӛ (1243)
							return character + 1;
						} else if (character == U'Ӝ') { // Ӝ (1244)
							return U'ӝ'; // ӝ (1245)
							return character + 1;
						} else if (character == U'Ӟ') { // Ӟ (1246)
							return U'ӟ'; // ӟ (1247)
							return character + 1;
						} else if (character == U'Ӡ') { // Ӡ (1248)
							return U'ӡ'; // ӡ (1249)
							return character + 1;
						} else if (character == U'Ӣ') { // Ӣ (1250)
							return U'ӣ'; // ӣ (1251)
							return character + 1;
						} else if (character == U'Ӥ') { // Ӥ (1252)
							return U'ӥ'; // ӥ (1253)
							return character + 1;
						} else if (character == U'Ӧ') { // Ӧ (1254)
							return U'ӧ'; // ӧ (1255)
							return character + 1;
						} else if (character == U'Ө') { // Ө (1256)
							return U'ө'; // ө (1257)
							return character + 1;
						} else if (character == U'Ӫ') { // Ӫ (1258)
							return U'ӫ'; // ӫ (1259)
							return character + 1;
						} else if (character == U'Ӯ') { // Ӯ (1262)
							return U'ӯ'; // ӯ (1263)
							return character + 1;
						} else if (character == U'Ӱ') { // Ӱ (1264)
							return U'ӱ'; // ӱ (1265)
							return character + 1;
						} else if (character == U'Ӳ') { // Ӳ (1266)
							return U'ӳ'; // ӳ (1267)
							return character + 1;
						} else if (character == U'Ӵ') { // Ӵ (1268)
							return U'ӵ'; // ӵ (1269)
							return character + 1;
						} else if (character == U'Ӹ') { // Ӹ (1272)
							return U'ӹ'; // ӹ (1273)
							return character + 1;
						} else if (character == U'Ա') { // Ա (1329)
							return U'ա'; // ա (1377)
							return character + 48;
						} else if (character == U'Բ') { // Բ (1330)
							return U'բ'; // բ (1378)
							return character + 48;
						} else if (character == U'Գ') { // Գ (1331)
							return U'գ'; // գ (1379)
							return character + 48;
						} else if (character == U'Դ') { // Դ (1332)
							return U'դ'; // դ (1380)
							return character + 48;
						} else if (character == U'Ե') { // Ե (1333)
							return U'ե'; // ե (1381)
							return character + 48;
						} else if (character == U'Զ') { // Զ (1334)
							return U'զ'; // զ (1382)
							return character + 48;
						} else if (character == U'Է') { // Է (1335)
							return U'է'; // է (1383)
							return character + 48;
						} else if (character == U'Ը') { // Ը (1336)
							return U'ը'; // ը (1384)
							return character + 48;
						} else if (character == U'Թ') { // Թ (1337)
							return U'թ'; // թ (1385)
							return character + 48;
						} else if (character == U'Ժ') { // Ժ (1338)
							return U'ժ'; // ժ (1386)
							return character + 48;
						} else if (character == U'Ի') { // Ի (1339)
							return U'ի'; // ի (1387)
							return character + 48;
						} else if (character == U'Լ') { // Լ (1340)
							return U'լ'; // լ (1388)
							return character + 48;
						} else if (character == U'Խ') { // Խ (1341)
							return U'խ'; // խ (1389)
							return character + 48;
						} else if (character == U'Ծ') { // Ծ (1342)
							return U'ծ'; // ծ (1390)
							return character + 48;
						} else if (character == U'Կ') { // Կ (1343)
							return U'կ'; // կ (1391)
							return character + 48;
						} else if (character == U'Հ') { // Հ (1344)
							return U'հ'; // հ (1392)
							return character + 48;
						} else if (character == U'Ձ') { // Ձ (1345)
							return U'ձ'; // ձ (1393)
							return character + 48;
						} else if (character == U'Ղ') { // Ղ (1346)
							return U'ղ'; // ղ (1394)
							return character + 48;
						} else if (character == U'Ճ') { // Ճ (1347)
							return U'ճ'; // ճ (1395)
							return character + 48;
						} else if (character == U'Մ') { // Մ (1348)
							return U'մ'; // մ (1396)
							return character + 48;
						} else if (character == U'Յ') { // Յ (1349)
							return U'յ'; // յ (1397)
							return character + 48;
						} else if (character == U'Ն') { // Ն (1350)
							return U'ն'; // ն (1398)
							return character + 48;
						} else if (character == U'Շ') { // Շ (1351)
							return U'շ'; // շ (1399)
							return character + 48;
						} else if (character == U'Ո') { // Ո (1352)
							return U'ո'; // ո (1400)
							return character + 48;
						} else if (character == U'Չ') { // Չ (1353)
							return U'չ'; // չ (1401)
							return character + 48;
						} else if (character == U'Պ') { // Պ (1354)
							return U'պ'; // պ (1402)
							return character + 48;
						} else if (character == U'Ջ') { // Ջ (1355)
							return U'ջ'; // ջ (1403)
							return character + 48;
						} else if (character == U'Ռ') { // Ռ (1356)
							return U'ռ'; // ռ (1404)
							return character + 48;
						} else if (character == U'Ս') { // Ս (1357)
							return U'ս'; // ս (1405)
							return character + 48;
						} else if (character == U'Վ') { // Վ (1358)
							return U'վ'; // վ (1406)
							return character + 48;
						} else if (character == U'Տ') { // Տ (1359)
							return U'տ'; // տ (1407)
							return character + 48;
						} else if (character == U'Ր') { // Ր (1360)
							return U'ր'; // ր (1408)
							return character + 48;
						} else if (character == U'Ց') { // Ց (1361)
							return U'ց'; // ց (1409)
							return character + 48;
						} else if (character == U'Ւ') { // Ւ (1362)
							return U'ւ'; // ւ (1410)
							return character + 48;
						} else if (character == U'Փ') { // Փ (1363)
							return U'փ'; // փ (1411)
							return character + 48;
						} else if (character == U'Ք') { // Ք (1364)
							return U'ք'; // ք (1412)
							return character + 48;
						} else if (character == U'Օ') { // Օ (1365)
							return U'օ'; // օ (1413)
							return character + 48;
						} else if (character == U'Ֆ') { // Ֆ (1366)
							return U'ֆ'; // ֆ (1414)
							return character + 48;
						} else if (character == U'Ⴀ') { // Ⴀ (4256)
							return U'ა'; // ა (4304)
							return character + 48;
						} else if (character == U'Ⴁ') { // Ⴁ (4257)
							return U'ბ'; // ბ (4305)
							return character + 48;
						} else if (character == U'Ⴂ') { // Ⴂ (4258)
							return U'გ'; // გ (4306)
							return character + 48;
						} else if (character == U'Ⴃ') { // Ⴃ (4259)
							return U'დ'; // დ (4307)
							return character + 48;
						} else if (character == U'Ⴄ') { // Ⴄ (4260)
							return U'ე'; // ე (4308)
							return character + 48;
						} else if (character == U'Ⴅ') { // Ⴅ (4261)
							return U'ვ'; // ვ (4309)
							return character + 48;
						} else if (character == U'Ⴆ') { // Ⴆ (4262)
							return U'ზ'; // ზ (4310)
							return character + 48;
						} else if (character == U'Ⴇ') { // Ⴇ (4263)
							return U'თ'; // თ (4311)
							return character + 48;
						} else if (character == U'Ⴈ') { // Ⴈ (4264)
							return U'ი'; // ი (4312)
							return character + 48;
						} else if (character == U'Ⴉ') { // Ⴉ (4265)
							return U'კ'; // კ (4313)
							return character + 48;
						} else if (character == U'Ⴊ') { // Ⴊ (4266)
							return U'ლ'; // ლ (4314)
							return character + 48;
						} else if (character == U'Ⴋ') { // Ⴋ (4267)
							return U'მ'; // მ (4315)
							return character + 48;
						} else if (character == U'Ⴌ') { // Ⴌ (4268)
							return U'ნ'; // ნ (4316)
							return character + 48;
						} else if (character == U'Ⴍ') { // Ⴍ (4269)
							return U'ო'; // ო (4317)
							return character + 48;
						} else if (character == U'Ⴎ') { // Ⴎ (4270)
							return U'პ'; // პ (4318)
							return character + 48;
						} else if (character == U'Ⴏ') { // Ⴏ (4271)
							return U'ჟ'; // ჟ (4319)
							return character + 48;
						} else if (character == U'Ⴐ') { // Ⴐ (4272)
							return U'რ'; // რ (4320)
							return character + 48;
						} else if (character == U'Ⴑ') { // Ⴑ (4273)
							return U'ს'; // ს (4321)
							return character + 48;
						} else if (character == U'Ⴒ') { // Ⴒ (4274)
							return U'ტ'; // ტ (4322)
							return character + 48;
						} else if (character == U'Ⴓ') { // Ⴓ (4275)
							return U'უ'; // უ (4323)
							return character + 48;
						} else if (character == U'Ⴔ') { // Ⴔ (4276)
							return U'ფ'; // ფ (4324)
							return character + 48;
						} else if (character == U'Ⴕ') { // Ⴕ (4277)
							return U'ქ'; // ქ (4325)
							return character + 48;
						} else if (character == U'Ⴖ') { // Ⴖ (4278)
							return U'ღ'; // ღ (4326)
							return character + 48;
						} else if (character == U'Ⴗ') { // Ⴗ (4279)
							return U'ყ'; // ყ (4327)
							return character + 48;
						} else if (character == U'Ⴘ') { // Ⴘ (4280)
							return U'შ'; // შ (4328)
							return character + 48;
						} else if (character == U'Ⴙ') { // Ⴙ (4281)
							return U'ჩ'; // ჩ (4329)
							return character + 48;
						} else if (character == U'Ⴚ') { // Ⴚ (4282)
							return U'ც'; // ც (4330)
							return character + 48;
						} else if (character == U'Ⴛ') { // Ⴛ (4283)
							return U'ძ'; // ძ (4331)
							return character + 48;
						} else if (character == U'Ⴜ') { // Ⴜ (4284)
							return U'წ'; // წ (4332)
							return character + 48;
						} else if (character == U'Ⴝ') { // Ⴝ (4285)
							return U'ჭ'; // ჭ (4333)
							return character + 48;
						} else if (character == U'Ⴞ') { // Ⴞ (4286)
							return U'ხ'; // ხ (4334)
							return character + 48;
						} else if (character == U'Ⴟ') { // Ⴟ (4287)
							return U'ჯ'; // ჯ (4335)
							return character + 48;
						} else if (character == U'Ⴠ') { // Ⴠ (4288)
							return U'ჰ'; // ჰ (4336)
							return character + 48;
						} else if (character == U'Ⴡ') { // Ⴡ (4289)
							return U'ჱ'; // ჱ (4337)
							return character + 48;
						} else if (character == U'Ⴢ') { // Ⴢ (4290)
							return U'ჲ'; // ჲ (4338)
							return character + 48;
						} else if (character == U'Ⴣ') { // Ⴣ (4291)
							return U'ჳ'; // ჳ (4339)
							return character + 48;
						} else if (character == U'Ⴤ') { // Ⴤ (4292)
							return U'ჴ'; // ჴ (4340)
							return character + 48;
						} else if (character == U'Ⴥ') { // Ⴥ (4293)
							return U'ჵ'; // ჵ (4341)
							return character + 48;
						} else if (character == U'Ḁ') { // Ḁ (7680)
							return U'ḁ'; // ḁ (7681)
							return character + 1;
						} else if (character == U'Ḃ') { // Ḃ (7682)
							return U'ḃ'; // ḃ (7683)
							return character + 1;
						} else if (character == U'Ḅ') { // Ḅ (7684)
							return U'ḅ'; // ḅ (7685)
							return character + 1;
						} else if (character == U'Ḇ') { // Ḇ (7686)
							return U'ḇ'; // ḇ (7687)
							return character + 1;
						} else if (character == U'Ḉ') { // Ḉ (7688)
							return U'ḉ'; // ḉ (7689)
							return character + 1;
						} else if (character == U'Ḋ') { // Ḋ (7690)
							return U'ḋ'; // ḋ (7691)
							return character + 1;
						} else if (character == U'Ḍ') { // Ḍ (7692)
							return U'ḍ'; // ḍ (7693)
							return character + 1;
						} else if (character == U'Ḏ') { // Ḏ (7694)
							return U'ḏ'; // ḏ (7695)
							return character + 1;
						} else if (character == U'Ḑ') { // Ḑ (7696)
							return U'ḑ'; // ḑ (7697)
							return character + 1;
						} else if (character == U'Ḓ') { // Ḓ (7698)
							return U'ḓ'; // ḓ (7699)
							return character + 1;
						} else if (character == U'Ḕ') { // Ḕ (7700)
							return U'ḕ'; // ḕ (7701)
							return character + 1;
						} else if (character == U'Ḗ') { // Ḗ (7702)
							return U'ḗ'; // ḗ (7703)
							return character + 1;
						} else if (character == U'Ḙ') { // Ḙ (7704)
							return U'ḙ'; // ḙ (7705)
							return character + 1;
						} else if (character == U'Ḛ') { // Ḛ (7706)
							return U'ḛ'; // ḛ (7707)
							return character + 1;
						} else if (character == U'Ḝ') { // Ḝ (7708)
							return U'ḝ'; // ḝ (7709)
							return character + 1;
						} else if (character == U'Ḟ') { // Ḟ (7710)
							return U'ḟ'; // ḟ (7711)
							return character + 1;
						} else if (character == U'Ḡ') { // Ḡ (7712)
							return U'ḡ'; // ḡ (7713)
							return character + 1;
						} else if (character == U'Ḣ') { // Ḣ (7714)
							return U'ḣ'; // ḣ (7715)
							return character + 1;
						} else if (character == U'Ḥ') { // Ḥ (7716)
							return U'ḥ'; // ḥ (7717)
							return character + 1;
						} else if (character == U'Ḧ') { // Ḧ (7718)
							return U'ḧ'; // ḧ (7719)
							return character + 1;
						} else if (character == U'Ḩ') { // Ḩ (7720)
							return U'ḩ'; // ḩ (7721)
							return character + 1;
						} else if (character == U'Ḫ') { // Ḫ (7722)
							return U'ḫ'; // ḫ (7723)
							return character + 1;
						} else if (character == U'Ḭ') { // Ḭ (7724)
							return U'ḭ'; // ḭ (7725)
							return character + 1;
						} else if (character == U'Ḯ') { // Ḯ (7726)
							return U'ḯ'; // ḯ (7727)
							return character + 1;
						} else if (character == U'Ḱ') { // Ḱ (7728)
							return U'ḱ'; // ḱ (7729)
							return character + 1;
						} else if (character == U'Ḳ') { // Ḳ (7730)
							return U'ḳ'; // ḳ (7731)
							return character + 1;
						} else if (character == U'Ḵ') { // Ḵ (7732)
							return U'ḵ'; // ḵ (7733)
							return character + 1;
						} else if (character == U'Ḷ') { // Ḷ (7734)
							return U'ḷ'; // ḷ (7735)
							return character + 1;
						} else if (character == U'Ḹ') { // Ḹ (7736)
							return U'ḹ'; // ḹ (7737)
							return character + 1;
						} else if (character == U'Ḻ') { // Ḻ (7738)
							return U'ḻ'; // ḻ (7739)
							return character + 1;
						} else if (character == U'Ḽ') { // Ḽ (7740)
							return U'ḽ'; // ḽ (7741)
							return character + 1;
						} else if (character == U'Ḿ') { // Ḿ (7742)
							return U'ḿ'; // ḿ (7743)
							return character + 1;
						} else if (character == U'Ṁ') { // Ṁ (7744)
							return U'ṁ'; // ṁ (7745)
							return character + 1;
						} else if (character == U'Ṃ') { // Ṃ (7746)
							return U'ṃ'; // ṃ (7747)
							return character + 1;
						} else if (character == U'Ṅ') { // Ṅ (7748)
							return U'ṅ'; // ṅ (7749)
							return character + 1;
						} else if (character == U'Ṇ') { // Ṇ (7750)
							return U'ṇ'; // ṇ (7751)
							return character + 1;
						} else if (character == U'Ṉ') { // Ṉ (7752)
							return U'ṉ'; // ṉ (7753)
							return character + 1;
						} else if (character == U'Ṋ') { // Ṋ (7754)
							return U'ṋ'; // ṋ (7755)
							return character + 1;
						} else if (character == U'Ṍ') { // Ṍ (7756)
							return U'ṍ'; // ṍ (7757)
							return character + 1;
						} else if (character == U'Ṏ') { // Ṏ (7758)
							return U'ṏ'; // ṏ (7759)
							return character + 1;
						} else if (character == U'Ṑ') { // Ṑ (7760)
							return U'ṑ'; // ṑ (7761)
							return character + 1;
						} else if (character == U'Ṓ') { // Ṓ (7762)
							return U'ṓ'; // ṓ (7763)
							return character + 1;
						} else if (character == U'Ṕ') { // Ṕ (7764)
							return U'ṕ'; // ṕ (7765)
							return character + 1;
						} else if (character == U'Ṗ') { // Ṗ (7766)
							return U'ṗ'; // ṗ (7767)
							return character + 1;
						} else if (character == U'Ṙ') { // Ṙ (7768)
							return U'ṙ'; // ṙ (7769)
							return character + 1;
						} else if (character == U'Ṛ') { // Ṛ (7770)
							return U'ṛ'; // ṛ (7771)
							return character + 1;
						} else if (character == U'Ṝ') { // Ṝ (7772)
							return U'ṝ'; // ṝ (7773)
							return character + 1;
						} else if (character == U'Ṟ') { // Ṟ (7774)
							return U'ṟ'; // ṟ (7775)
							return character + 1;
						} else if (character == U'Ṡ') { // Ṡ (7776)
							return U'ṡ'; // ṡ (7777)
							return character + 1;
						} else if (character == U'Ṣ') { // Ṣ (7778)
							return U'ṣ'; // ṣ (7779)
							return character + 1;
						} else if (character == U'Ṥ') { // Ṥ (7780)
							return U'ṥ'; // ṥ (7781)
							return character + 1;
						} else if (character == U'Ṧ') { // Ṧ (7782)
							return U'ṧ'; // ṧ (7783)
							return character + 1;
						} else if (character == U'Ṩ') { // Ṩ (7784)
							return U'ṩ'; // ṩ (7785)
							return character + 1;
						} else if (character == U'Ṫ') { // Ṫ (7786)
							return U'ṫ'; // ṫ (7787)
							return character + 1;
						} else if (character == U'Ṭ') { // Ṭ (7788)
							return U'ṭ'; // ṭ (7789)
							return character + 1;
						} else if (character == U'Ṯ') { // Ṯ (7790)
							return U'ṯ'; // ṯ (7791)
							return character + 1;
						} else if (character == U'Ṱ') { // Ṱ (7792)
							return U'ṱ'; // ṱ (7793)
							return character + 1;
						} else if (character == U'Ṳ') { // Ṳ (7794)
							return U'ṳ'; // ṳ (7795)
							return character + 1;
						} else if (character == U'Ṵ') { // Ṵ (7796)
							return U'ṵ'; // ṵ (7797)
							return character + 1;
						} else if (character == U'Ṷ') { // Ṷ (7798)
							return U'ṷ'; // ṷ (7799)
							return character + 1;
						} else if (character == U'Ṹ') { // Ṹ (7800)
							return U'ṹ'; // ṹ (7801)
							return character + 1;
						} else if (character == U'Ṻ') { // Ṻ (7802)
							return U'ṻ'; // ṻ (7803)
							return character + 1;
						} else if (character == U'Ṽ') { // Ṽ (7804)
							return U'ṽ'; // ṽ (7805)
							return character + 1;
						} else if (character == U'Ṿ') { // Ṿ (7806)
							return U'ṿ'; // ṿ (7807)
							return character + 1;
						} else if (character == U'Ẁ') { // Ẁ (7808)
							return U'ẁ'; // ẁ (7809)
							return character + 1;
						} else if (character == U'Ẃ') { // Ẃ (7810)
							return U'ẃ'; // ẃ (7811)
							return character + 1;
						} else if (character == U'Ẅ') { // Ẅ (7812)
							return U'ẅ'; // ẅ (7813)
							return character + 1;
						} else if (character == U'Ẇ') { // Ẇ (7814)
							return U'ẇ'; // ẇ (7815)
							return character + 1;
						} else if (character == U'Ẉ') { // Ẉ (7816)
							return U'ẉ'; // ẉ (7817)
							return character + 1;
						} else if (character == U'Ẋ') { // Ẋ (7818)
							return U'ẋ'; // ẋ (7819)
							return character + 1;
						} else if (character == U'Ẍ') { // Ẍ (7820)
							return U'ẍ'; // ẍ (7821)
							return character + 1;
						} else if (character == U'Ẏ') { // Ẏ (7822)
							return U'ẏ'; // ẏ (7823)
							return character + 1;
						} else if (character == U'Ẑ') { // Ẑ (7824)
							return U'ẑ'; // ẑ (7825)
							return character + 1;
						} else if (character == U'Ẓ') { // Ẓ (7826)
							return U'ẓ'; // ẓ (7827)
							return character + 1;
						} else if (character == U'Ẕ') { // Ẕ (7828)
							return U'ẕ'; // ẕ (7829)
							return character + 1;
						} else if (character == U'ẞ') { // ẞ (7838)
							return U'ß'; // ß (223)
							return character + 7615;
						} else if (character == U'Ạ') { // Ạ (7840)
							return U'ạ'; // ạ (7841)
							return character + 1;
						} else if (character == U'Ả') { // Ả (7842)
							return U'ả'; // ả (7843)
							return character + 1;
						} else if (character == U'Ấ') { // Ấ (7844)
							return U'ấ'; // ấ (7845)
							return character + 1;
						} else if (character == U'Ầ') { // Ầ (7846)
							return U'ầ'; // ầ (7847)
							return character + 1;
						} else if (character == U'Ẩ') { // Ẩ (7848)
							return U'ẩ'; // ẩ (7849)
							return character + 1;
						} else if (character == U'Ẫ') { // Ẫ (7850)
							return U'ẫ'; // ẫ (7851)
							return character + 1;
						} else if (character == U'Ậ') { // Ậ (7852)
							return U'ậ'; // ậ (7853)
							return character + 1;
						} else if (character == U'Ắ') { // Ắ (7854)
							return U'ắ'; // ắ (7855)
							return character + 1;
						} else if (character == U'Ằ') { // Ằ (7856)
							return U'ằ'; // ằ (7857)
							return character + 1;
						} else if (character == U'Ẳ') { // Ẳ (7858)
							return U'ẳ'; // ẳ (7859)
							return character + 1;
						} else if (character == U'Ẵ') { // Ẵ (7860)
							return U'ẵ'; // ẵ (7861)
							return character + 1;
						} else if (character == U'Ặ') { // Ặ (7862)
							return U'ặ'; // ặ (7863)
							return character + 1;
						} else if (character == U'Ẹ') { // Ẹ (7864)
							return U'ẹ'; // ẹ (7865)
							return character + 1;
						} else if (character == U'Ẻ') { // Ẻ (7866)
							return U'ẻ'; // ẻ (7867)
							return character + 1;
						} else if (character == U'Ẽ') { // Ẽ (7868)
							return U'ẽ'; // ẽ (7869)
							return character + 1;
						} else if (character == U'Ế') { // Ế (7870)
							return U'ế'; // ế (7871)
							return character + 1;
						} else if (character == U'Ề') { // Ề (7872)
							return U'ề'; // ề (7873)
							return character + 1;
						} else if (character == U'Ể') { // Ể (7874)
							return U'ể'; // ể (7875)
							return character + 1;
						} else if (character == U'Ễ') { // Ễ (7876)
							return U'ễ'; // ễ (7877)
							return character + 1;
						} else if (character == U'Ệ') { // Ệ (7878)
							return U'ệ'; // ệ (7879)
							return character + 1;
						} else if (character == U'Ỉ') { // Ỉ (7880)
							return U'ỉ'; // ỉ (7881)
							return character + 1;
						} else if (character == U'Ị') { // Ị (7882)
							return U'ị'; // ị (7883)
							return character + 1;
						} else if (character == U'Ọ') { // Ọ (7884)
							return U'ọ'; // ọ (7885)
							return character + 1;
						} else if (character == U'Ỏ') { // Ỏ (7886)
							return U'ỏ'; // ỏ (7887)
							return character + 1;
						} else if (character == U'Ố') { // Ố (7888)
							return U'ố'; // ố (7889)
							return character + 1;
						} else if (character == U'Ồ') { // Ồ (7890)
							return U'ồ'; // ồ (7891)
							return character + 1;
						} else if (character == U'Ổ') { // Ổ (7892)
							return U'ổ'; // ổ (7893)
							return character + 1;
						} else if (character == U'Ỗ') { // Ỗ (7894)
							return U'ỗ'; // ỗ (7895)
							return character + 1;
						} else if (character == U'Ộ') { // Ộ (7896)
							return U'ộ'; // ộ (7897)
							return character + 1;
						} else if (character == U'Ớ') { // Ớ (7898)
							return U'ớ'; // ớ (7899)
							return character + 1;
						} else if (character == U'Ờ') { // Ờ (7900)
							return U'ờ'; // ờ (7901)
							return character + 1;
						} else if (character == U'Ở') { // Ở (7902)
							return U'ở'; // ở (7903)
							return character + 1;
						} else if (character == U'Ỡ') { // Ỡ (7904)
							return U'ỡ'; // ỡ (7905)
							return character + 1;
						} else if (character == U'Ợ') { // Ợ (7906)
							return U'ợ'; // ợ (7907)
							return character + 1;
						} else if (character == U'Ụ') { // Ụ (7908)
							return U'ụ'; // ụ (7909)
							return character + 1;
						} else if (character == U'Ủ') { // Ủ (7910)
							return U'ủ'; // ủ (7911)
							return character + 1;
						} else if (character == U'Ứ') { // Ứ (7912)
							return U'ứ'; // ứ (7913)
							return character + 1;
						} else if (character == U'Ừ') { // Ừ (7914)
							return U'ừ'; // ừ (7915)
							return character + 1;
						} else if (character == U'Ử') { // Ử (7916)
							return U'ử'; // ử (7917)
							return character + 1;
						} else if (character == U'Ữ') { // Ữ (7918)
							return U'ữ'; // ữ (7919)
							return character + 1;
						} else if (character == U'Ự') { // Ự (7920)
							return U'ự'; // ự (7921)
							return character + 1;
						} else if (character == U'Ỳ') { // Ỳ (7922)
							return U'ỳ'; // ỳ (7923)
							return character + 1;
						} else if (character == U'Ỵ') { // Ỵ (7924)
							return U'ỵ'; // ỵ (7925)
							return character + 1;
						} else if (character == U'Ỷ') { // Ỷ (7926)
							return U'ỷ'; // ỷ (7927)
							return character + 1;
						} else if (character == U'Ỹ') { // Ỹ (7928)
							return U'ỹ'; // ỹ (7929)
							return character + 1;
						} else if (character == U'Ἀ') { // Ἀ (7944)
							return U'ἀ'; // ἀ (7936)
							return character + 8;
						} else if (character == U'Ἁ') { // Ἁ (7945)
							return U'ἁ'; // ἁ (7937)
							return character + 8;
						} else if (character == U'Ἂ') { // Ἂ (7946)
							return U'ἂ'; // ἂ (7938)
							return character + 8;
						} else if (character == U'Ἃ') { // Ἃ (7947)
							return U'ἃ'; // ἃ (7939)
							return character + 8;
						} else if (character == U'Ἄ') { // Ἄ (7948)
							return U'ἄ'; // ἄ (7940)
							return character + 8;
						} else if (character == U'Ἅ') { // Ἅ (7949)
							return U'ἅ'; // ἅ (7941)
							return character + 8;
						} else if (character == U'Ἆ') { // Ἆ (7950)
							return U'ἆ'; // ἆ (7942)
							return character + 8;
						} else if (character == U'Ἇ') { // Ἇ (7951)
							return U'ἇ'; // ἇ (7943)
							return character + 8;
						} else if (character == U'Ἐ') { // Ἐ (7960)
							return U'ἐ'; // ἐ (7952)
							return character + 8;
						} else if (character == U'Ἑ') { // Ἑ (7961)
							return U'ἑ'; // ἑ (7953)
							return character + 8;
						} else if (character == U'Ἒ') { // Ἒ (7962)
							return U'ἒ'; // ἒ (7954)
							return character + 8;
						} else if (character == U'Ἓ') { // Ἓ (7963)
							return U'ἓ'; // ἓ (7955)
							return character + 8;
						} else if (character == U'Ἔ') { // Ἔ (7964)
							return U'ἔ'; // ἔ (7956)
							return character + 8;
						} else if (character == U'Ἕ') { // Ἕ (7965)
							return U'ἕ'; // ἕ (7957)
							return character + 8;
						} else if (character == U'Ἠ') { // Ἠ (7976)
							return U'ἠ'; // ἠ (7968)
							return character + 8;
						} else if (character == U'Ἡ') { // Ἡ (7977)
							return U'ἡ'; // ἡ (7969)
							return character + 8;
						} else if (character == U'Ἢ') { // Ἢ (7978)
							return U'ἢ'; // ἢ (7970)
							return character + 8;
						} else if (character == U'Ἣ') { // Ἣ (7979)
							return U'ἣ'; // ἣ (7971)
							return character + 8;
						} else if (character == U'Ἤ') { // Ἤ (7980)
							return U'ἤ'; // ἤ (7972)
							return character + 8;
						} else if (character == U'Ἥ') { // Ἥ (7981)
							return U'ἥ'; // ἥ (7973)
							return character + 8;
						} else if (character == U'Ἦ') { // Ἦ (7982)
							return U'ἦ'; // ἦ (7974)
							return character + 8;
						} else if (character == U'Ἧ') { // Ἧ (7983)
							return U'ἧ'; // ἧ (7975)
							return character + 8;
						} else if (character == U'Ἰ') { // Ἰ (7992)
							return U'ἰ'; // ἰ (7984)
							return character + 8;
						} else if (character == U'Ἱ') { // Ἱ (7993)
							return U'ἱ'; // ἱ (7985)
							return character + 8;
						} else if (character == U'Ἲ') { // Ἲ (7994)
							return U'ἲ'; // ἲ (7986)
							return character + 8;
						} else if (character == U'Ἳ') { // Ἳ (7995)
							return U'ἳ'; // ἳ (7987)
							return character + 8;
						} else if (character == U'Ἴ') { // Ἴ (7996)
							return U'ἴ'; // ἴ (7988)
							return character + 8;
						} else if (character == U'Ἵ') { // Ἵ (7997)
							return U'ἵ'; // ἵ (7989)
							return character + 8;
						} else if (character == U'Ἶ') { // Ἶ (7998)
							return U'ἶ'; // ἶ (7990)
							return character + 8;
						} else if (character == U'Ἷ') { // Ἷ (7999)
							return U'ἷ'; // ἷ (7991)
							return character + 8;
						} else if (character == U'Ὀ') { // Ὀ (8008)
							return U'ὀ'; // ὀ (8000)
							return character + 8;
						} else if (character == U'Ὁ') { // Ὁ (8009)
							return U'ὁ'; // ὁ (8001)
							return character + 8;
						} else if (character == U'Ὂ') { // Ὂ (8010)
							return U'ὂ'; // ὂ (8002)
							return character + 8;
						} else if (character == U'Ὃ') { // Ὃ (8011)
							return U'ὃ'; // ὃ (8003)
							return character + 8;
						} else if (character == U'Ὄ') { // Ὄ (8012)
							return U'ὄ'; // ὄ (8004)
							return character + 8;
						} else if (character == U'Ὅ') { // Ὅ (8013)
							return U'ὅ'; // ὅ (8005)
							return character + 8;
						} else if (character == U'Ὑ') { // Ὑ (8025)
							return U'ὑ'; // ὑ (8017)
							return character + 8;
						} else if (character == U'Ὓ') { // Ὓ (8027)
							return U'ὓ'; // ὓ (8019)
							return character + 8;
						} else if (character == U'Ὕ') { // Ὕ (8029)
							return U'ὕ'; // ὕ (8021)
							return character + 8;
						} else if (character == U'Ὗ') { // Ὗ (8031)
							return U'ὗ'; // ὗ (8023)
							return character + 8;
						} else if (character == U'Ὠ') { // Ὠ (8040)
							return U'ὠ'; // ὠ (8032)
							return character + 8;
						} else if (character == U'Ὡ') { // Ὡ (8041)
							return U'ὡ'; // ὡ (8033)
							return character + 8;
						} else if (character == U'Ὢ') { // Ὢ (8042)
							return U'ὢ'; // ὢ (8034)
							return character + 8;
						} else if (character == U'Ὣ') { // Ὣ (8043)
							return U'ὣ'; // ὣ (8035)
							return character + 8;
						} else if (character == U'Ὤ') { // Ὤ (8044)
							return U'ὤ'; // ὤ (8036)
							return character + 8;
						} else if (character == U'Ὥ') { // Ὥ (8045)
							return U'ὥ'; // ὥ (8037)
							return character + 8;
						} else if (character == U'Ὦ') { // Ὦ (8046)
							return U'ὦ'; // ὦ (8038)
							return character + 8;
						} else if (character == U'Ὧ') { // Ὧ (8047)
							return U'ὧ'; // ὧ (8039)
							return character + 8;
						} else if (character == U'ᾈ') { // ᾈ (8072)
							return U'ᾀ'; // ᾀ (8064)
							return character + 8;
						} else if (character == U'ᾉ') { // ᾉ (8073)
							return U'ᾁ'; // ᾁ (8065)
							return character + 8;
						} else if (character == U'ᾊ') { // ᾊ (8074)
							return U'ᾂ'; // ᾂ (8066)
							return character + 8;
						} else if (character == U'ᾋ') { // ᾋ (8075)
							return U'ᾃ'; // ᾃ (8067)
							return character + 8;
						} else if (character == U'ᾌ') { // ᾌ (8076)
							return U'ᾄ'; // ᾄ (8068)
							return character + 8;
						} else if (character == U'ᾍ') { // ᾍ (8077)
							return U'ᾅ'; // ᾅ (8069)
							return character + 8;
						} else if (character == U'ᾎ') { // ᾎ (8078)
							return U'ᾆ'; // ᾆ (8070)
							return character + 8;
						} else if (character == U'ᾏ') { // ᾏ (8079)
							return U'ᾇ'; // ᾇ (8071)
							return character + 8;
						} else if (character == U'ᾘ') { // ᾘ (8088)
							return U'ᾐ'; // ᾐ (8080)
							return character + 8;
						} else if (character == U'ᾙ') { // ᾙ (8089)
							return U'ᾑ'; // ᾑ (8081)
							return character + 8;
						} else if (character == U'ᾚ') { // ᾚ (8090)
							return U'ᾒ'; // ᾒ (8082)
							return character + 8;
						} else if (character == U'ᾛ') { // ᾛ (8091)
							return U'ᾓ'; // ᾓ (8083)
							return character + 8;
						} else if (character == U'ᾜ') { // ᾜ (8092)
							return U'ᾔ'; // ᾔ (8084)
							return character + 8;
						} else if (character == U'ᾝ') { // ᾝ (8093)
							return U'ᾕ'; // ᾕ (8085)
							return character + 8;
						} else if (character == U'ᾞ') { // ᾞ (8094)
							return U'ᾖ'; // ᾖ (8086)
							return character + 8;
						} else if (character == U'ᾟ') { // ᾟ (8095)
							return U'ᾗ'; // ᾗ (8087)
							return character + 8;
						} else if (character == U'ᾨ') { // ᾨ (8104)
							return U'ᾠ'; // ᾠ (8096)
							return character + 8;
						} else if (character == U'ᾩ') { // ᾩ (8105)
							return U'ᾡ'; // ᾡ (8097)
							return character + 8;
						} else if (character == U'ᾪ') { // ᾪ (8106)
							return U'ᾢ'; // ᾢ (8098)
							return character + 8;
						} else if (character == U'ᾫ') { // ᾫ (8107)
							return U'ᾣ'; // ᾣ (8099)
							return character + 8;
						} else if (character == U'ᾬ') { // ᾬ (8108)
							return U'ᾤ'; // ᾤ (8100)
							return character + 8;
						} else if (character == U'ᾭ') { // ᾭ (8109)
							return U'ᾥ'; // ᾥ (8101)
							return character + 8;
						} else if (character == U'ᾮ') { // ᾮ (8110)
							return U'ᾦ'; // ᾦ (8102)
							return character + 8;
						} else if (character == U'ᾯ') { // ᾯ (8111)
							return U'ᾧ'; // ᾧ (8103)
							return character + 8;
						} else if (character == U'Ᾰ') { // Ᾰ (8120)
							return U'ᾰ'; // ᾰ (8112)
							return character + 8;
						} else if (character == U'Ᾱ') { // Ᾱ (8121)
							return U'ᾱ'; // ᾱ (8113)
							return character + 8;
						} else if (character == U'Ῐ') { // Ῐ (8152)
							return U'ῐ'; // ῐ (8144)
							return character + 8;
						} else if (character == U'Ῑ') { // Ῑ (8153)
							return U'ῑ'; // ῑ (8145)
							return character + 8;
						} else if (character == U'Ῠ') { // Ῠ (8168)
							return U'ῠ'; // ῠ (8160)
							return character + 8;
						} else if (character == U'Ῡ') { // Ῡ (8169)
							return U'ῡ'; // ῡ (8161)
							return character + 8;
						} else if (character == U'Ⓐ') { // Ⓐ (9398)
							return U'ⓐ'; // ⓐ (9424)
							return character + 26;
						} else if (character == U'Ⓑ') { // Ⓑ (9399)
							return U'ⓑ'; // ⓑ (9425)
							return character + 26;
						} else if (character == U'Ⓒ') { // Ⓒ (9400)
							return U'ⓒ'; // ⓒ (9426)
							return character + 26;
						} else if (character == U'Ⓓ') { // Ⓓ (9401)
							return U'ⓓ'; // ⓓ (9427)
							return character + 26;
						} else if (character == U'Ⓔ') { // Ⓔ (9402)
							return U'ⓔ'; // ⓔ (9428)
							return character + 26;
						} else if (character == U'Ⓕ') { // Ⓕ (9403)
							return U'ⓕ'; // ⓕ (9429)
							return character + 26;
						} else if (character == U'Ⓖ') { // Ⓖ (9404)
							return U'ⓖ'; // ⓖ (9430)
							return character + 26;
						} else if (character == U'Ⓗ') { // Ⓗ (9405)
							return U'ⓗ'; // ⓗ (9431)
							return character + 26;
						} else if (character == U'Ⓘ') { // Ⓘ (9406)
							return U'ⓘ'; // ⓘ (9432)
							return character + 26;
						} else if (character == U'Ⓙ') { // Ⓙ (9407)
							return U'ⓙ'; // ⓙ (9433)
							return character + 26;
						} else if (character == U'Ⓚ') { // Ⓚ (9408)
							return U'ⓚ'; // ⓚ (9434)
							return character + 26;
						} else if (character == U'Ⓛ') { // Ⓛ (9409)
							return U'ⓛ'; // ⓛ (9435)
							return character + 26;
						} else if (character == U'Ⓜ') { // Ⓜ (9410)
							return U'ⓜ'; // ⓜ (9436)
							return character + 26;
						} else if (character == U'Ⓝ') { // Ⓝ (9411)
							return U'ⓝ'; // ⓝ (9437)
							return character + 26;
						} else if (character == U'Ⓞ') { // Ⓞ (9412)
							return U'ⓞ'; // ⓞ (9438)
							return character + 26;
						} else if (character == U'Ⓟ') { // Ⓟ (9413)
							return U'ⓟ'; // ⓟ (9439)
							return character + 26;
						} else if (character == U'Ⓠ') { // Ⓠ (9414)
							return U'ⓠ'; // ⓠ (9440)
							return character + 26;
						} else if (character == U'Ⓡ') { // Ⓡ (9415)
							return U'ⓡ'; // ⓡ (9441)
							return character + 26;
						} else if (character == U'Ⓢ') { // Ⓢ (9416)
							return U'ⓢ'; // ⓢ (9442)
							return character + 26;
						} else if (character == U'Ⓣ') { // Ⓣ (9417)
							return U'ⓣ'; // ⓣ (9443)
							return character + 26;
						} else if (character == U'Ⓤ') { // Ⓤ (9418)
							return U'ⓤ'; // ⓤ (9444)
							return character + 26;
						} else if (character == U'Ⓥ') { // Ⓥ (9419)
							return U'ⓥ'; // ⓥ (9445)
							return character + 26;
						} else if (character == U'Ⓦ') { // Ⓦ (9420)
							return U'ⓦ'; // ⓦ (9446)
							return character + 26;
						} else if (character == U'Ⓧ') { // Ⓧ (9421)
							return U'ⓧ'; // ⓧ (9447)
							return character + 26;
						} else if (character == U'Ⓨ') { // Ⓨ (9422)
							return U'ⓨ'; // ⓨ (9448)
							return character + 26;
						} else if (character == U'Ⓩ') { // Ⓩ (9423)
							return U'ⓩ'; // ⓩ (9449)
							return character + 26;
						} else if (character == U'Ɽ') { // Ɽ (11364)
							return U'ɽ'; // ɽ (637)
							return character + 10727;
						} else if (character == U'Ɑ') { // Ɑ (11373)
							return U'ɑ'; // ɑ (593)
							return character + 10780;
						} else if (character == U'Ɱ') { // Ɱ (11374)
							return U'ɱ'; // ɱ (625)
							return character + 10749;
						} else if (character == U'Ɐ') { // Ɐ (11375)
							return U'ɐ'; // ɐ (592)
							return character + 10783;
						} else if (character == U'Ɒ') { // Ɒ (11376)
							return U'ɒ'; // ɒ (594)
							return character + 10782;
						} else if (character == U'Ȿ') { // Ȿ (11390)
							return U'ȿ'; // ȿ (575)
							return character + 10815;
						} else if (character == U'Ɀ') { // Ɀ (11391)
							return U'ɀ'; // ɀ (576)
							return character + 10815;
						} else if (character == U'Ɥ') { // Ɥ (42893)
							return U'ɥ'; // ɥ (613)
							return character + 42280;
						} else if (character == U'Ɪ') { // Ɪ (42926)
							return U'ɪ'; // ɪ (618)
							return character + 42308;
						} else if (character == U'Ａ') { // Ａ (65313)
							return U'ａ'; // ａ (65345)
							return character + 32;
						} else if (character == U'Ｂ') { // Ｂ (65314)
							return U'ｂ'; // ｂ (65346)
							return character + 32;
						} else if (character == U'Ｃ') { // Ｃ (65315)
							return U'ｃ'; // ｃ (65347)
							return character + 32;
						} else if (character == U'Ｄ') { // Ｄ (65316)
							return U'ｄ'; // ｄ (65348)
							return character + 32;
						} else if (character == U'Ｅ') { // Ｅ (65317)
							return U'ｅ'; // ｅ (65349)
							return character + 32;
						} else if (character == U'Ｆ') { // Ｆ (65318)
							return U'ｆ'; // ｆ (65350)
							return character + 32;
						} else if (character == U'Ｇ') { // Ｇ (65319)
							return U'ｇ'; // ｇ (65351)
							return character + 32;
						} else if (character == U'Ｈ') { // Ｈ (65320)
							return U'ｈ'; // ｈ (65352)
							return character + 32;
						} else if (character == U'Ｉ') { // Ｉ (65321)
							return U'ｉ'; // ｉ (65353)
							return character + 32;
						} else if (character == U'Ｊ') { // Ｊ (65322)
							return U'ｊ'; // ｊ (65354)
							return character + 32;
						} else if (character == U'Ｋ') { // Ｋ (65323)
							return U'ｋ'; // ｋ (65355)
							return character + 32;
						} else if (character == U'Ｌ') { // Ｌ (65324)
							return U'ｌ'; // ｌ (65356)
							return character + 32;
						} else if (character == U'Ｍ') { // Ｍ (65325)
							return U'ｍ'; // ｍ (65357)
							return character + 32;
						} else if (character == U'Ｎ') { // Ｎ (65326)
							return U'ｎ'; // ｎ (65358)
							return character + 32;
						} else if (character == U'Ｏ') { // Ｏ (65327)
							return U'ｏ'; // ｏ (65359)
							return character + 32;
						} else if (character == U'Ｐ') { // Ｐ (65328)
							return U'ｐ'; // ｐ (65360)
							return character + 32;
						} else if (character == U'Ｑ') { // Ｑ (65329)
							return U'ｑ'; // ｑ (65361)
							return character + 32;
						} else if (character == U'Ｒ') { // Ｒ (65330)
							return U'ｒ'; // ｒ (65362)
							return character + 32;
						} else if (character == U'Ｓ') { // Ｓ (65331)
							return U'ｓ'; // ｓ (65363)
							return character + 32;
						} else if (character == U'Ｔ') { // Ｔ (65332)
							return U'ｔ'; // ｔ (65364)
							return character + 32;
						} else if (character == U'Ｕ') { // Ｕ (65333)
							return U'ｕ'; // ｕ (65365)
							return character + 32;
						} else if (character == U'Ｖ') { // Ｖ (65334)
							return U'ｖ'; // ｖ (65366)
							return character + 32;
						} else if (character == U'Ｗ') { // Ｗ (65335)
							return U'ｗ'; // ｗ (65367)
							return character + 32;
						} else if (character == U'Ｘ') { // Ｘ (65336)
							return U'ｘ'; // ｘ (65368)
							return character + 32;
						} else if (character == U'Ｙ') { // Ｙ (65337)
							return U'ｙ'; // ｙ (65369)
							return character + 32;
						} else if (character == U'Ｚ') { // Ｚ (65338)
							return U'ｚ'; // ｚ (65370)
							return character + 32;
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
