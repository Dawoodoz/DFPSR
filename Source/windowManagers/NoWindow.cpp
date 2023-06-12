
#include "../DFPSR/gui/BackendWindow.h"
#include "../DFPSR/api/stringAPI.h"

std::shared_ptr<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height) {
	dsr::sendWarning("Tried to create a DsrWindow without a window manager selected!\n");
	return std::shared_ptr<dsr::BackendWindow>();
}

