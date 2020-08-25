
#include "../../Source/DFPSR/includeFramework.h"

using namespace dsr;

// This program is only for maintaining the library's documentation,
//   so it's okay to use features specific to the Linux operating system.

String ResourceFolderPath;

bool string_beginsWith(const ReadableString &a, const ReadableString &b) {
	return string_caseInsensitiveMatch(string_before(a, string_length(b)), b);
}

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
		} else if (string_beginsWith(section, U"Code:")) {
            // TODO: Find a clean syntax for multi-line quotes in <PRE><BLOCKQUOTE>...</PRE></BLOCKQUOTE>
			ReadableString code = string_from(section, 5);
			//printText(U"    Code: ", code, U"\n");
			string_append(target, U"<blockquote>", code, U"</blockquote>");
		} else {
			string_append(target, section, U"\n");
		}
	}, content, U'\n');
}

String generateHtml(String content) {
	String style = string_load(ResourceFolderPath + U"Default.css");
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

int main(int argn, char **argv) {
	if (argn != 4) {
		printText(U"The generator needs filenames for target!\n");
		return 1;
	}
	String SourceFilePath = argv[1];
	String TargetFilePath = argv[2];
	ResourceFolderPath = argv[3];

	printText(U"Generating ", TargetFilePath, U" from ", SourceFilePath, U" using the style", ResourceFolderPath, U"\n");
	
	String content = string_load(SourceFilePath);
	String result = generateHtml(content);
	string_save(TargetFilePath, result);
	printText(U"Done\n");
	return 0;
}
