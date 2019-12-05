# Native

The native window implementation for a certain system should begin with the name of the library being linked, and continue with a postfix "Window.cpp".
For example: Linking to "X11" using "-lX11" will look for "X11Window.cpp" for the code that implements the following constructor.
std::shared_ptr<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height);

If your application won't create any window, compile and link with NoWindow.cpp to get rid of all dependencies.
