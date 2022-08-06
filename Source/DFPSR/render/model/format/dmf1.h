// zlib open source license
//
// Copyright (c) 2017 to 2022 David Forsgren Piuva
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

#ifndef DFPSR_RENDER_MODEL_IMPORT_DMF1
#define DFPSR_RENDER_MODEL_IMPORT_DMF1

#include "../../ResourcePool.h"
#include "../../../api/stringAPI.h"

namespace dsr {

Model importFromContent_DMF1(const String &fileContent, ResourcePool &pool, int detailLevel);

}

#endif
