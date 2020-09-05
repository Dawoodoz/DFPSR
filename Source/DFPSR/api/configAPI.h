// zlib open source license
//
// Copyright (c) 2018 to 2019 David Forsgren Piuva
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

#ifndef DFPSR_API_CONFIG
#define DFPSR_API_CONFIG

#include "stringAPI.h"
#include <functional>

namespace dsr {
	// A type of function sending (Block, Key, Value) to the caller.
	//   One can have hard-coded options, lookup-tables, dictionaries, et cetera for looking up the given key names.
	using ConfigIniCallback = std::function<void(const ReadableString&, const ReadableString&, const ReadableString&)>;
	/*
		Parsing the given content of a *.ini configuration file.
		Sending callbacks to receiverLambda for each key being assigned a value.
		  * If there's any preceding [] block, the content of the last preceding block will be given as the first argument.
		  * The key will be sent as the second argument.
		  * The value will be sent as the third argument.
		Example:
			config_parse_ini(content, [this](const ReadableString& block, const ReadableString& key, const ReadableString& value) {
				if (block.length() == 0) {
					if (string_caseInsensitiveMatch(key, U"A")) {
						this->valueA = string_parseInteger(value);
					} else if (string_caseInsensitiveMatch(key, U"B")) {
						this->valueB = string_parseInteger(value);
					} else {
						printText("Unrecognized key \"", key, "\" in A&B value configuration file.\n");
					}
				} else {
					printText("Unrecognized block \"", block, "\" in A&B value configuration file.\n");
				}
			});
	*/
	void config_parse_ini(const ReadableString& content, ConfigIniCallback receiverLambda);

	// Adding an ini generator might be convenient for complying with the *.ini file standard
	// but it would also take away some artistic freedom with how lines are indented
	// and it's not really difficult to generate a few assignments manually.
}

#endif

