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

Handle<StructureDefinition> Persistent::getStructure() const {
	return Handle<StructureDefinition>();
}

static int32_t findPersistentClass(const String &type) {
	for (int32_t i = 0; i < persistentClasses.length(); i++) {
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
	int32_t existingIndex = findPersistentClass(this->getClassName());
	// If a class of the name doesn't already exist
	if (existingIndex == -1) {
		// Register its constructor using the name
		persistentClasses.push(ConstructorInfo(this->getClassName(), this->getConstructor()));
	}
}

bool Persistent::addChild(Handle<Persistent> child) {
	return false;
}

int32_t Persistent::getChildCount() const {
	return 0;
}

Handle<Persistent> Persistent::getChild(int32_t index) const {
	return Handle<Persistent>();
}

void Persistent::setProperty(const ReadableString &key, const ReadableString &value, const ReadableString &fromPath) {
	Persistent* target = this->findAttribute(key);
	if (target == nullptr) {
		printText(U"setProperty: ", key, U" in ", this->getClassName(), U" could not be found.\n");
	} else {
		if (!target->assignValue(value, fromPath)) {
			printText(U"setProperty: The input ", value, U" could not be assigned to property ", key, U" because of incorrect format.\n");
		}
	}
}

Persistent* Persistent::findAttribute(const ReadableString &name) {
	return nullptr;
}

void Persistent::declareAttributes(StructureDefinition &target) const {}

bool Persistent::assignValue(const ReadableString &content, const ReadableString &fromPath) {
	printText(U"Warning! assignValue is not implemented for ", this->getClassName(), U".\n");
	return false;
}

String& Persistent::toStreamIndented(String& out, const ReadableString& indentation) const {
	Handle<StructureDefinition> structure = this->getStructure();
	if (structure.isNull()) {
		throwError(U"Failed to get the structure of a class being serialized.\n");
	}
	string_append(out, indentation, U"Begin : ", structure->name, U"\n");
	String nextIndentation = indentation + U"	";
	// Save parameters
	for (int32_t i = 0; i < structure->length(); i++) {
		String name = structure->attributes[i].name;
		Persistent* value = ((Persistent*)this)->findAttribute(name); // Override const
		if (value == nullptr) {
			printText(U"Warning! ", name, U" in ", structure->name, U" was declared but not found from findAttribute.\n");
		} else {
			string_append(out, nextIndentation, name, U" = ");
			value->toStream(out);
			string_append(out, U"\n");
		}
	}
	// Save child objects
	for (int32_t c = 0; c < this->getChildCount(); c++) {
		this->getChild(c)->toStreamIndented(out, nextIndentation);
	}
	string_append(out, indentation, U"End\n");
	return out;
}

Handle<Persistent> dsr::createPersistentClass(const String &type, bool mustExist) {
	// Look for the component
	int32_t existingIndex = findPersistentClass(type);
	if (existingIndex > -1) {
		return persistentClasses[existingIndex].defaultConstructor();
	}
	if (mustExist) {
		throwError(U"Failed to default create a class named ", type, U". Call registerPersistentClass on a temporary instance of the class to register the name.\n");
	}
	// Failed to load by name
	return Handle<Persistent>(); // Null
}

Handle<Persistent> dsr::createPersistentClassFromText(const ReadableString &text, const ReadableString &fromPath) {
	Handle<Persistent> rootObject, newObject;
	List<Handle<Persistent>> stack;
	string_split_callback([&rootObject, &newObject, &stack, &fromPath](ReadableString line) {
		int32_t equalityIndex = string_findFirst(line, '=');
		if (equalityIndex > -1) {
			// Assignment
			String key = string_removeOuterWhiteSpace(string_before(line, equalityIndex));
			String value = string_removeOuterWhiteSpace(string_after(line, equalityIndex));
			stack.last()->setProperty(key, value, fromPath);
		} else {
			int32_t colonIndex = string_findFirst(line, ':');
			if (colonIndex > -1) {
				// Declaration
				String keyword = string_removeOuterWhiteSpace(string_before(line, colonIndex));
				if (string_caseInsensitiveMatch(keyword, U"Begin")) {
					String type = string_removeOuterWhiteSpace(string_after(line, colonIndex));
					newObject = createPersistentClass(type);
					if (rootObject.isNull()) {
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
	}, text, U'\n');
	// Return the root component which is null on failure
	return rootObject;
}

