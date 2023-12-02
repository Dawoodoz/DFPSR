// zlib open source license
//
// Copyright (c) 2018 to 2023 David Forsgren Piuva
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

#ifndef DFPSR_GUI_COMPONENTSTATES
#define DFPSR_GUI_COMPONENTSTATES

namespace dsr {

// Bit flags for component states.
//  The size of ComponentState may change if running out of bits for new flags.
//  Each state should have a direct state and an indirect state, so that bitwise operations can be used to scan all states at once.
using ComponentState = uint32_t;
static const ComponentState componentState_focusDirect = 1 << 0; // Component being directly focused.
static const ComponentState componentState_focusIndirect = 1 << 1; // Contains the component being focused.
static const ComponentState componentState_hoverDirect = 1 << 2; // Component being hovered.
static const ComponentState componentState_hoverIndirect = 1 << 3; // Contains the component being hovered.
static const ComponentState componentState_showingOverlayDirect = 1 << 4; // The component will have drawOverlay called, if also visible.
static const ComponentState componentState_showingOverlayIndirect = 1 << 5; // The component contains a component drawing overlays.
static const ComponentState componentState_focus = componentState_focusDirect | componentState_focusIndirect; // Direct or indirect focus.
static const ComponentState componentState_hover = componentState_hoverDirect | componentState_hoverIndirect; // Direct or indirect hover.
static const ComponentState componentState_showingOverlay = componentState_showingOverlayDirect | componentState_showingOverlayIndirect; // Direct or indirect overlay.
static const ComponentState componentState_direct   = 0b01010101010101010101010101010101;
static const ComponentState componentState_indirect = 0b10101010101010101010101010101010;

}

#endif

