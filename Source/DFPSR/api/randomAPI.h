/*
Copyright (C) 2025 Miguel Castillo
Reviewed and adapted 2025 to 2026 by David Forsgren Piuva

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
*/

#ifndef DFPSR_API_RANDOM
#define DFPSR_API_RANDOM

#include <cstdint>

// A random generator.
struct RandomGenerator {
	// The state makes sure that we get very different values each time the random generator is called.
	uint64_t impl_state[2] = {};
	// The indices rotate in a set of 68839 combination of indices using unique prime numbers.
	//   23 * 41 * 73 = 68839
	//   This shakes the state with a limited source of true entropy repeated in a loop, because patterns are created each time a number is repeated.
	uint32_t impl_index[3] = {};
	// The nonce rotates in a set of 2⁶⁴ values to be absolutely sure that the state does not get stuck in a narrow loop.
	uint64_t impl_nonce = 7245416u;
	RandomGenerator() {}
};

// Returns a new random generator initialized by seed.
RandomGenerator random_createGenerator(uint64_t seed);

// The raw output from the generator without any limits.
uint64_t random_generate_U64(RandomGenerator &generator);

// Pre-condition: minimum <= maximum.
int32_t random_generate_range(RandomGenerator &generator, int32_t minimum, int32_t maximum);
float random_generate_range(RandomGenerator &generator, float minimum, float maximum);

// Returns true for roughly perCentProbability times in a hundred.
bool random_generate_probability(RandomGenerator &generator, int32_t perCentProbability);

#endif
