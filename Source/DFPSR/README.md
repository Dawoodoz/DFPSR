# David Piuva's software renderer

Since august 12 2017

## Why use this software renderer

* No core dependencies, just a static library defining all rendering mathematically in C++. You can render without any GPU and save directly to files without depending on a window manager. You can show grayscale images as ascii art in the terminal for debugging. If you want a window manager, you can inject one from the outside by creating a backend class and inject into a portable DsrWindow.

* No device lost exceptions randomly erasing all your buffers. This may happen on GPUs because of poor hardware design that never considered general purpose computations.

* No shader compilation failure at end users. It's all compiled with the application.

* No visible shader source code exposing secret algoritms to competitors. It's just more code in your executable where identifiers can be obfuscated.

* No missing GPU extensions. You don't even need a GPU if you save the result to a file or send it over a network.

### Classic games

Making a classic game without defining how rendering is done in pure high-level math makes as little sense as saving the last copy of your family history book for generations to come in a platform specific encrypted DRM format. If someone's going to restore your game into playable conditions 500 years from now by porting the code, they shouldn't have to guess what OpenGL is nor which month's driver patch it requires to run.

### Long lifetime projects

If you plan to make something that takes 30 years to develop and should be operational far into the future, you need a statically linked framework that defines everything clearly and precisely in one place without leving room for alternative interpretation by external libraries.

### Generate textures for the GPU

To save space and get more randomness to your textures. You can use this framework to generate images with higher determinism and less maintenance cost.

### Determinism

By having a single implementation on top of a SIMD hardware abstraction layer, operations are following the same algorithms on every platform with less surprises. If you can avoid using floating-point operations, you'll get 100% determinism.

### Always direct memory access

While not advisable to break multi-threading convention, you can always access your data directly without having to worry about the long delay from the GPU back to the CPU.

### Fast and precise 2D graphics

To use OpenGL for 2D graphics without GPU acceleration on Linux is technically to let the CPU emulate a GPU, which then does what the CPU does best to begin with. Using the CPU directly for 2D is both faster and more precise.

* No need to fake GPU memory uploads nor downloads. You have direct access to the data and can get pointers to your image buffers.

* Pixel-exact 2D image drawing using direct integer coordinates. No trial and error with different graphics drivers, it just works correctly with speeds comparable to the GPU.

* If you're making a low-resolution 2D retro game with many small sprites, the CPU will probably reach far higher framerates than your screen is capable of displaying, which gives room for more game logic and visual effects.

* Filters on aligned images can read, write and process using SIMD vectors, which is very fast even in high resolutions. Even better if you combine multiple filters into compound functions that read and write once.

### Possible to modify down to the core

If you miss something in the rendering pipeline or just want to learn all the math behind fundamental 3D graphics, you can modify the source code directly and have it statically linked to your application. Maybe you want a custom 12 channel image format for generating 2D sprites with depth and normal maps. Maybe you want to try a new triangle occlusion system or threading algorithm for improved rendering speed.

There's no need to look at cryptic assembly code if something won't compile, just high-level math operations taking multiple pixels at once. The whole renderer is built on top of a simplistic SIMD vector abstraction layer in the simd.h module, which is well tested on both Intel and ARM processors.

### Write your SIMD filter once, run on Intel/ARM

The SIMD vectors work on SSE, NEON and scalar operations, so you aren't forced to write another version in case that SIMD doesn't exist. It is however good practice to validate your ideas and create regression tests with a safe algorithm first.

Even when running without a supported SIMD instruction set, the emulated scalar version can be better at utilizing hyper-threading than a naive non-vectorized pixel loop, because the CPU's instruction window can process multiple pixels at once on different ALUs within the same block.

## Requirements

* Just like when building your own desktop computer, ARM based mini-computers also need cooling if you plan to do something resource intensive non-stop for hours.

* Big-endian mode is untested and gives a warning if compiling using the DSR_BIG_ENDIAN macro. The file endian.h only exists in case that big-endian ever comes back to personal computers in the future.

* Requires the VLA compiler extension (Variable length array) from C. It does make the machine code look like crap, but it's still a lot better than fragmenting the heap every time you draw a triangle.

