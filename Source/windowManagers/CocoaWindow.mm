
#import <Cocoa/Cocoa.h>

#include "../DFPSR/base/Handle.h"
#include "../DFPSR/implementation/gui/BackendWindow.h"
#include "../DFPSR/api/stringAPI.h"

dsr::Handle<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height) {
	dsr::sendWarning("Tried to create a DsrWindow with Cocoa!\n");
	return dsr::Handle<dsr::BackendWindow>();
}
