# DFPSR
A modern software rendering library for C++14 using SSE/NEON created by David Forsgren Piuva. If you're looking for the latest mainstream fad, look elsewhere. This is a library for quality software meant to be developed over multiple decades and survive your grandchildren with minimal maintenance. Just like carving your legacy into stone, it takes more effort to master the skill but gives a more robust result by not relying on a far away library. Maximum user experience and minimum system dependency.

The official website:
[dawoodoz.com](https://dawoodoz.com)

## What your games might look like using isometric CPU rendering

![Screenshot of the sandbox example running on a hexacore Intel Core I5 9600K.](Sandbox.jpg "Sandbox example")

Real-time dynamic light with depth-based casted shadows and normal mapping at 453 frames per second in 800x600 pixels running on the CPU. Higher resolutions would break the retro style and actually look worse, but there's lots of time left for game logic and additional effects. By pre-rendering 3D models to diffuse, normal and height images, reading the data is much more cache efficient on modern CPUs than using a free perspective. This also allow having more triangles than pixels on the screen and doing passive updates of static geometry. Low-detailed 3D models are used to cast dynamic shadows.

## Why use a software renderer when GPUs are so fast?
* **Minimal dependency** for minimal support cost. No customer will ever tell you that some shader wouldn't compile on a GPU driver you never even heard of. It's all pure math on the CPU sending calls directly to the system.
* **Robust** because it will probably not ruin your system when making a mistake, unlike graphics APIs for the GPU that are prone to blue-screens. There are layers of safety for most API calls and pointer classes have extra tight memory protection in debug mode while leaving no trace in release mode.
* **Determinism down to machine precision** means that if it worked on one computer, it will probably work the same on another computer.
* **Often faster than the monitor's refresh rate** for isometric graphics with dynamic light. Try the Sandbox SDK example compiled in release mode on Ubuntu or Manjaro to check if it's smooth on your CPU. Quad-core Intel Core I5 should be fast enough in resonable resolutions, hexa-core I5 will have plenty of performance and octa-core I7 is butter smooth even in high resolutions.
* **Low latency for retro 2D graphics** using the CPU's higher frequency for low resolutions. There are no hardware limits other than CPU cycles and memory. Render to textures, apply deferred light filters or write your modified rendering pipeline for custom geometry formats.
* **Create your legacy.** Make software that future generations might be able to port, compile and run natively without the need for emulators. Each new operating system is supported by implementing any deviations from the Posix standard filesystem into fileAPI.cpp and implementing a wrapper module for window management in Source/windowManagers. This standardization of minimal system dependency makes it easy to repair things and port to new systems on your own, rather than having everyone porting their own subset of a feature bloated media layer when popularity dies out.

## More than a graphics API, less than a graphics engine
It is a rendering API, image processing framework and graphical user interface system in a static C++14 library meant to minimize the use of dynamic dependencies in long-term projects while still offering the power to make your own abstractions on top of low-level rendering operations. The core library itself is pure math on a hardware abstraction and can be compiled on most systems using GNU's C++14.

## Still a public beta
Don't use it for safety-critical projects unless you verify correctness yourself and take all responsibility. Either way, it's probably a lot safer than using OpenGL, OpenCL or Direct3D simply by being a single implementation where bugs will be mostly the same on each platform. Stack memory for VLA may vary. Test everything with billions of cases.

## Supported CPU hardware:
* **Intel/AMD** using **SSE2** intrinsics and optional extensions.
* **ARM** using **NEON** intrinsics.
* Unknown CPU architectures, without SIMD vectorization as a fallback solution.

## Platforms:
* **Linux**, tested on Mint, Mate, Manjaro, Ubuntu, RaspberryPi OS, Raspbian Buster or later.
Linux Mint needs the compiler and X11 headers, so run "sudo apt install g++" and "sudo apt install libx11-dev" before compiling.
Currently supporting X11 and Wayland is planned for future versions.
* **Microsoft Windows**, but slower than on Linux because the multi-threaded canvas upload is not yet implemented and both threading and memory allocation is slower on Windows.

## Might also work on:
* BSD and Solaris has code targeting the platforms in fileAPI.cpp for getting the application folder, but there are likely some applications missing for running the build script.
Future Posix compliant systems should only have a few quirks to sort out if it has an X11 server.
* Big-Endian is supported in theory if enabling the DSR_BIG_ENDIAN macro globally, but this has never actually been tested due to difficulties with targeting such an old system with modern compilers.

## Not yet ported to:
* Macintosh no longer uses X11, so it will require some porting effort.
Macintosh does not have a symbolic link to the binary of the running process, so it would fall back on the current directory when asking for the application folder.

## Will not target:
* Mobile phones. Because the constant changes breaking backwards compatibility on mobile platforms would defeat the purpose of using a long-lifetime framework, you will be on your own if you try to use the library for such use cases. You cannot just take something from a desktop and run it on a phone, because mobile platforms require custom C++ compilers, screen rotation, battery saving, knowing when to display the virtual keyboard, security permissions, forced full-screen... Trying to do both at the same time would end up with design compromises in both ends like Microsoft Windows 8 or Ubuntu's Unity lock screen.
* Web frontends. Such a wrapper over this library would not be able to get the power of SIMD intrinsics for defining your own image filters, so you would be better off targeting a GPU shading language from the browser which is more suited for dynamic scripting.
