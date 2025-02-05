
#include "../testTools.h"

// These tests will fail if the source code document or stored files change their encoding of line breaks.

String expected_latin1 =
R"QUOTE(Hello my friend
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

START_TEST(TextEncoding)
	String folderPath = file_combinePaths(U".", U"resources");
	// Check that we have a valid folder path to the resources.
	ASSERT_EQUAL(file_getEntryType(folderPath), EntryType::Folder);
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
				ASSERT_EQUAL(originalLatin1, decodedLatin1);
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
				ASSERT_EQUAL(originalUTF8, decodedUTF8);
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
				ASSERT_EQUAL(originalUTF16, decoded);
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
				ASSERT_EQUAL(original, decoded);
			}
		}
	}
	{ // Loading strings of different encodings
		String fileLatin1 = string_load(file_combinePaths(folderPath, U"Latin1.txt"), true);
		ASSERT_EQUAL(fileLatin1, expected_latin1);

		String fileUTF8 = string_load(file_combinePaths(folderPath, U"BomUtf8.txt"), true);
		ASSERT_EQUAL(fileUTF8, expected_utf8);

		String fileUTF16LE = string_load(file_combinePaths(folderPath, U"BomUtf16Le.txt"), true);
		ASSERT_EQUAL(fileUTF16LE, expected_utf16le);

		String fileUTF16BE = string_load(file_combinePaths(folderPath, U"BomUtf16Be.txt"), true);
		ASSERT_EQUAL(fileUTF16BE, expected_utf16be);
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
			ASSERT_EQUAL(latin1Loaded, latin1Expected);

			// UFT-8 should store up to 21 bits correctly
			string_save(tempPath, unicodeContent, CharacterEncoding::BOM_UTF8, lineEncoding);
			ASSERT_EQUAL(string_load(tempPath, true), unicodeContent);

			// UFT-16 should store up to 20 bits correctly
			string_save(tempPath, unicodeContent, CharacterEncoding::BOM_UTF16BE, lineEncoding);
			ASSERT_EQUAL(string_load(tempPath, true), unicodeContent);
			string_save(tempPath, unicodeContent, CharacterEncoding::BOM_UTF16LE, lineEncoding);
			ASSERT_EQUAL(string_load(tempPath, true), unicodeContent);
			string_save(tempPath, U"This file is used when testing text encoding.");
		}
	}
END_TEST
