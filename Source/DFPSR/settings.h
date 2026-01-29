
// This header collects hardcoded settings for the entire framework in one place.
//   Either modify this header for all your projects, or define macros using compiler flags for a specific project.

#ifndef DFPSR_SETTINGS
#define DFPSR_SETTINGS
	// If you are not using try-catch, you can let the default error handler call heap_hardExitCleaning and std::exit instead of throwing std::exception.
	//   This may reduce some runtime overhead from stack unwinding.
	#ifndef __EXCEPTIONS
		// If compiling with -fno-exceptions, hard exit must be enabled.
		#define DSR_HARD_EXIT_ON_ERROR
	#endif

	// Enable to ban any implicit string conversion from Ascii to Unicode, so that accidentally writing "" instead of U"" does not create temporary heap allocations.
	//#define BAN_IMPLICIT_ASCII_CONVERSION

	// If EXTRA_SAFE_POINTER_CHECKS is defined, debug mode will let SafePointer perform thread and allocation identity checks.
	//     Makes sure that the accessed memory has not been freed, recycled or shared with the wrong thread.
	//     This will make memory access super slow but catch more memory errors when basic bound checks are not enough.
	// If EXTRA_SAFE_POINTER_CHECKS is not defined, debug mode will 
	// Has no effect in release mode, because it is only active when SAFE_POINTER_CHECKS is also defined.
	//#define EXTRA_SAFE_POINTER_CHECKS

	// Enable this macro to disable multi-threading.
	//   Can be used to quickly rule out concurrency problems when debugging, by recreating the same error without extra threads.
	//#define DISABLE_MULTI_THREADING

	// Identify operating systems in a somewhat future-proof way.
	//   More will have to be added later.
	#if defined(_WIN32) || defined(_WIN64) || defined(_WIN128) || defined(_WIN256) || defined(_WIN512) || defined(_WIN1024) || defined(_WIN2048)
		#define USE_MICROSOFT_WINDOWS
	#elif defined(__gnu_linux__) || defined(__linux__) || defined(__linux)
		#define USE_LINUX
		#define USE_POSIX
	#elif defined(__APPLE__) || defined(__MACH__)
		#define USE_MACOS
		#define USE_POSIX
	#elif defined(__unix) || defined(__unix__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__BSD__)
		// Trying to cover as many systems as possible using Posix for basic command line applications.
		#define USE_POSIX
	#else
		#error "Could not identify the operating system!\n"
	#endif

	// Identify processor types.
	#if defined(__INTEL__) || defined(__SSE2__)
		#define USE_INTEL
	#elif defined(__ARM__) || defined(__ARM_NEON)
		#define USE_ARM
	#endif

	// Determine which SIMD extensions to use in base/simd.h.
	// Use the standard compiler flags for enabling SIMD extensions.
	//   If your compiler uses a different macro name to indicate the presence of a SIMD extension, you can add them here to enable the USE_* macros.
	// You can compile different versions of the program for different capabilities.
	//   SSE2 and NEON are usually enabled by default on instruction sets where they are not optional, which is good if you just want one release that is fast enough.
	//   AVX with 256-bit float SIMD is useful for sound and physics that can be computed without integers.
	//   AVX2 with full 256-bit SIMD is useful for 3D graphics and heavy 2D filters where memory bandwidth is not the bottleneck.
	#if defined(__SSE2__)
		#define USE_SSE2 // Comment out this line to test without SSE2
		#ifdef USE_SSE2
			#ifdef __SSSE3__
				#define USE_SSSE3 // Comment out this line to test without SSSE3
			#endif
			#ifdef __AVX__
				#define USE_AVX // Comment out this line to test without AVX
				#ifdef USE_AVX
					#ifdef __AVX2__
						#define USE_AVX2 // Comment out this line to test without AVX2
					#endif
				#endif
			#endif
		#endif
	#elif defined(__ARM_NEON)
		#define USE_NEON // Comment out this line to test without NEON
		// TODO: Check if SVE is enabled once implemented in simd.h.
	#endif

	// Enable the EMULATE_X_256BIT_SIMD macro to force use of 256-bit vectors even when there is no hardware instructions supporting it.
	//   To get real 256-bit SIMD on an Intel processor with AVX2 hardware instructions, enable the AVX2 compiler flag for the library and your project (which is -mavx2 in GCC).
	//#define EMULATE_256BIT_X_SIMD

	// Enable the EMULATE_F_256BIT_SIMD macro to force use of 256-bit float vectors even when there is no hardware instructions supporting it.
	//   To get real 256-bit float SIMD on an Intel processor with AVX hardware instructions, enable the AVX compiler flag for the library and your project (which is -mavx in GCC).
	//#define EMULATE_256BIT_F_SIMD

	// A platform independent summary of which features are enabled.
	#ifdef USE_SSE2
		// We have hardware support for 128-bit SIMD, which is enough to make memory bandwidth the bottleneck for light computation.
		#define USE_BASIC_SIMD
		#ifdef USE_AVX
			// We have hardware support for 256-bit float SIMD, so that we get a performance boost from using F32x8 or its alias F32xF instead of F32x4
			#define USE_256BIT_F_SIMD
			#ifdef USE_AVX2
				// We also have hardware support for the other 256-bit SIMD types, pushing the size of an X vector and default alignment to 256 bits.
				//   F32xX will now refer to F32x8
				//   I32xX will now refer to I32x8
				//   U32xX will now refer to U32x8
				//   U16xX will now refer to U16x16
				//   U8xX will now refer to U8x32
				#define USE_256BIT_X_SIMD
			#endif
		#endif
	#endif
	#ifdef USE_NEON
		#define USE_BASIC_SIMD
	#endif

	// Enable this flag if you are compiling for big-endian.
	//#define DSR_BIG_ENDIAN
#endif
