
#ifndef DFPSR_IMPORTER
#define DFPSR_IMPORTER

#include "../../DFPSR/includeFramework.h"

namespace dsr {

// Returning a new model
Model importer_loadModel(const ReadableString& filename, bool flipX, Transform3D axisConversion);

// In-place loading of a new part
void importer_loadModel(Model& targetModel, int32_t part, const ReadableString& filename, bool flipX, Transform3D axisConversion);

}

#endif
