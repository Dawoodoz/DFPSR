
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
)QUOTE";
String expected_utf8 = unicodeContent + U"\nThis is UTF-8";
String expected_utf16le = unicodeContent + U"\nThis is UTF-16 Little Endian";
String expected_utf16be = unicodeContent + U"\nThis is UTF-8 Big Endian";

void printCharacterCode(uint32_t value) {
	for (int i = 0; i < 32; i++) {
		if (value & 0b10000000000000000000000000000000) {
			printText(U"1");
		} else {
			printText(U"0");
		}
		value = value << 1;
	}
}

// Method for printing the character codes of a string for debugging
void compareCharacterCodes(String textA, String textB) {
	int lengthA = string_length(textA);
	int lengthB = string_length(textB);
	int minLength = lengthA < lengthB ? lengthA : lengthB;
	printText("Character codes for strings of length ", lengthA, U" and ", lengthB, U":\n");
	for (int i = 0; i < minLength; i++) {
		uint32_t codeA = (uint32_t)textA[i];
		uint32_t codeB = (uint32_t)textB[i];
		printCharacterCode(codeA);
		if (codeA == codeB) {
			printText(U" == ");
		} else {
			printText(U" != ");
		}
		printCharacterCode(codeB);
		printText(U" (", textA[i], U") (", textB[i], U")\n");
	}
	if (lengthA > lengthB) {
		for (int i = minLength; i < lengthA; i++) {
			uint32_t codeA = (uint32_t)textA[i];
			printCharacterCode(codeA);
			printText(U" (", textA[i], U")\n");
		}
	} else {
		printText(U"                                    ");
		for (int i = minLength; i < lengthB; i++) {
			uint32_t codeB = (uint32_t)textB[i];
			printCharacterCode(codeB);
			printText(U" (", textB[i], U")\n");
		}
	}
}

START_TEST(TextEncoding)
	String folderPath = string_combine(U"test", file_separator(), U"tests", file_separator(), U"resources", file_separator());
	{ // Text encodings stored in memory
		// TODO: Test string_loadFromMemory
		
		
	}
	{ // Loading strings of different encodings
		String fileLatin1 = string_load(folderPath + U"Latin1.txt", true);
		//compareCharacterCodes(fileLatin1, expected_latin1);
		ASSERT_MATCH(fileLatin1, expected_latin1);

		String fileUTF8 = string_load(folderPath + U"BomUtf8.txt", true);
		compareCharacterCodes(fileUTF8, expected_utf8);
		ASSERT_MATCH(fileUTF8, expected_utf8);

		//String fileUTF16LE = string_load(folderPath + U"BomUtf16Le.txt", true);
		//printText("BomUtf16Le.txt contains:\n", fileUTF16LE, "\n");
		//ASSERT_MATCH(fileUTF16LE, expected_utf16le);

		//String fileUTF16BE = string_load(folderPath + U"BomUtf16Be.txt", true);
		//printText("BomUtf16Be.txt contains:\n", fileUTF16BE, "\n");
		//ASSERT_MATCH(fileUTF16BE, expected_utf16be);
	}
	{ // Saving text to files
		String originalContent = U"你好我的朋友";
		
		
	}
END_TEST
