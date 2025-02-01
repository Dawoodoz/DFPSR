# Source folder
The source folder contains the framework, system dependent backends, code examples and tools.

## System dependent code
Some of the code depends on different hardware and operating systems to abstract them away.

* Source/DFPSR/api/fileAPI: The file API is implemented to hande file access on different operating systems.

* Source/DFPSR/base/simd.h: The simd.h header is a SIMD abstraction layer that works without SIMD but can become faster for specific processor architectures by implementing the features.

* Source/DFPSR/base/heap.cpp: The getCacheLineSize function depends on the operating system to get the cache line width for memory alignment, because aligning with cache lines is needed for thread safety and getting cache line width directly from hardware would require contemporary inline assembler hacks that will not work for future generations of hardware.

* Source/windowManagers: Contains the integrations for displaying graphics and getting mouse and keyboard input on specific operating systems.
These are selected based on the operating system in Source/DFPSR/DFPSR.DsrHead

* Source/soundManagers: Contains the integrations for sound on specific operating systems.
These are selected based on the operating system in Source/DFPSR/DFPSR.DsrHead
