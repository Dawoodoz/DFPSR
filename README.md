# DFPSR
A modern software rendering library for C++14 using SSE/NEON created by David Forsgren Piuva. If you're looking for the latest mainstream fad, look elsewhere. This is a library for quality software meant to be developed over multiple decades and survive your grandchildren with minimal maintenance.

## What your games might look like using isometric CPU rendering

![Screenshot of the sandbox example running on a hexacore Intel Core I5 9600K.](Sandbox.png "Sandbox example")

Real-time dynamic light with depth-based casted shadows and normal mapping at 295 frames per second in 800x600 pixels running on the CPU. Higher resolutions would break the retro style and actually look worse, but there's lots of time left for game logic and additional effects. By pre-rendering 3D models to diffuse, normal and height images, reading the data is much more cache efficient on modern CPUs than using a free perspective. This also allow having more triangles than pixels on the screen and doing passive updates of static geometry. Low-detailed 3D models are used to cast dynamic shadows.

## Why use a software renderer when GPUs are so fast?
* **Minimal dependency** for minimal support cost. No customer will ever tell you that some shader wouldn't compile on a GPU driver you never even heard of. It's all pure math on the CPU sending calls directly to the system.
* **Robust** because it will probably not ruin your system when making a mistake, unlike graphics APIs for the GPU that are prone to blue-screens. There are layers of safety for most API calls and pointer classes have extra tight memory protection in debug mode while leaving no trace in release mode.
* **Determinism down to machine precision** means that if it worked on one computer, it will probably work the same on another computer.
* **Often faster than the monitor's refresh rate** for isometric graphics with dynamic light. Try the Sandbox SDK example compiled in release mode on Ubuntu or Manjaro to check if it's smooth on your CPU. Quad-core Intel Core I5 should be fast enough in resonable resolutions, hexa-core I5 will have plenty of performance and octa-core I7 is butter smooth even in high resolutions.
* **Low latency for retro 2D graphics** using the CPU's higher frequency for low resolutions. There are no hardware limits other than CPU cycles and memory. Render to textures, apply deferred light filters or write your modified rendering pipeline for custom geometry formats.
* **Easy to use GUI system.** Create a window, load the interface from a file and connect a lambda function to a button's click action using only a few lines of procedural code.
* **Create your legacy.** Make software that future generations might be able to port, compile and run natively without the need for emulators.

## Still a public beta
Don't use it for safety-critical projects unless you verify correctness yourself and take all responsibility. Either way, it's probably a lot safer than using OpenGL, OpenCL or Direct3D simply by being a single implementation where bugs will be mostly the same on each platform. Stack memory for VLA may vary. Test everything with billions of cases.

## Platforms
* Developed mainly with Ubuntu on desktops and laptops.
* Tested with Manjaro, my new favorite Linux distribution. Installing GPU drivers and running the latest AAA games was much easier than on other distributions. I would still use CPU rendering for most of my development where GPU rendering would simply be total overkill and only increase future maintenance costs.
* Tested with Ubuntu mate on Raspberry Pi 3B and Pine64. (Ubuntu Mate didn't work on Raspberry Pi Zero)
* Tested with Raspbian Buster on Raspberry Pi Zero W (X11 doesn't work on older versions of Raspbian)
* Linux Mint need the compiler and X11 headers, so run "sudo apt install g++" and "sudo apt install libx11-dev" before compiling.
* There's a half finished Win32 port that's not published because it wasn't fast enough when emulated on 64-bit Windows. Might have to write for 64-bit only on Windows to prevent poor performance.

## Remaining work
* Optimization of 3D rendering is still quite primitive. There's no quad-tree algorithm for quickly skipping large chunks of unseen pixels. There's no depth sorting nor early removal of triangles yet.
* The 3D camera system doesn't have a fixed API yet. The ability to not have a far-clip-plane made it complicated to express using a standard view space, so clip planes are stored separatelly while rendering. There might be a better solution.

## How you can help
* Report bugs that you find. (Visual C++ is not supported because it doesn't have C++14 nor C11 extensions)
* Port the Window wrapper to more platforms with instructions for compiling and linking.
* Create your own GUI components that can be pasted into a project and registered to the class factory. The most used may be taken into the core.
* Develop minimal-dependency open-source games as a complement to the SDK.
* Create 3D model import/export functions for standard formats.
* Create 3D model editor with custom format for generating pre-rendered extremely detailed isometric sprites with diffuse, normal and height images, compatible with the Sandbox example.

## Supported CPU hardware
* Intel/AMD using SSE2 intrinsics.
* ARM using NEON intrinsics.
* Unknown Little-Endian systems. (without SIMD vectorization)

## Might support if it comes back:
* Big endian. Currently mostly used in routers, which aren't nearly powerful enough.

## Will never support: (Don't worry power-users, this is a nerdy library meant for Linux geeks, retro gamers and scientists.)
* Mobile phones. You can't take a phone application and just release it for a desktop system, because this is why Windows 8 failed. Use the right tool for each task and consider the differences in usability contexts.
* Web frontends. The whole point of using this software renderer is the ability to write your own image filters using portable SIMD vectors in C++. A web script that can run native C++ applications is a web script that I would never allow into my browser due to the total lack of security. At least let me scan for viruses and choose whether to trust the source first.
