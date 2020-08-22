
#include "../testTools.h"

// These tests will fail if the source code document or stored files change their encoding of line breaks.

String expected_latin1 =
UR"QUOTE(Hello my friend
Hej min vän
Halló, vinur minn
Hei ystäväni
Hola mi amigo
Ciao amico
)QUOTE";

// Warning!
//   String literals containing characters above value 255 must be stored explicitly in unicode literals using U"" instead of "".
//   Because string literals do not begin with a byte order mark to say which encoding is being used.
//   Also make sure to save the source code document using a byte order mark so that the C++ compiler receives the correct symbol.
String unicodeContent =
UR"QUOTE(Hello my friend
Hej min vän
Halló, vinur minn
Hei ystäväni
Hola mi amigo
Ciao amico
你好我的朋友
こんにちは、友よ
नमस्ते मेरो साथी
Talofa laʻu uo
Xin chào bạn của tôi
העלא מיין פרייַנד
안녕 내 친구
سلام دوست من
ਹੈਲੋ ਮੇਰੇ ਦੋਸਤ
ওহে, বন্ধু আমার
សួស្តី​សម្លាញ់
Γεια σου φίλε μου
Привет, мой друг
здраво пријатељу
Բարեւ իմ ընկեր
ආයුබෝවන් මාගේ යාළුවා
ಹಲೋ ನನ್ನ ಸ್ನೇಹಿತನೇ
Silav hevalê min
اهلا صديقي
𐐷
)QUOTE";
String expected_utf8 = unicodeContent + U"\nThis is UTF-8";
String expected_utf16le = unicodeContent + U"\nThis is UTF-16 Little Endian";
String expected_utf16be = unicodeContent + U"\nThis is UTF-16 Big Endian";

void printBinary(uint32_t value, int maxBits) {
	for (int i = 0; i < maxBits; i++) {
		if (value & (uint32_t)0b1 << (maxBits - 1)) {
			printText(U"1");
		} else {
			printText(U"0");
		}
		value = value << 1;
	}
}

void printBuffer(Buffer buffer) {
	int length = buffer_getSize(buffer);
	SafePointer<uint8_t> data = buffer_getSafeData<uint8_t>(buffer, "Generic buffer");
	printText(U"Buffer of length ", length, U":\n");
	for (int i = 0; i < length; i++) {
		printBinary(data[i], 8);
		printText(U" @", i, U"\n");
	}
}

// Method for printing the character codes of a string for debugging
void compareCharacterCodes(String textA, String textB) {
	int lengthA = string_length(textA);
	int lengthB = string_length(textB);
	int minLength = lengthA < lengthB ? lengthA : lengthB;
	printText(U"Character codes for strings of length ", lengthA, U" and ", lengthB, U":\n");
	for (int i = 0; i < minLength; i++) {
		uint32_t codeA = (uint32_t)textA[i];
		uint32_t codeB = (uint32_t)textB[i];
		printBinary(codeA, 32);
		if (codeA == codeB) {
			printText(U" == ");
		} else {
			printText(U" != ");
		}
		printBinary(codeB, 32);
		printText(U" (", textA[i], U") (", textB[i], U")\n");
	}
	if (lengthA > lengthB) {
		for (int i = minLength; i < lengthA; i++) {
			uint32_t codeA = (uint32_t)textA[i];
			printBinary(codeA, 32);
			printText(U" (", textA[i], U")\n");
		}
	} else {
		printText(U"                                    ");
		for (int i = minLength; i < lengthB; i++) {
			uint32_t codeB = (uint32_t)textB[i];
			printBinary(codeB, 32);
			printText(U" (", textB[i], U")\n");
		}
	}
}

START_TEST(TextEncoding)
	String folderPath = string_combine(U"test", file_separator(), U"tests", file_separator(), U"resources", file_separator());
	{ // Text encodings stored in memory
		// Run these tests for all line encodings
		for (int l = 0; l <= 1; l++) {
			LineEncoding lineEncoding = (l == 0) ? LineEncoding::CrLf : LineEncoding::Lf;
			// \r is not saved to files for cross-platform compatibility
			// \0 is not saved to files because files have a known size and don't need them
			{ // Latin-1 up to U+FF excluding \r and \0
				String originalLatin1;
				string_reserve(originalLatin1, 0xFF);
				for (DsrChar c = 0x1; c <= 0xFF; c++) {
					if (c != U'\r') {
						string_appendChar(originalLatin1, c);
					}
				}
				Buffer encoded = string_saveToMemory(originalLatin1, CharacterEncoding::Raw_Latin1, lineEncoding);
				String decodedLatin1 = string_loadFromMemory(encoded);
				//compareCharacterCodes(originalLatin1, decodedLatin1);
				ASSERT_MATCH(originalLatin1, decodedLatin1);
			}
			{ // UTF-8 up to U+10FFFF excluding \r and \0
				String originalUTF8;
				string_reserve(originalUTF8, 0x10FFFF);
				for (DsrChar c = 0x1; c <= 0x10FFFF; c++) {
					if (c != U'\r') {
						string_appendChar(originalUTF8, c);
					}
				}
				Buffer encoded = string_saveToMemory(originalUTF8, CharacterEncoding::BOM_UTF8, lineEncoding);
				String decodedUTF8 = string_loadFromMemory(encoded);
				ASSERT_MATCH(originalUTF8, decodedUTF8);
			}
			// Selected cases for UTF-16
			for (int e = 0; e <= 1; e++) {
				CharacterEncoding characterEncoding = (e == 0) ? CharacterEncoding::BOM_UTF16BE : CharacterEncoding::BOM_UTF16LE;
				String originalUTF16;
				// 20-bit test cases
				string_appendChar(originalUTF16, 0b00000000000000000001);
				string_appendChar(originalUTF16, 0b00000000000000000010);
				string_appendChar(originalUTF16, 0b00000000000000000011);
				string_appendChar(originalUTF16, 0b00000000000000000100);
				string_appendChar(originalUTF16, 0b00000000000000000111);
				string_appendChar(originalUTF16, 0b00000000000000001000);
				string_appendChar(originalUTF16, 0b00000000000000001111);
				string_appendChar(originalUTF16, 0b00000000000000010000);
				string_appendChar(originalUTF16, 0b00000000000000011111);
				string_appendChar(originalUTF16, 0b00000000000000100000);
				string_appendChar(originalUTF16, 0b00000000000000111111);
				string_appendChar(originalUTF16, 0b00000000000001000000);
				string_appendChar(originalUTF16, 0b00000000000001111111);
				string_appendChar(originalUTF16, 0b00000000000010000000);
				string_appendChar(originalUTF16, 0b00000000000011111111);
				string_appendChar(originalUTF16, 0b00000000000100000000);
				string_appendChar(originalUTF16, 0b00000000000111111111);
				string_appendChar(originalUTF16, 0b00000000001000000000);
				string_appendChar(originalUTF16, 0b00000000001111111111);
				string_appendChar(originalUTF16, 0b00000000010000000000);
				string_appendChar(originalUTF16, 0b00000000011111111111);
				string_appendChar(originalUTF16, 0b00000000100000000000);
				string_appendChar(originalUTF16, 0b00000000111111111111);
				string_appendChar(originalUTF16, 0b00000001000000000000);
				string_appendChar(originalUTF16, 0b00000001111111111111);
				string_appendChar(originalUTF16, 0b00000010000000000000);
				string_appendChar(originalUTF16, 0b00000011111111111111);
				string_appendChar(originalUTF16, 0b00000100000000000000);
				string_appendChar(originalUTF16, 0b00000111111111111111);
				string_appendChar(originalUTF16, 0b00001000000000000000);
				string_appendChar(originalUTF16, 0b00001111111111111111);
				string_appendChar(originalUTF16, 0b00010000000000000000);
				string_appendChar(originalUTF16, 0b00011111111111111111);
				string_appendChar(originalUTF16, 0b00100000000000000000);
				string_appendChar(originalUTF16, 0b00111111111111111111);
				string_appendChar(originalUTF16, 0b01000000000000000000);
				string_appendChar(originalUTF16, 0b01111111111111111111);
				string_appendChar(originalUTF16, 0b10000000000000000000);
				string_appendChar(originalUTF16, 0b11111111111111111111);
				// 21-bit test cases exploiting the high range offset
				string_appendChar(originalUTF16, 0x100000); // Using the 21:st bit
				string_appendChar(originalUTF16, 0x10FFFF); // Maximum range for UTF
				Buffer encoded = string_saveToMemory(originalUTF16, characterEncoding, lineEncoding);
				String decoded = string_loadFromMemory(encoded);
				//printBuffer(encoded);
				//compareCharacterCodes(originalUTF16, decoded);
				ASSERT_MATCH(originalUTF16, decoded);
			}
			// All UTF-16 characters excluding \r and \0
			for (int e = 0; e <= 1; e++) {
				CharacterEncoding characterEncoding = (e == 0) ? CharacterEncoding::BOM_UTF16BE : CharacterEncoding::BOM_UTF16LE;
				String original;
				string_reserve(original, 0x10FFFF);
				for (DsrChar c = 0x1; c <= 0xD7FF; c++) {
					if (c != U'\r') {
						string_appendChar(original, c);
					}
				}
				// 0xD800 to 0xDFFF is reserved for 
				for (DsrChar c = 0xE000; c <= 0x10FFFF; c++) {
					string_appendChar(original, c);
				}
				Buffer encoded = string_saveToMemory(original, characterEncoding, lineEncoding);
				String decoded = string_loadFromMemory(encoded);
				ASSERT_MATCH(original, decoded);
			}
		}
	}
	{ // Loading strings of different encodings
		String fileLatin1 = string_load(folderPath + U"Latin1.txt", true);
		//compareCharacterCodes(fileLatin1, expected_latin1);
		ASSERT_MATCH(fileLatin1, expected_latin1);

		String fileUTF8 = string_load(folderPath + U"BomUtf8.txt", true);
		//compareCharacterCodes(fileUTF8, expected_utf8);
		ASSERT_MATCH(fileUTF8, expected_utf8);

		String fileUTF16LE = string_load(folderPath + U"BomUtf16Le.txt", true);
		//compareCharacterCodes(fileUTF16LE, expected_utf16le);
		ASSERT_MATCH(fileUTF16LE, expected_utf16le);

		String fileUTF16BE = string_load(folderPath + U"BomUtf16Be.txt", true);
		//compareCharacterCodes(fileUTF16BE, expected_utf16be);
		ASSERT_MATCH(fileUTF16BE, expected_utf16be);
	}
	{ // Saving and loading text to files using every combination of character and line encoding
		String originalContent = U"Hello my friend\n你好我的朋友\n𐐷𤭢\n";
		String latin1Expected = U"Hello my friend\n??????\n??\n";
		String tempPath = folderPath + U"Temporary.txt";
		for (int l = 0; l < 2; l++) {
			LineEncoding lineEncoding = (l == 0) ? LineEncoding::CrLf : LineEncoding::Lf;

			// Latin-1 should store up to 8 bits correctly, and write ? for complex characters
			string_save(tempPath, originalContent, CharacterEncoding::Raw_Latin1, lineEncoding);
			String latin1Loaded = string_load(tempPath, true);
			//compareCharacterCodes(latin1Loaded, latin1Expected);
			ASSERT_MATCH(latin1Loaded, latin1Expected);

			// UFT-8 should store up to 21 bits correctly
			string_save(tempPath, unicodeContent, CharacterEncoding::BOM_UTF8, lineEncoding);
			ASSERT_MATCH(string_load(tempPath, true), unicodeContent);

			// UFT-16 should store up to 20 bits correctly
			string_save(tempPath, unicodeContent, CharacterEncoding::BOM_UTF16BE, lineEncoding);
			ASSERT_MATCH(string_load(tempPath, true), unicodeContent);
			string_save(tempPath, unicodeContent, CharacterEncoding::BOM_UTF16LE, lineEncoding);
			ASSERT_MATCH(string_load(tempPath, true), unicodeContent);
			string_save(tempPath, U"This file is used when testing text encoding.");
		}
	}
END_TEST
