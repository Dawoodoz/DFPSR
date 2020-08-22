To get started with your own project:
* Copy one of these folders into a location with stable relative access to the DFPSR library.
  You can also place DFPSR in a standard path and refer to it using absolute paths.
* Change #include "../../DFPSR/includeFramework.h" in main.cpp to find the header from the new location.
* Change ROOT_PATH=../.. the same way in build.sh or just delete the whole build script if you're not going to use it.
* Give execution permission using chmod +x build.sh. (if using Linux)
* Build and run your new application. (./build.sh if using Linux)

