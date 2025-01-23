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

// Starting seed values
#define SEED1 124523689
#define SEED2 987654321

class Xorshiftrplus {

public:
    static constexpr uint64_t primeNumbers[3] = {23, 41, 73};
    uint64_t nonce; // nonce for added entropy
    // Main seeds
    uint64_t seeds[2];

    //Buffers for prime numbers modulo operations
    uint64_t buff1[primeNumbers[0]]{
	0xA9D5F38E6B72C104, 0x4B7F93E5C28A61D0, 0xE2C7A9F5B36D1048,
	0x8D6A47C5B3E29F10, 0x5F38C9D6A47E210B, 0xC49A7E5D3B6F2081,
	0x3D6C5A9F7E42B108, 0xB6F49A3D7C5E2108, 0x1A7F6D3C49E5B208,
	0xF4C2A9D6B73E1085, 0x6A7B5F9C3D28E401, 0xD3A5B49E7C26F108,
	0x7E4A5D3F9C28B601, 0xC9A4F5D3B67E1082, 0x3B9F7E4D6A5C2108,
	0x2F5C7A6D49E3B108, 0xA6D3B9C5F7E42108, 0xE4A9D6F3C57B2018,
	0xF5C3D6A9E4B72108, 0x8B3D7A5F9C62E410, 0x6E7C49F5D2B3A108,
	0xD9A6F3C5B47E2108, 0x4A5C7E9D3B62F108};
    uint64_t buff2[primeNumbers[1]]{
	0xB38A24C9E0F153DC, 0x1A37C49A2B7F4D6E, 0x4F8B13D7A92E56CF,
	0xE9AB47C1D3F8057A, 0x7BC5A18E46F92D3B, 0x3FA9D8B7426CE8F0,
	0xACD35F9E82B471C6, 0x59EF38A7D62C9B01, 0xD2B18F47E39C6A5D,
	0x8EA57D4C19B23F60, 0x4C9FA8D36E57B2E1, 0xF725D39B68AC104E,
	0x6B38A9CF27E5D140, 0xC9B4A72F36E8D50A, 0x3F7DAE619C5B2048,
	0x1AE493B7C85D621F, 0x52CFA64D39E8B701, 0xD73B9C5A128E40F6,
	0x7F45A9D38C26E10B, 0xE14FA732D6B58C90, 0x8B63D19FA2C40E57,
	0x4AD3E29C58B7601F, 0x9F6A2B4E183C50D7, 0xB17F3D8A96C45E20,
	0x2CFA93E68B7D5014, 0x7B38D5A1C92F4E06, 0xD19C6A4F85E372B8,
	0xF26A3C7E48B5910D, 0x39C1D47A6F85B20E, 0x8A57D3B9E146C205,
	0xE4B6A3F78C29D10F, 0x1D9C5E4A72F38B60, 0xB7A18E6F9C435D20,
	0x5C7FA9B28D34E160, 0xACD5E47B39F28C10, 0x9F83B2A7D4C16E50,
	0x3E7A5F49C1B86D20, 0xD4B7A9C3F85E6210, 0x2A8F57E49D36C150,
        0x6F7A2C9B85D41E30, 0xB28D4C5E7A9F1360};
    uint64_t buff3[primeNumbers[2]]{
        0xA1D3F6B8C72E5940, 0x5C9F3E7B21D46A08, 0x7D1A5B9E46F82C30,
        0xE25A6F3C48B9D710, 0xF7A9D5B2E16C3048, 0x3C92F8A5D47E1B60,
        0x1D4B6F2C9E78A350, 0x9F4C7A2D3B5E1860, 0x6A7D5F9C32B48E10,
        0xC3A8E49F5D7B2160, 0xB4F9C57D2A6E3018, 0x8A2E6F39C57D410B,
        0xF18A7C49D52E36B0, 0x2C3A57D9E46B108F, 0xE7B58C3D4A29F601,
        0x1A49F3C57D2E6B80, 0xD7F8C4E5B29A3601, 0x6E2D3B9F47C58A10,
        0xB5F29C4D7A63E108, 0xA7C6F3E5B49D2108, 0x3F2D4B6E9A58C701,
        0xE49A7F5B3C26D108, 0x7D9F2A6B4C58E301, 0xF6E3C7A9B25D4108,
        0x1B48D5C29F76E301, 0x2F6B3C9A7D48E501, 0xD49F6A5B7C23E108,
        0x8B2C5A9F47D6E310, 0xC47E9B5A2D6F8103, 0x3E6A5B49D7C2F108,
        0xA5B49F3E27D6C108, 0xE27C5A9F3B46D108, 0x9F3D4B6A2E5C7810,
        0x5C8B2A6F47D9E301, 0xF4C2A7E9B5D6F108, 0x6D5A3F2B9E47C801,
        0xC9A47F5B6D23E801, 0x3A5D9F7C2B48E601, 0x8E2C7A5F49D3B601,
        0x1F6B3A9D47C5E208, 0xB4F9C57E2D6A1083, 0xD7C4A5F39B2E6810,
        0x2E6A3B5C9D48F701, 0xA9B47F5D2C36E108, 0x8F6A3B5E27D49C10,
        0xE4C7A3F59B2D6108, 0x3F9D6A5B4C27E108, 0xB27F5D4C9E36A108,
        0x7A5F3B9D2C48E601, 0xC3D6A5B49F27E108, 0xF5B9C4A7D26E3010,
        0xD3B5C7A9F26E4108, 0x6B2A9F47C5D3E108, 0x4A7F9D6B5C23E108,
        0x5C9D4B7A6E2F1083, 0x8D6A3C5B47F9E108, 0x9A7B4F5D26C3E801,
        0xE7F3D5C9A26B4018, 0xA6B4C9F27D5E1083, 0x3E9A6D5B4F27C108,
        0x7A3B5D6F49C2E801, 0xC5B9E47F2A6D3108, 0xF4D6C7B9E2A51083,
        0x8E4C9A5B7D3F2108, 0x6A5F9B3D7C28E101, 0x2D9C7A3B5F48E601,
        0x1B6F3C5D9A27E408, 0xD7A49B3F6C58E210, 0xC4A6F9B7D3E58C20,
        0xF5D4A3B9C76E28F1, 0x3F9D6A5B4C7E2A10};

    size_t index1 = 0;
    size_t index2 = 0;
    size_t index3 = 0;

    // default seed values constructor
    Xorshiftrplus() {
	seeds[0] = SEED1;
	seeds[1] = SEED2;
	nonce = 0;

    }

    uint64_t generate(){

	uint64_t a = buff1[index1];
	uint64_t b = buff2[index2];
	uint64_t c = buff3[index3];

	// Index incrementation
	index1++;
	if (index1 >= primeNumbers[0]) index1 = 0u;
	index2++;
	if (index2 >= primeNumbers[1]) index2 = 0u;
	index3++;
	if (index3 >= primeNumbers[2]) index3 = 0u;

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
