
#include "importer.h"

namespace dsr {

struct PlyProperty {
	String name;
	bool list;
	int scale = 1; // 1 for normalized input, 255 for uchar
	// Single property
	PlyProperty(String name, ReadableString typeName) : name(name), list(false) {
		if (string_caseInsensitiveMatch(typeName, U"UCHAR")) {
			this->scale = 255;
		} else {
			this->scale = 1;
		}
	}
	// List of properties
	PlyProperty(String name, ReadableString typeName, ReadableString lengthTypeName) : name(name), list(true) {
		if (string_caseInsensitiveMatch(typeName, U"UCHAR")) {
			this->scale = 255;
		} else {
			this->scale = 1;
		}
		if (string_caseInsensitiveMatch(lengthTypeName, U"FLOAT")) {
			printText("loadPlyModel: Using floating-point numbers to describe the length of a list is nonsense!\n");
		}
	}
};
struct PlyElement {
	String name; // Name of the collection
	int count; // Size of the collection
	List<PlyProperty> properties; // Properties on each line (list properties consume additional tokens)
	PlyElement(const String &name, int count) : name(name), count(count) {}
};
enum class PlyDataInput {
	Ignore, Vertex, Face
};
static PlyDataInput PlyDataInputFromName(const ReadableString& name) {
	if (string_caseInsensitiveMatch(name, U"VERTEX")) {
		return PlyDataInput::Vertex;
	} else if (string_caseInsensitiveMatch(name, U"FACE")) {
		return PlyDataInput::Face;
	} else {
		return PlyDataInput::Ignore;
	}
}
struct PlyVertex {
	FVector3D position = FVector3D(0.0f, 0.0f, 0.0f);
	FVector4D color = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
};
// When exporting PLY to this tool:
//   +X is right
//   +Y is up
//   +Z is forward
//   This coordinate system is left handed, which makes more sense when working with depth buffers.
// If exporting from a right-handed editor, setting Y as up and Z as forward might flip the X axis to the left side.
//   In that case, flip the X axis when calling this function.
static void loadPlyModel(Model& targetModel, int targetPart, const ReadableString& content, bool flipX, Transform3D axisConversion) {
	//printText("loadPlyModel:\n", content, "\n");
	// Find the target model
	int startPointIndex = model_getNumberOfPoints(targetModel);
	// Split lines
	List<String> lines = string_split(content, U'\n', true);
	List<PlyElement> elements;
	bool readingContent = false; // True after passing end_header
	int elementIndex = -1; // current member of elements
	int memberIndex = 0; // current data line within the content of the current element
	PlyDataInput inputMode = PlyDataInput::Ignore;
	// Temporary geometry
	List<PlyVertex> vertices;
	if (lines.length() < 2) {
		printText("loadPlyModel: Failed to identify line-breaks in the PLY file!\n");
		return;
	} else if (!string_caseInsensitiveMatch(string_removeOuterWhiteSpace(lines[0]), U"PLY")) {
		printText("loadPlyModel: Failed to identify the file as PLY!\n");
		return;
	} else if (!string_caseInsensitiveMatch(string_removeOuterWhiteSpace(lines[1]), U"FORMAT ASCII 1.0")) {
		printText("loadPlyModel: Only supporting the ascii 1.0 format!\n");
		return;
	}
	for (int l = 0; l < lines.length(); l++) {
		// Tokenize the current line
		List<String> tokens = string_split(lines[l], U' ');
		if (tokens.length() > 0 && !string_caseInsensitiveMatch(tokens[0], U"COMMENT")) {
			if (readingContent) {
				// Parse geometry
				if (inputMode == PlyDataInput::Vertex || inputMode == PlyDataInput::Face) {
					// Create new vertex with default properties
					if (inputMode == PlyDataInput::Vertex) {
						vertices.push(PlyVertex());
					}
					PlyElement *currentElement = &(elements[elementIndex]);
					int tokenIndex = 0;
					for (int propertyIndex = 0; propertyIndex < currentElement->properties.length(); propertyIndex++) {
						if (tokenIndex >= tokens.length()) {
							printText("loadPlyModel: Undeclared properties given to ", currentElement->name, " in the data!\n");
							break;
						}
						PlyProperty *currentProperty = &(currentElement->properties[propertyIndex]);
						if (currentProperty->list) {
							int listLength = string_toInteger(tokens[tokenIndex]);
							tokenIndex++;
							// Detect polygons
							if (inputMode == PlyDataInput::Face && string_caseInsensitiveMatch(currentProperty->name, U"VERTEX_INDICES")) {
								if (vertices.length() == 0) {
									printText("loadPlyModel: This ply importer does not support feeding polygons before vertices! Using vertices before defining them would require an additional intermediate representation.\n");
								}
								bool flipSides = flipX;
								if (listLength == 4) {
									// Use a quad to save memory
									int indexA = string_toInteger(tokens[tokenIndex]);
									int indexB = string_toInteger(tokens[tokenIndex + 1]);
									int indexC = string_toInteger(tokens[tokenIndex + 2]);
									int indexD = string_toInteger(tokens[tokenIndex + 3]);
									FVector4D colorA = vertices[indexA].color;
									FVector4D colorB = vertices[indexB].color;
									FVector4D colorC = vertices[indexC].color;
									FVector4D colorD = vertices[indexD].color;
									if (flipSides) {
										int polygon = model_addQuad(targetModel, targetPart,
										  startPointIndex + indexD,
										  startPointIndex + indexC,
										  startPointIndex + indexB,
										  startPointIndex + indexA
										);
										model_setVertexColor(targetModel, targetPart, polygon, 0, colorD);
										model_setVertexColor(targetModel, targetPart, polygon, 1, colorC);
										model_setVertexColor(targetModel, targetPart, polygon, 2, colorB);
										model_setVertexColor(targetModel, targetPart, polygon, 3, colorA);
									} else {
										int polygon = model_addQuad(targetModel, targetPart,
										  startPointIndex + indexA,
										  startPointIndex + indexB,
										  startPointIndex + indexC,
										  startPointIndex + indexD
										);
										model_setVertexColor(targetModel, targetPart, polygon, 0, colorA);
										model_setVertexColor(targetModel, targetPart, polygon, 1, colorB);
										model_setVertexColor(targetModel, targetPart, polygon, 2, colorC);
										model_setVertexColor(targetModel, targetPart, polygon, 3, colorD);
									}
								} else {
									// Polygon generating a triangle fan
									int indexA = string_toInteger(tokens[tokenIndex]);
									int indexB = string_toInteger(tokens[tokenIndex + 1]);
									FVector4D colorA = vertices[indexA].color;
									FVector4D colorB = vertices[indexB].color;
									for (int i = 2; i < listLength; i++) {
										int indexC = string_toInteger(tokens[tokenIndex + i]);
										FVector4D colorC = vertices[indexC].color;
										// Create a triangle
										if (flipSides) {
											int polygon = model_addTriangle(targetModel, targetPart,
											  startPointIndex + indexC,
											  startPointIndex + indexB,
											  startPointIndex + indexA
											);
											model_setVertexColor(targetModel, targetPart, polygon, 0, colorC);
											model_setVertexColor(targetModel, targetPart, polygon, 1, colorB);
											model_setVertexColor(targetModel, targetPart, polygon, 2, colorA);
										} else {
											int polygon = model_addTriangle(targetModel, targetPart,
											  startPointIndex + indexA,
											  startPointIndex + indexB,
											  startPointIndex + indexC
											);
											model_setVertexColor(targetModel, targetPart, polygon, 0, colorA);
											model_setVertexColor(targetModel, targetPart, polygon, 1, colorB);
											model_setVertexColor(targetModel, targetPart, polygon, 2, colorC);
										}
										// Iterate the triangle fan
										indexB = indexC;
										colorB = colorC;
									}
								}
							}
							tokenIndex += listLength;
						} else {
							// Detect vertex data
							if (inputMode == PlyDataInput::Vertex) {
								float value = string_toDouble(tokens[tokenIndex]) / (double)currentProperty->scale;
								// Swap X, Y and Z to convert from PLY coordinates
								if (string_caseInsensitiveMatch(currentProperty->name, U"X")) {
									if (flipX) {
										value = -value; // Right-handed to left-handed conversion
									}
									vertices[vertices.length() - 1].position.x = value;
								} else if (string_caseInsensitiveMatch(currentProperty->name, U"Y")) {
									vertices[vertices.length() - 1].position.y = value;
								} else if (string_caseInsensitiveMatch(currentProperty->name, U"Z")) {
									vertices[vertices.length() - 1].position.z = value;
								} else if (string_caseInsensitiveMatch(currentProperty->name, U"RED")) {
									vertices[vertices.length() - 1].color.x = value;
								} else if (string_caseInsensitiveMatch(currentProperty->name, U"GREEN")) {
									vertices[vertices.length() - 1].color.y = value;
								} else if (string_caseInsensitiveMatch(currentProperty->name, U"BLUE")) {
									vertices[vertices.length() - 1].color.z = value;
								} else if (string_caseInsensitiveMatch(currentProperty->name, U"ALPHA")) {
									vertices[vertices.length() - 1].color.w = value;
								}
							}
						}
						// Count one for a list size or single property
						tokenIndex++;
					}
					// Complete the vertex
					if (inputMode == PlyDataInput::Vertex) {
						FVector3D localPosition = vertices[vertices.length() - 1].position;
						model_addPoint(targetModel, axisConversion.transformPoint(localPosition));
					}
				}
				memberIndex++;
				if (memberIndex >= elements[elementIndex].count) {
					// Done with the element
					elementIndex++;
					memberIndex = 0;
					if (elementIndex >= elements.length()) {
						// Done with the file
						if (l < lines.length() - 1) {
							// Remaining lines will be ignored with a warning
							printText("loadPlyModel: Ignored ", (lines.length() - 1) - l, " undeclared lines at file end!\n");
						}
						return;
					} else {
						// Identify the next element by name
						inputMode = PlyDataInputFromName(elements[elementIndex].name);
					}
				}
			} else {
				if (tokens.length() == 1) {
					if (string_caseInsensitiveMatch(tokens[0], U"END_HEADER")) {
						readingContent = true;
						elementIndex = 0;
						memberIndex = 0;
						if (elements.length() < 2) {
							printText("loadPlyModel: Need at least two elements to defined faces and vertices in the model!\n");
							return;
						}
						// Identify the first element by name
						inputMode = PlyDataInputFromName(elements[elementIndex].name);
					}
				} else if (tokens.length() >= 3) {
					if (string_caseInsensitiveMatch(tokens[0], U"ELEMENT")) {
						elements.push(PlyElement(tokens[1], string_toInteger(tokens[2])));
						elementIndex = elements.length() - 1;
					} else if (string_caseInsensitiveMatch(tokens[0], U"PROPERTY")) {
						if (elementIndex < 0) {
							printText("loadPlyModel: Cannot declare a property without an element!\n");
						} else if (readingContent) {
							printText("loadPlyModel: Cannot declare a property outside of the header!\n");
						} else {
							if (tokens.length() == 3) {
								// Single property
								elements[elementIndex].properties.push(PlyProperty(tokens[2], tokens[1]));
							} else if (tokens.length() == 5 && string_caseInsensitiveMatch(tokens[1], U"LIST")) {
								// Integer followed by that number of properties as a list
								elements[elementIndex].properties.push(PlyProperty(tokens[4], tokens[3], tokens[2]));
							} else {
								printText("loadPlyModel: Unable to parse property!\n");
								return;
							}
						}
					}
				}
			}
		}
	}
}

void importer_loadModel(Model& targetModel, int part, const ReadableString& filename, bool flipX, Transform3D axisConversion) {
	int lastDotIndex = string_findLast(filename, U'.');
	if (lastDotIndex == -1) {
		printText("The model's filename ", filename, " does not have an extension!\n");
	} else {
		ReadableString extension = string_after(filename, lastDotIndex);
		if (string_caseInsensitiveMatch(extension, U"PLY")) {
			// Store the whole model file in a string for fast reading
			String content = string_load(filename);
			// Parse the file from the string
			loadPlyModel(targetModel, part, content, flipX, axisConversion);
		} else {
			printText("The extension ", extension, " in ", filename, " is not yet supported! You can implement an importer and call it from the loadModel function in tool.cpp.\n");
		}
	}
}

Model importer_loadModel(const ReadableString& filename, bool flipX, Transform3D axisConversion) {
	Model result = model_create();
	model_addEmptyPart(result, U"Imported");
	importer_loadModel(result, 0, filename, flipX, axisConversion);
	return result;
}

}
