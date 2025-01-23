/*
Copyright (C) 2025 Miguel Castillo

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

#include <cstddef>
#include <cstdint>
#include <iostream>

// Starting seed values
#define SEED1 123456789
#define SEED2 987654321

class Xorshiftrplus {

public:
    static constexpr uint64_t primeNumbers[3] = {23, 41, 73};
    uint64_t nonce; // nonce for added entropy
    // Main seeds
    uint64_t seeds[2];

    //Buffers for prime numbers modulo operations
    uint64_t buff1[primeNumbers[0]];
    uint64_t buff2[primeNumbers[1]];
    uint64_t buff3[primeNumbers[2]];
    size_t index1 = 0;
    size_t index2 = 0;
    size_t index3 = 0;

    // default seed values constructor
    Xorshiftrplus() {
	seeds[0] = SEED1;
	seeds[1] = SEED2;
	nonce = 0;

	for(size_t i = 0 ; i < primeNumbers[0] ; i++) buff1[i] = initGenerate();
	for(size_t i = 0 ; i < primeNumbers[1] ; i++) buff2[i] = initGenerate();
	for(size_t i = 0 ; i < primeNumbers[2] ; i++) buff3[i] = initGenerate();
    }

    uint64_t generate(){

	uint64_t a = buff1[index1];
	uint64_t b = buff2[index2];
	uint64_t c = buff3[index3];

	// Index incrementation
	index1 = (index1 + 1) % primeNumbers[0];
	index2 = (index2 + 1) % primeNumbers[1];
	index3 = (index3 + 1) % primeNumbers[2];

	uint64_t result = a + b * c + nonce;
	nonce++;

	buff1[index1] = result ^ initGenerate();
	buff2[index2] = result ^ initGenerate();
	buff3[index3] = result ^ initGenerate();
	
	return result;
	
    }

private:
    // Initial random number generator implementing xorshiftr+
    uint64_t initGenerate() {
	uint64_t x = this->seeds[0];
	const uint64_t y = this->seeds[1];

	this->seeds[0] = y;
	x ^= x << 23;
	x ^= x >> 17;
	x ^= y;
	this->seeds[1] = x + y;

	return x;
    }
};

int main(){
    Xorshiftrplus generator;

    for(size_t i = 0 ; i < 1000 ; i++){
	std::cout << generator.generate() << std::endl;
    }
}
