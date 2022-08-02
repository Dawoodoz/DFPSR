
/* TODO:
* Create syntax for automatically including links to outmost documents in a specified folder.
* Create syntax for automatically documenting methods in specified headers based on above comments.
  Follow include "", but not include <> when listing types.
*/

#include "../../Source/DFPSR/includeFramework.h"

using namespace dsr;

// This program is only for maintaining the library's documentation,
//   so it's okay to use features specific to the Linux operating system.

String resourceFolderPath;

bool string_beginsWith(const ReadableString &a, const ReadableString &b) {
	return string_caseInsensitiveMatch(string_before(a, string_length(b)), b);
}

String substituteCharacters(ReadableString text) {
	String result;
	for (int i = 0; i < string_length(text); i++) {
		DsrChar c = text[i];
		if (c == U'<') {
			string_append(result, U"&lt;");
		} else if (c == U'>') {
			string_append(result, U"&gt;");
		} else if (c == U'\\') {
			string_append(result, U"&bsol;");
		} else {
			string_appendChar(result, text[i]);
		}
	}
	return result;
}

bool codeBlock = false;
void processContent(String &target, String content) {
	string_split_callback([&target](ReadableString section) {
		//printText(U"Processing: ", section, U"\n");
		if (string_length(section) == 0) {
			//printText(U"    Break\n");+
			string_append(target, U"\n</P><P>\n");
		} else if (string_match(section, U"*")) {
			//printText(U"    Dot\n");
			string_append(target, U"<IMG SRC=\"Images/SmallDot.png\">\n");
		} else if (string_match(section, U"---")) {
			//printText(U"    Border\n");
			string_append(target, U"</P><IMG SRC=\"Images/Border.png\"><P>\n");
		} else if (string_beginsWith(section, U"<-")) {
			ReadableString arguments = string_from(section, 2);
			int splitIndex = string_findFirst(arguments, U'|');
			if (splitIndex > -1) {
				ReadableString link = string_removeOuterWhiteSpace(string_before(arguments, splitIndex));
				ReadableString text = string_removeOuterWhiteSpace(string_after(arguments, splitIndex));
				//printText(U"    Link to ", link, U" as ", text, U"\n");
				string_append(target, U"<A href=\"", link, "\">", text, "</A>");
			} else {
				//printText(U"    Link to ", arguments, U"\n");
				string_append(target, U"<A href=\"", arguments, "\">", arguments, "</A>");
			}
		} else if (string_beginsWith(section, U"Image:")) {
			ReadableString arguments = string_from(section, 6);
			int splitIndex = string_findFirst(arguments, U'|');
			if (splitIndex > -1) {
				ReadableString image = string_removeOuterWhiteSpace(string_before(arguments, splitIndex));
				ReadableString text = string_removeOuterWhiteSpace(string_after(arguments, splitIndex));
				//printText(U"    Image at ", image, U" as ", text, U"\n");
				string_append(target, U"<IMG SRC=\"", image, "\" ALT=\"", text, "\">\n");
			} else {
				//printText(U"    Image at ", arguments, U"\n");
				string_append(target, U"<IMG SRC=\"", arguments, "\" ALT=\"\">\n");
			}
		} else if (string_beginsWith(section, U"Title:")) {
			ReadableString title = string_from(section, 6);
			//printText(U"    Title: ", title, U"\n");
			string_append(target, U"</P><H1>", title, U"</H1><P>");
		} else if (string_beginsWith(section, U"Title2:")) {
			ReadableString title = string_from(section, 7);
			//printText(U"    Title2: ", title, U"\n");
			string_append(target, U"</P><H2>", title, U"</H2><P>");
		} else if (string_beginsWith(section, U"Title3:")) {
			ReadableString title = string_from(section, 7);
			//printText(U"    Title3: ", title, U"\n");
			string_append(target, U"</P><H3>", title, U"</H3><P>");
		} else if (string_beginsWith(section, U"CodeStart:")) {
			ReadableString code = string_from(section, 10);
			string_append(target, U"<PRE><BLOCKQUOTE>", substituteCharacters(code));
			codeBlock = true;
		} else if (string_beginsWith(section, U"CodeEnd:")) {
			string_append(target, U"</BLOCKQUOTE></PRE>");
			codeBlock = false;
		} else {
			if (codeBlock) {
				string_append(target, substituteCharacters(section), U"\n");
			} else {
				string_append(target, section, U"\n");
			}
		}
	}, content, U'\n');
}

String generateHtml(String content) {
	String style = string_load(file_combinePaths(resourceFolderPath, U"Default.css"));
	String result = U"<!DOCTYPE html> <HTML lang=en> <HEAD> <STYLE>\n";
	string_append(result, style);
	string_append(result, U"</STYLE> </HEAD> <BODY>\n");
	string_append(result, U"<IMG SRC=\"Images/Title.png\" ALT=\"Images/Title.png\">\n");
	string_append(result, U"<P>\n");
	processContent(result, content);
	string_append(result, U"</P>\n");
	string_append(result, U"</BODY> </HTML>\n");
	return result;
}

static ReadableString getExtensionless(const String& filename) {
	int lastDotIndex = string_findLast(filename, U'.');
	if (lastDotIndex != -1) {
		return string_removeOuterWhiteSpace(string_before(filename, lastDotIndex));
	} else {
		return U"?";
	}
}

void processFolder(const ReadableString& sourceFolderPath, const ReadableString& targetFolderPath) {
	file_getFolderContent(sourceFolderPath, [targetFolderPath](const ReadableString& sourcePath, const ReadableString& entryName, EntryType entryType) {
		printText("* Entry: ", entryName, " as ", entryType, "\n");
		if (entryType == EntryType::Folder) {
			// TODO: Create new output folders if needed for nested output.
			//processFolder(sourcePath, file_combinePaths(targetFolderPath, entryName));
		} else if (entryType == EntryType::File) {
			ReadableString extensionless = getExtensionless(entryName);
			String targetPath = file_combinePaths(targetFolderPath, extensionless + U".html");
			printText(U"Generating ", targetPath, U" from ", sourcePath, U" using the style ", resourceFolderPath, U"\n");
			String content = string_load(sourcePath);
			String result = generateHtml(content);
			string_save(file_combinePaths(targetFolderPath, targetPath), result);
		}
	});
}

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	if (args.length() != 4) {
		printText(U"The generator needs input, output and resource folder paths as three arguments!\n");
	} else {
		String sourceFolderPath = file_getAbsolutePath(args[1]);
		String targetFolderPath = file_getAbsolutePath(args[2]);
		resourceFolderPath = args[3];
		printText(U"Processing ", targetFolderPath, U" from ", sourceFolderPath, U" using the style ", resourceFolderPath, U"\n");
		processFolder(sourceFolderPath, targetFolderPath);
		printText(U"Done\n");
	}
}
