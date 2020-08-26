
#ifndef DFPSR_IMPORTER
#define DFPSR_IMPORTER

#include "../../../DFPSR/includeFramework.h"

namespace dsr {

// Returning a new model
Model importer_loadModel(const ReadableString& filename, bool flipX, Transform3D axisConversion);

// In-place loading of a new part
void importer_loadModel(Model& targetModel, int part, const ReadableString& filename, bool flipX, Transform3D axisConversion);

// To be applied to visible models after importing to save space in the files
// Side-effects: Generating smooth normals from polygon positions in model and packing the resulting (NX, NY, NZ) into (U1, V1, U2) texture coordinates
void importer_generateNormalsIntoTextureCoordinates(Model model);

}

#endif
