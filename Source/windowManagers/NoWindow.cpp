
#include "../DFPSR/gui/BackendWindow.h"
#include "../DFPSR/base/text.h"

std::shared_ptr<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height) {
	dsr::throwError("Tried to create a DsrWindow without a window manager selected!\n");
	return std::shared_ptr<dsr::BackendWindow>();
}

