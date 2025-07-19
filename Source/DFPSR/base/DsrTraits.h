// zlib open source license
//
// Copyright (c) 2025 David Forsgren Piuva
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

// These custom traits allow implementing template functions that can work with SIMD types when needed, without exposing simd.h in headers.

#ifndef DFPSR_TRAITS
#define DFPSR_TRAITS

	#include <stdint.h>
	#include <type_traits>

	namespace dsr {
		// Subset of std::integral_constant.
		template <typename T, T VALUE>
		struct DSR_PROPERTY {
			static constexpr T value = VALUE;
		};
		// Custom implementation of std::false_type.
		using DSR_TRAIT_FALSE = dsr::DSR_PROPERTY<bool, false>;
		// Custom implementation of std::true_type.
		using DSR_TRAIT_TRUE = dsr::DSR_PROPERTY<bool, true>;
		// Custom implementation of std::is_same.
		template <typename T, typename U>
		struct DsrTrait_SameType { static const bool value = false; };
		template <typename T>
		struct DsrTrait_SameType<T, T> { static const bool value = true; };
		// Custom implementation of std::enable_if.
		template<bool B, class T = void>
		struct DsrTrait_EnableIf;
		template<class T>
		struct DsrTrait_EnableIf<true, T> {
			using type = T;
		};

		// Place this as a template argument to disable the template function when false.
		#define DSR_ENABLE_IF(TRAIT) \
			typename = typename dsr::DsrTrait_EnableIf<TRAIT>::type 

		// Properties are given to single types.
		#define DSR_DECLARE_PROPERTY(PROPERTY_NAME) \
			template <typename T> struct PROPERTY_NAME : dsr::DSR_TRAIT_FALSE {};

		#define DSR_APPLY_PROPERTY(PROPERTY_NAME, TYPE_NAME) \
			template <> struct PROPERTY_NAME<TYPE_NAME> : dsr::DSR_TRAIT_TRUE  {};

		#define DSR_CHECK_PROPERTY(PROPERTY_NAME, TYPE_NAME) \
			(PROPERTY_NAME<TYPE_NAME>::value)

		// Relations are given to pairs of types.
		#define DSR_DECLARE_RELATION(RELATION_NAME) \
			template <typename T, typename U> struct RELATION_NAME : dsr::DSR_TRAIT_FALSE {};

		#define DSR_APPLY_RELATION(RELATION_NAME, TYPE_A, TYPE_B) \
			template <> struct RELATION_NAME<TYPE_A, TYPE_B> : dsr::DSR_TRAIT_TRUE  {};

		#define DSR_CHECK_RELATION(RELATION_NAME, TYPE_A, TYPE_B) \
			(RELATION_NAME<TYPE_A, TYPE_B>::value)

		// Checking types.
		#define DSR_SAME_TYPE(TYPE_A, TYPE_B) dsr::DsrTrait_SameType<TYPE_A, TYPE_B>::value
		#define DSR_CONVERTIBLE_TO(FROM, TO) std::is_convertible<FROM, TO>::value
		#define DSR_UTF32_LITERAL(TYPE) std::is_convertible<TYPE, const char32_t*>::value
		#define DSR_ASCII_LITERAL(TYPE) std::is_convertible<TYPE, const char*>::value
		#define DSR_INHERITS_FROM(DERIVED, BASE) std::is_base_of<BASE, DERIVED>::value

		// Supress type safety when impossible conversions can never execute.
		template<typename TO, typename FROM>
		inline const TO& unsafeCast(const FROM &value) {
			const void *pointer = (const void*)&value;
			return *(const TO*)pointer;
		}

		DSR_DECLARE_PROPERTY(DsrTrait_Any_U8)
		DSR_APPLY_PROPERTY(DsrTrait_Any_U8, uint8_t)

		DSR_DECLARE_PROPERTY(DsrTrait_Any_U16)
		DSR_APPLY_PROPERTY(DsrTrait_Any_U16, uint16_t)

		DSR_DECLARE_PROPERTY(DsrTrait_Any_U32)
		DSR_APPLY_PROPERTY(DsrTrait_Any_U32, uint32_t)

		DSR_DECLARE_PROPERTY(DsrTrait_Any_I32)
		DSR_APPLY_PROPERTY(DsrTrait_Any_I32, int32_t)

		DSR_DECLARE_PROPERTY(DsrTrait_Any_F32)
		DSR_APPLY_PROPERTY(DsrTrait_Any_F32, float)

		DSR_DECLARE_PROPERTY(DsrTrait_Any)
		DSR_APPLY_PROPERTY(DsrTrait_Any,   int8_t)
		DSR_APPLY_PROPERTY(DsrTrait_Any,  int16_t)
		DSR_APPLY_PROPERTY(DsrTrait_Any,  int32_t)
		DSR_APPLY_PROPERTY(DsrTrait_Any,  int64_t)
		DSR_APPLY_PROPERTY(DsrTrait_Any,  uint8_t)
		DSR_APPLY_PROPERTY(DsrTrait_Any, uint16_t)
		DSR_APPLY_PROPERTY(DsrTrait_Any, uint32_t)
		DSR_APPLY_PROPERTY(DsrTrait_Any, uint64_t)
		DSR_APPLY_PROPERTY(DsrTrait_Any,    float)
		DSR_APPLY_PROPERTY(DsrTrait_Any,   double)

		DSR_DECLARE_PROPERTY(DsrTrait_Scalar_SignedInteger)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_SignedInteger,  int8_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_SignedInteger, int16_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_SignedInteger, int32_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_SignedInteger, int64_t)

		DSR_DECLARE_PROPERTY(DsrTrait_Scalar_UnsignedInteger)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_UnsignedInteger,  uint8_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_UnsignedInteger, uint16_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_UnsignedInteger, uint32_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_UnsignedInteger, uint64_t)

		DSR_DECLARE_PROPERTY(DsrTrait_Scalar_Floating)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_Floating,  float)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_Floating, double)

		DSR_DECLARE_PROPERTY(DsrTrait_Scalar_Integer)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_Integer,   int8_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_Integer,  int16_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_Integer,  int32_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_Integer,  int64_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_Integer,  uint8_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_Integer, uint16_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_Integer, uint32_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar_Integer, uint64_t)

		DSR_DECLARE_PROPERTY(DsrTrait_Scalar)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar,   int8_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar,  int16_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar,  int32_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar,  int64_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar,  uint8_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar, uint16_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar, uint32_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar, uint64_t)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar,    float)
		DSR_APPLY_PROPERTY(DsrTrait_Scalar,   double)
	}
#endif
