// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
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

#include "../../../api/modelAPI.h"
#include "../Model.h" // TODO: Only use the public API

using namespace dsr;

struct Vertex_DMF1 {
	FVector3D position;
	FVector4D texCoord;
	FVector4D color;
	Vertex_DMF1() : position(FVector3D(0.0f)), texCoord(FVector4D(0.0f)), color(FVector4D(1.0f)) {}
};

struct Triangle_DMF1 {
	Vertex_DMF1 vertices[3];
	Triangle_DMF1() {}
};

struct Part_DMF1 {
	String textures[16];
	String shaderZero;
	int minDetailLevel = 0, maxDetailLevel = 2;
	List<Triangle_DMF1> triangles;
	String name;
	Part_DMF1() {}
	void addEmptyTriangle() {
		this->triangles.push(Triangle_DMF1());
	}
	Triangle_DMF1* getLastTriangle() {
		if (this->triangles.length() > 0) {
			return &(triangles[this->triangles.length() - 1]);
		} else {
			return nullptr;
		}
	}
};

struct Model_DMF1 {
	Filter filter = Filter::Solid;
	List<Part_DMF1> parts;
	Model_DMF1() {}
	Part_DMF1* getLastPart() {
		if (this->parts.length() > 0) {
			return &(parts[this->parts.length() - 1]);
		} else {
			return nullptr;
		}
	}
	void addEmptyPart() {
		this->parts.push(Part_DMF1());
	}
};

#define LoopForward(min, var, max) for(var = min; var <= max; var++)
#define PARSER_NOINDEX if (index != 0) { printText("This version of the engine does not have an index for the property ", propertyName, ".\n"); }
#define PROPERTY_MATCH(NAME) (string_caseInsensitiveMatch(propertyName, U ## #NAME))
#define CONTENT_MATCH(NAME) (string_caseInsensitiveMatch(content, U ## #NAME))

static const DsrChar tab = 9;
static const DsrChar space = 32;
static const DsrChar lineFeed = 10;
static const DsrChar carriageReturn = 13;

static const int ParserState_WaitForStatement = 0; // NameSpace -> WaitForStatement, Identifier -> WaitForIndexOrProperty
static const int ParserState_WaitForIndexOrProperty = 1; // index -> WaitForProperty, Property -> WaitForStatement
static const int ParserState_WaitForProperty = 2; // Property -> WaitForStatement

static const int ParserSpace_Main = 0;
static const int ParserSpace_Part = 1;
static const int ParserSpace_Triangle = 2;
static const int ParserSpace_Bone = 3;
static const int ParserSpace_Shape = 4;
static const int ParserSpace_Point = 5;
static const int ParserSpace_Unhandled = 6;

struct ParserState {
	Model_DMF1 *model;
	int parserState, parserSpace, propertyIndex;
	String lastPropertyName;
	explicit ParserState(Model_DMF1 *model) :
	  model(model),
	  parserState(ParserState_WaitForStatement),
	  parserSpace(ParserSpace_Main),
	  propertyIndex(0) {}
};

// Precondition: value >= 0.0
static int roundIndex(double value) {
	return (int)round(value);
}

static void setProperty(ParserState &state, const String &propertyName, int index, const ReadableString &content) {
	float value = (float)string_toDouble(content);
	if (state.parserSpace == ParserSpace_Main) {
		if PROPERTY_MATCH(FilterType) {
			PARSER_NOINDEX
			if CONTENT_MATCH(Alpha) {
				state.model->filter = Filter::Alpha;
			} else { // None
				state.model->filter = Filter::Solid;
			}
		} else if PROPERTY_MATCH(CullingType) {
			PARSER_NOINDEX
			if CONTENT_MATCH(AABB) {
				// TODO: Use bounding box culling on state.model
			} else if CONTENT_MATCH(Radius) {
				// TODO: Use centered sphere culling on state.model
			} else { // None
				// TODO: Use no culling on state.model
			}
		} else if PROPERTY_MATCH(BoundMultiplier) {
			PARSER_NOINDEX
			// TODO: Use bound multiplier exactly like in the other engine
			//state.model->ModelProperties.BoundMultiplier = value;
		}
	} else if (state.parserSpace == ParserSpace_Part) {
		Part_DMF1* lastPart = state.model->getLastPart();
		if (!lastPart) {
			printText("Failed to find the last part!\n");
		} else if PROPERTY_MATCH(Name) {
			PARSER_NOINDEX
			lastPart->name = content;
		} else if PROPERTY_MATCH(Texture) {
			if (index < 0 || index >= 16) {
				printText("Texture index ", index, " is out of bound 0..15\n");
			} else {
				lastPart->textures[index] = content;
			}
		} else if PROPERTY_MATCH(Shader) {
			if (index == 0) {
				lastPart->shaderZero = content;
			}
		} else if PROPERTY_MATCH(TextureOverride) {
			// TODO: Set the texture override channel
		} else if PROPERTY_MATCH(MinDetailLevel) {
			lastPart->minDetailLevel = roundIndex(value);
		} else if PROPERTY_MATCH(MaxDetailLevel) {
			lastPart->maxDetailLevel = roundIndex(value);
		}
	} else if (state.parserSpace == ParserSpace_Triangle) {
		Part_DMF1* lastPart = state.model->getLastPart();
		if (!lastPart) {
			printText("Failed to find the last part!\n");
		} else {
			Triangle_DMF1 *lastTriangle = lastPart->getLastTriangle();
			if (!lastTriangle) {
				printText("Cannot define vertex data after failing to create a triangle!\n");
			} else if (index < 0 || index > 2) {
				printText("Triangle vertex index ", index, " is out of bound 0..2!\n");
			} else {
				if PROPERTY_MATCH(X) {
					lastTriangle->vertices[index].position.x = value;
				} else if PROPERTY_MATCH(Y) {
					lastTriangle->vertices[index].position.y = value;
				} else if PROPERTY_MATCH(Z) {
					lastTriangle->vertices[index].position.z = value;
				} else if PROPERTY_MATCH(CR) {
					lastTriangle->vertices[index].color.x = value;
				} else if PROPERTY_MATCH(CG) {
					lastTriangle->vertices[index].color.y = value;
				} else if PROPERTY_MATCH(CB) {
					lastTriangle->vertices[index].color.z = value;
				} else if PROPERTY_MATCH(CA) {
					lastTriangle->vertices[index].color.w = value;
				} else if PROPERTY_MATCH(U1) {
					lastTriangle->vertices[index].texCoord.x = value;
				} else if PROPERTY_MATCH(V1) {
					lastTriangle->vertices[index].texCoord.y = value;
				} else if PROPERTY_MATCH(U2) {
					lastTriangle->vertices[index].texCoord.z = value;
				} else if PROPERTY_MATCH(V2) {
					lastTriangle->vertices[index].texCoord.w = value;
				}
			}
		}
	}
}

static void changeNamespace(ParserState &state, const String &newNamespace) {
	if (string_caseInsensitiveMatch(newNamespace, U"Part")) {
		// Create a new part
		state.model->addEmptyPart();
		state.parserSpace = ParserSpace_Part;
	} else if (string_caseInsensitiveMatch(newNamespace, U"Triangle")) {
		if (state.parserSpace == ParserSpace_Part || state.parserSpace == ParserSpace_Triangle) {
			// Create a new triangle
			state.model->getLastPart()->addEmptyTriangle();
			state.parserSpace = ParserSpace_Triangle;
		} else {
			printText("Triangles must be created as members of a part!\n");
		}
	} else if (string_caseInsensitiveMatch(newNamespace, U"Bone")) {
		state.parserSpace = ParserSpace_Bone; // A bone for animation
	} else if (string_caseInsensitiveMatch(newNamespace, U"Shape")) {
		state.parserSpace = ParserSpace_Shape; // A physical shape.
	} else if (string_caseInsensitiveMatch(newNamespace, U"Point")) {
		state.parserSpace = ParserSpace_Point; // A physical point in a convex hull.
	} else {
		state.parserSpace = ParserSpace_Unhandled;
	}
}

// Start and end are in base zero
// End is an inclusive index
static void readToken(ParserState &state, const String &fileContent, int start, int end) {
	if (end >= start) {
		if (fileContent[start] == U'(' && fileContent[end] == U')') {
			// Property
			if (state.parserState == ParserState_WaitForProperty || state.parserState == ParserState_WaitForIndexOrProperty) {
				setProperty(state, state.lastPropertyName, state.propertyIndex, string_inclusiveRange(fileContent, start + 1, end - 1));
				state.parserState = ParserState_WaitForStatement;
				state.propertyIndex = 0; // Reset index for the next property
			} else {
				printText("Unexpected property!\n");
			}
		} else if (fileContent[start] == U'[' && fileContent[end] == U']') {
			// Index
			if (state.parserState == ParserState_WaitForIndexOrProperty) {
				state.propertyIndex = roundIndex(string_toDouble(string_inclusiveRange(fileContent, start + 1, end - 1)));
			} else {
				printText("Unexpected index!\n");
			}
		} else if (fileContent[start] == U'<' && fileContent[end] == U'>') {
			// Namespace
			if (state.parserState == ParserState_WaitForStatement) {
				if (end - start > 258) {
					printText("Name of namespace is too long!\n");
				} else {
					// Change namespace and create things
					changeNamespace(state, string_inclusiveRange(fileContent, start + 1, end - 1));
				}
			} else {
				printText("Change of namespace before finishing the last statement!\n");
			}
		} else {
			// Identifier
			if (state.parserState == ParserState_WaitForStatement) {
				// Global property
				if (end - start > 258) {
					printText("Name of property is too long!\n");
				} else {
					state.lastPropertyName = string_inclusiveRange(fileContent, start, end);
					state.parserState = ParserState_WaitForIndexOrProperty;
				}
			}
		}
	}
}

// Parses a simplified native representation of the model as a syntax tree that can later be converted into a real model
static Model_DMF1 loadNative_DMF1(const String &fileContent) {
	Model_DMF1 resultModel;
	ParserState state(&resultModel);
	if (string_length(fileContent) < 4 || (fileContent[0] != 'D' || fileContent[1] != 'M' || fileContent[2] != 'F' || fileContent[3] != '1')) {
		printText("The file does not start with \"DMF1\"!\n");
		return resultModel;
	}
	int tokenStart = 4; // Everything before this will no longer be used
	int readIndex = 4;
	// Scan the string and send tokens to the state machine
	DsrChar firstCharOfToken = U'\0';
	for (readIndex = tokenStart; readIndex < string_length(fileContent); readIndex++) {
		DsrChar curChar = fileContent[readIndex];
		if (firstCharOfToken == U'\0' && (curChar == tab || curChar == space || curChar == lineFeed || curChar == carriageReturn)) {
			// Finish the current token and don't save this character
			readToken(state, fileContent, tokenStart, readIndex - 1);
			tokenStart = readIndex + 1;
		} else if (curChar == '<' || curChar == '(' || curChar == '[') {
			// Finish the last token and save this character
			readToken(state, fileContent, tokenStart, readIndex-1);
			tokenStart = readIndex;
			firstCharOfToken = curChar;
		} else if (firstCharOfToken == '<' && curChar == '>') {
			// Use this token
			readToken(state, fileContent, tokenStart, readIndex);
			tokenStart = readIndex + 1;
			firstCharOfToken = U'\0';
		} else if (firstCharOfToken == '(' && curChar == ')') {
			// Use this token
			readToken(state, fileContent, tokenStart, readIndex);
			tokenStart = readIndex + 1;
			firstCharOfToken = U'\0';
		} else if (firstCharOfToken == '[' && curChar == ']') {
			// Use this token
			readToken(state, fileContent, tokenStart, readIndex);
			tokenStart = readIndex + 1;
			firstCharOfToken = U'\0';
		}
	}
	readToken(state, fileContent, tokenStart, readIndex - 1);
	if (state.parserState != ParserState_WaitForStatement) {
		printText("The last statement in the model was not finished.\n");
	}
	return resultModel;
}

static Model convertFromDMF1(const Model_DMF1 &nativeModel, ResourcePool &pool, int detailLevel) {
	Model result = model_create();
	// Convert all parts from the native representation
	for (int inputPartIndex = 0; inputPartIndex < nativeModel.parts.length(); inputPartIndex++) {
		const Part_DMF1 *inputPart = &(nativeModel.parts[inputPartIndex]);
		if (detailLevel >= inputPart->minDetailLevel && detailLevel <= inputPart->maxDetailLevel) {
			int part = result->addEmptyPart(inputPart->name);
			if (string_caseInsensitiveMatch(inputPart->shaderZero, U"M_Diffuse_0Tex")) {
				// Color
			} else if (string_caseInsensitiveMatch(inputPart->shaderZero, U"M_Diffuse_1Tex")) {
				// Diffuse
				result->setDiffuseMapByName(pool, inputPart->textures[0], part);
			} else if (string_caseInsensitiveMatch(inputPart->shaderZero, U"M_Diffuse_2Tex")) {
				// Diffuse and light
				result->setDiffuseMapByName(pool, inputPart->textures[0], part);
				result->setLightMapByName(pool, inputPart->textures[1], part);
			} else {
				printText("The shader ", inputPart->shaderZero, " is not supported. Use M_Diffuse_0Tex, M_Diffuse_1Tex or M_Diffuse_2Tex.\n");
			}
			for (int inputTriangleIndex = 0; inputTriangleIndex < inputPart->triangles.length(); inputTriangleIndex++) {
				const Triangle_DMF1 *inputTriangle = &(inputPart->triangles[inputTriangleIndex]);
				const float threshold = 0.00001f;
				int posIndexA = result->addPointIfNeeded(inputTriangle->vertices[0].position, threshold);
				int posIndexB = result->addPointIfNeeded(inputTriangle->vertices[1].position, threshold);
				int posIndexC = result->addPointIfNeeded(inputTriangle->vertices[2].position, threshold);
				VertexData dataA(inputTriangle->vertices[0].texCoord, inputTriangle->vertices[0].color);
				VertexData dataB(inputTriangle->vertices[1].texCoord, inputTriangle->vertices[1].color);
				VertexData dataC(inputTriangle->vertices[2].texCoord, inputTriangle->vertices[2].color);
				Polygon polygon(Vertex(posIndexA, dataA), Vertex(posIndexB, dataB), Vertex(posIndexC, dataC));
				result->addPolygon(polygon, part);
			}
		}
	}
	return result;
}

Model dsr::importFromContent_DMF1(const String &fileContent, ResourcePool &pool, int detailLevel) {
	// Load the raw data
	Model_DMF1 nativeModel = loadNative_DMF1(fileContent);
	// Construct a model while loading resources
	return convertFromDMF1(nativeModel, pool, detailLevel);
}

