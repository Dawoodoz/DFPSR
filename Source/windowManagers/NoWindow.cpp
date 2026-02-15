
#include "../DFPSR/base/Handle.h"
#include "../DFPSR/implementation/gui/BackendWindow.h"
#include "../DFPSR/api/stringAPI.h"

dsr::Handle<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int32_t width, int32_t height) {
	dsr::sendWarning(U"Tried to create a DsrWindow without a window manager selected!\n");
	return dsr::Handle<dsr::BackendWindow>();
}
