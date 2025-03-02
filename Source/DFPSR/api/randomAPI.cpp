/*
Copyright (C) 2025 Miguel Castillo
Reviewed and adapted by David Forsgren Piuva

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

#include "randomAPI.h"
#include <cstddef>

// By having three buffers of unique prime number lengths, we can reuse
//   the same entropy for longer when looping over them in modulo of prime numbers.
static const uint32_t primeA = 23;
static const uint32_t primeB = 41;
static const uint32_t primeC = 73;
static const uint64_t entropySourceA[primeA]{
	0x047A9215084C5274, 0x2190597518265E01, 0x6309218F74086502,
	0xC6798DE298345963, 0x846B57382919049D, 0x84710C2987A56082,
	0x01658E63202758B9, 0x3717F65728A28164, 0x470F29187482650E,
	0x325D7694E6328627, 0x8509163276904542, 0x23984729D06C9847,
	0x845378668563782E, 0x36699BC017501765, 0x1082764502893740,
	0x61430017835D4791, 0x9262D38057234098, 0x5871326487618271,
	0x0B976428F1018759, 0x7123876520928740, 0x5308761287560187,
	0x72525749301E8567, 0x123642E085710874};
static const uint64_t entropySourceB[primeB]{
	0x651EB08756138047, 0x4716F984C7156384, 0x76105374562C8379,
	0x5C162A3932818456, 0x57E16208756D0187, 0x4EB5F62897361423,
	0x2D03478203156081, 0x012847560285A205, 0xC456D0187A640587,
	0x62038F5DE1F20817, 0x6F103B4C71205762, 0x61E087F608760837,
	0x560198F473C08576, 0x10D52F8750813765, 0x6056320874098543,
	0x193D02C865761082, 0x0827350B82375624, 0x0E87542098431A87,
	0x735694025698D470, 0x7560182756023897, 0xC9846F712035FF6E,
	0xB541287346293485, 0x138476BC10238472, 0x81347560812763F2,
	0x73460A8602837A15, 0x1856127F561D2837, 0x9286108456F87560,
	0x608560A1837651B0, 0x56A1922746021785, 0x7FC16C4872340817,
	0x2FE84715304E8175, 0x346087C561037610, 0xB827456092875082,
	0x2609587263E859A2, 0x3485610387415684, 0x1C82374590283650,
	0xC570897F18726508, 0x4B781DEF56C03295, 0x32748012356834B6,
	0x2031571603485708, 0x069187320813260F};
static const uint64_t entropySourceC[primeC]{
	0x1364085D71360847, 0x46F081C734560817, 0xA56C1D0BE2F8358C,
	0xA16823746182D1F8, 0xB230591826585081, 0x92D7057BAE1C267D,
	0x07823C5687DEA16B, 0x9827309168710481, 0x5623F94871FBD60E,
	0x05716847601FB986, 0x6082375608876A78, 0x32857CE96BA48FD7,
	0x3082B1E763502375, 0x34E71B5602837509, 0x56192A8C75FED6B0,
	0x640591620348BB76, 0x386C701287B56023, 0xF1287E5623AD4F76,
	0x2806408A27501872, 0x4708D46591247385, 0x5CDB921E75062C3F,
	0x7308B4B760891237, 0x80347512A87568B1, 0xDA8752E96B8D7145,
	0x40B82368073B5C60, 0x1B9AF082D7E3CA45, 0x6A0DE2C87F4560DE,
	0x9127640812D73408, 0x028E63FB0412D86C, 0x1B276ACF50281746,
	0x96804C7D12630517, 0x9B5701AF39E4A8FD, 0x91DB2E83ACF75608,
	0xD260851723468344, 0x701C83B57DEC280F, 0xE4D7F13B0857AC61,
	0x30856120895308E9, 0x18273B5602A987DF, 0x0D7E50F6287B56D1,
	0x13680A7360E853F4, 0x1E0613A4B8C5E2D3, 0xE3807C4A5FB6D207,
	0x60D95628765CEB08, 0xF74598F37450BEA1, 0x65A08EC1F273D56B,
	0x34765039F4E88307, 0x6281D5CF6230712E, 0x89E13476FC5AD908,
	0x6508E17236487653, 0x35D8CA93B470569A, 0x3E779CF8235BDA81,
	0x0713C460D5826309, 0xF3B1D84EFC758273, 0xE072F63C5098B363,
	0xC482685076C3B408, 0x0CA8E65BD27F0982, 0x7BA481C4F723F6E2,
	0x7D1058971F2603D8, 0xE46158A3C0D4827B, 0x1B3751C8E6FA9730,
	0x56108750987618B6, 0x8512F3D65EA0C2B5, 0x3A048B6F41C87A95,
	0x856B043867143876, 0x60A824ED7F652B19, 0xC5A840178ED763A8,
	0xC230982519032857, 0x73C46082E13DC1FA, 0x792BD59F56A83069,
	0x6120560129851C03, 0x75603B4E65D1F847};

RandomGenerator random_createGenerator(uint64_t seed) {
	RandomGenerator generator = RandomGenerator();
	generator.impl_state[0] = 0x2F6B3C9A7D48E50 ^ seed;
	generator.impl_state[1] = 0xA7B42948581A283 ^ seed;
	return generator;
}

// Based on XorShiftR, with added nonce and constant entropy source.
uint64_t random_generate_U64(RandomGenerator &generator) {
	generator.impl_index[0]++;
	if (generator.impl_index[0] >= primeA) generator.impl_index[0] = 0u;
	generator.impl_index[1]++;
	if (generator.impl_index[1] >= primeB) generator.impl_index[1] = 0u;
	generator.impl_index[2]++;
	if (generator.impl_index[2] >= primeC) generator.impl_index[2] = 0u;
	uint64_t x = generator.impl_state[0];
	uint64_t y = generator.impl_state[1];
	x ^= x << 23;
	x ^= x >> 17;
	x ^= y
	   + entropySourceA[generator.impl_index[0]] * entropySourceB[generator.impl_index[1]]
	   + entropySourceC[generator.impl_index[2]]
	   + generator.impl_nonce;
	generator.impl_nonce++;
	generator.impl_state[1] = x + y;
	generator.impl_state[0] = y;
	return generator.impl_state[0] + generator.impl_state[1];
}

// Because modulo does not give a perfectly random distribution, we need to take modulo of a 64 bit value in order to get an even enough distribution for 32-bit values.
int32_t random_generate_range(RandomGenerator &generator, int32_t minimum, int32_t maximum) {
	return minimum + int32_t(random_generate_U64(generator) % (uint64_t(maximum) - uint64_t(minimum) + 1u));
}

float random_generate_range(RandomGenerator &generator, float minimum, float maximum) {
	double normalized = double(random_generate_U64(generator)) * (1.0 / 18446744073709551615.0);
	return (normalized * (maximum - minimum)) + minimum;
}

bool random_generate_probability(RandomGenerator &generator, int32_t perCentProbability) {
	return int32_t(random_generate_U64(generator) % 100u) < perCentProbability;
}
