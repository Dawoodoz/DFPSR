
#ifndef DFPSR_DIRTY_RECTANGLES
#define DFPSR_DIRTY_RECTANGLES

#include "../../../DFPSR/includeFramework.h"

namespace dsr {

class DirtyRectangles {
private:
	int32_t width = 0;
	int32_t height = 0;
	List<IRect> dirtyRectangles;
public:
	DirtyRectangles() {}
	// Call before rendering to let the dirty rectangles know if the resolution changed
	void setTargetResolution(int32_t width, int32_t height) {
		if (this->width != width || this->height != height) {
			this->width = width;
			this->height = height;
			this->allDirty();
		}
	}
public:
	IRect getTargetBound() const {
		return IRect(0, 0, this->width, this->height);
	}
	// Call when everything needs an update
	void allDirty() {
		this->dirtyRectangles.clear();
		this->dirtyRectangles.push(getTargetBound());
	}
	void noneDirty() {
		this->dirtyRectangles.clear();
	}
	void makeRegionDirty(IRect newRegion) {
		newRegion = IRect::cut(newRegion, getTargetBound());
		if (newRegion.hasArea()) {
			for (int i = 0; i < this->dirtyRectangles.length(); i++) {
				if (IRect::touches(this->dirtyRectangles[i], newRegion)) {
					// Merge with any existing bound
					newRegion = IRect::merge(newRegion, this->dirtyRectangles[i]);
					this->dirtyRectangles.remove(i);
					// Restart the search for overlaps with a larger region
					i = -1;
				}
			}
			this->dirtyRectangles.push(newRegion);
		}
	}
	int64_t getRectangleCount() const {
		return dirtyRectangles.length();
	}
	IRect getRectangle(int64_t index) const {
		return this->dirtyRectangles[index];
	}
};

}

#endif
