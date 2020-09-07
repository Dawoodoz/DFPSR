# Sprite engine for the DFPSR SDK

Created for the SDK and used as an example for how to make your own isometric rendering engine on top of DFPSR.
Instead of feeling limited by what it can't do, try modifying the code and optimizing your own filters using the SIMD and threading abstractions.
This engine is currently re-using some DFPSR internals of the "image" and "render" folders, but these are not version stable APIs and might break backward compatibility.
