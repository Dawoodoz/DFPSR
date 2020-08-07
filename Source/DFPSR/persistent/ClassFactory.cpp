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

#include "ClassFactory.h"

using namespace dsr;

// A global list of registered persistent classes
struct ConstructorInfo {
public:
	String type;
	decltype(&classConstructor) defaultConstructor;
public:
	ConstructorInfo(String type, decltype(&classConstructor) defaultConstructor) : type(type), defaultConstructor(defaultConstructor) {}
};
static List<ConstructorInfo> persistentClasses;

std::shared_ptr<StructureDefinition> Persistent::getStructure() const {
	return std::shared_ptr<StructureDefinition>();
}

static int findPersistentClass(const String &type) {
	for (int i = 0; i < persistentClasses.length(); i++) {
		if (string_match(persistentClasses[i].type, type)) {
			return i;
		}
	}
	return -1;
}

String Persistent::getClassName() const {
	return this->getStructure()->name;
}

void Persistent::registerPersistentClass() {
	int existingIndex = findPersistentClass(this->getClassName());
	// If a class of the name doesn't already exist
	if (existingIndex == -1) {
		// Register its constructor using the name
		persistentClasses.push(ConstructorInfo(this->getClassName(), this->getConstructor()));
	}
}

bool Persistent::addChild(std::shared_ptr<Persistent> child) {
	return false;
}

int Persistent::getChildCount() const {
	return 0;
}

std::shared_ptr<Persistent> Persistent::getChild(int index) const {
	return std::shared_ptr<Persistent>();
}

void Persistent::setProperty(const ReadableString &key, const ReadableString &value) {
	Persistent* target = this->findAttribute(key);
	if (target == nullptr) {
		printText("setProperty: ", key, " in ", this->getClassName(), " could not be found.\n");
	} else {
		if (!target->assignValue(value)) {
			printText("setProperty: The input ", value, " could not be assigned to property ", key, " because of incorrect format.\n");
		}
	}
}

Persistent* Persistent::findAttribute(const ReadableString &name) {
	return nullptr;
}

void Persistent::declareAttributes(StructureDefinition &target) const {}

bool Persistent::assignValue(const ReadableString &content) {
	printText("Warning! assignValue is not implemented for ", this->getClassName(), ".\n");
	return false;
}

String& Persistent::toStreamIndented(String& out, const ReadableString& indentation) const {
	std::shared_ptr<StructureDefinition> structure = this->getStructure();
	if (structure.get() == nullptr) {
		throwError(U"Failed to get the structure of a class being serialized.\n");
	}
	string_append(out, indentation, U"Begin : ", structure->name, U"\n");
	String nextIndentation = indentation + U"	";
	// Save parameters
	for (int i = 0; i < structure->length(); i++) {
		String name = structure->attributes[i].name;
		Persistent* value = ((Persistent*)this)->findAttribute(name); // Override const
		if (value == nullptr) {
			printText("Warning! ", name, " in ", structure->name, " was declared but not found from findAttribute.\n");
		} else {
			string_append(out, nextIndentation, name, U" = ");
			value->toStream(out);
			string_append(out, U"\n");
		}
	}
	// Save child objects
	for (int c = 0; c < this->getChildCount(); c++) {
		this->getChild(c)->toStreamIndented(out, nextIndentation);
	}
	string_append(out, indentation, U"End\n");
	return out;
}

std::shared_ptr<Persistent> dsr::createPersistentClass(const String &type, bool mustExist) {
	// Look for the component
	int existingIndex = findPersistentClass(type);
	if (existingIndex > -1) {
		return persistentClasses[existingIndex].defaultConstructor();
	}
	if (mustExist) {
		throwError(U"Failed to default create a class named ", type, U". Call registerPersistentClass on a temporary instance of the class to register the name.\n");
	}
	// Failed to load by name
	return std::shared_ptr<Persistent>(); // Null
}

std::shared_ptr<Persistent> dsr::createPersistentClassFromText(const ReadableString &text) {
	std::shared_ptr<Persistent> rootObject, newObject;
	List<std::shared_ptr<Persistent>> stack;
	List<ReadableString> lines = string_split(text, U'\n');
	for (int l = 0; l < lines.length(); l++) {
		ReadableString line = lines[l];
		int equalityIndex = string_findFirst(line, '=');
		if (equalityIndex > -1) {
			// Assignment
			String key = string_removeOuterWhiteSpace(string_before(line, equalityIndex));
			String value = string_removeOuterWhiteSpace(string_after(line, equalityIndex));
			stack.last()->setProperty(key, value);
		} else {
			int colonIndex = string_findFirst(line, ':');
			if (colonIndex > -1) {
				// Declaration
				String keyword = string_removeOuterWhiteSpace(string_before(line, colonIndex));
				if (string_caseInsensitiveMatch(keyword, U"Begin")) {
					String type = string_removeOuterWhiteSpace(string_after(line, colonIndex));
					newObject = dsr::createPersistentClass(type);
					if (rootObject.get() == nullptr) {
						rootObject = newObject;
					} else {
						if (!(stack.last()->addChild(newObject))) {
							throwError(U"Failed to add a child object!\n");
						}
					}
					stack.push(newObject);
				}
			} else {
				// Single keyword or empty line
				String keyword = string_removeOuterWhiteSpace(line);
				if (string_caseInsensitiveMatch(keyword, U"End")) {
					if (stack.length() > 0) {
						stack.pop();
					} else {
						throwError(U"Using end outside of root object!\n");
					}
				}
			}
		}
	}
	// Return the root component which is null on failure
	return rootObject;
}

