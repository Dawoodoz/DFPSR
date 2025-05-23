﻿// zlib open source license
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

#ifndef DFPSR_PERSISTENT_CLASSFACTORY
#define DFPSR_PERSISTENT_CLASSFACTORY

#include "../../api/stringAPI.h"
#include "../../api/fileAPI.h"
#include "../../collection/List.h"
#include <memory>

namespace dsr {

class Persistent;

// Reference method for creating a persistent class
inline dsr::Handle<Persistent> classConstructor() {
	return dsr::Handle<Persistent>(); // Null
}

// Must be used in each class inheriting from Persistent (both directly and indirectly)
#define PERSISTENT_DECLARATION(CLASS) \
	dsr::Handle<dsr::StructureDefinition> getStructure() const override; \
	decltype(&dsr::classConstructor) getConstructor() const override; \
	explicit CLASS(const dsr::ReadableString &content, const dsr::ReadableString &fromPath);

// Must be used in the implementation of each class inheriting from Persistent
#define PERSISTENT_DEFINITION(CLASS) \
	dsr::Handle<dsr::StructureDefinition> CLASS##Type; \
	dsr::Handle<dsr::StructureDefinition> CLASS::getStructure() const { \
		if (CLASS##Type.isNull()) { \
			CLASS##Type = dsr::handle_create<dsr::StructureDefinition>(U ## #CLASS).setName("Persistent " #CLASS " StructureDefinition"); \
			this->declareAttributes((CLASS##Type).getReference()); \
		} \
		return CLASS##Type; \
	} \
	CLASS::CLASS(const dsr::ReadableString &content, const dsr::ReadableString &fromPath) { \
		this->assignValue(content, fromPath); \
	} \
	dsr::Handle<dsr::Persistent> CLASS##Constructor() { \
		return dsr::handle_dynamicCast<dsr::Persistent>(dsr::handle_create<CLASS>().setName("Persistent " #CLASS)); \
	} \
	decltype(&dsr::classConstructor) CLASS::getConstructor() const { \
		return &CLASS##Constructor; \
	}

class PersistentAttribute {
public:
	String name; // The name of the attribute
public:
	PersistentAttribute() {}
	explicit PersistentAttribute(const String &name) : name(name) {}
};

class StructureDefinition {
public:
	// The name of the class
	String name;
	// All attributes in the data structure, including attributes inherited from the parent class
	List<PersistentAttribute> attributes;
public:
	StructureDefinition() {}
	explicit StructureDefinition(const String &name) : name(name) {}
	// TODO: Prevent adding the same name twice
	void declareAttribute(const String &name) {
		this->attributes.push(PersistentAttribute(name));
	}
	int length() {
		return this->attributes.length();
	}
};

class Persistent : public Printable {
public:
	// Persistent attributes may not be write protected
	virtual Persistent* findAttribute(const ReadableString &name);
	virtual Handle<StructureDefinition> getStructure() const;
	virtual decltype(&classConstructor) getConstructor() const = 0;
	// Call from the start of main, to allow constructing the class by name
	void registerPersistentClass();
	void setProperty(const ReadableString &key, const ReadableString &value, const ReadableString &fromPath);
	String getClassName() const;
public:
	// Override for non-atomic collection types
	//   Each child object will be constructed using Begin and End keywords directly inside of the parent
	//   For atomic collections, direct parsing and generation of comma separated lists would be more compact

	// Attempt to add another persistent object
	//   Return false if the child object was rejected
	//   Make sure that connections that would create an infinite loop are rejected
	virtual bool addChild(Handle<Persistent> child);
	virtual int getChildCount() const;
	virtual Handle<Persistent> getChild(int index) const;
public:
	// Override for new compound types

	// Override declareAttributes if your persistent structure has any variables to register as persistent
	// Use the MAKE_PERSISTENT macro with the attribute name for each persistent member
	// Each persistent attribute's type must also inherit from Persistent and start with the PERSISTENT_IMPL macro
	virtual void declareAttributes(StructureDefinition &target) const;
public:
	// Override for new atomic types

	// Assign content from a string
	//   Returns true on success and false if no assigment was made
	virtual bool assignValue(const ReadableString &content, const ReadableString &fromPath);
	
	// Save to a stream using any indentation
	virtual String& toStreamIndented(String& out, const ReadableString& indentation) const override;
};

// Macro to be placed at the start of the global main function
//   The dsr namespace must be used to access registerPersistentClass
#define REGISTER_PERSISTENT_CLASS(CLASS) \
(CLASS().registerPersistentClass());

// Create a single class instance without any content
Handle<Persistent> createPersistentClass(const String &type, bool mustExist = true);

// Create a class instance from text
Handle<Persistent> createPersistentClassFromText(const ReadableString &text, const ReadableString &fromPath);

}

#endif

