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

#ifndef DFPSR_RENDER_CONSTANTS
#define DFPSR_RENDER_CONSTANTS

namespace dsr {

// Used for culling tests where visibility can be partial
enum class Visibility { Hidden, Full, Partial };

enum class Interpolation { NN, BL };

enum class Filter { Solid, Alpha };

// A set of global constants that should be easy to access without getting cyclic dependencies
namespace constants {

// Used for rasterization of triangles using integer math to prevent gaps
static const int32_t unitsPerPixel = 256;
static const int32_t unitsPerHalfPixel = unitsPerPixel / 2;

}

}

#endif

