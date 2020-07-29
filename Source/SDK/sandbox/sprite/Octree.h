
#ifndef DFPSR_OCTREE
#define DFPSR_OCTREE

#include "../../../DFPSR/includeFramework.h"
#include <memory>

namespace dsr {

// TODO: A method for receiving 3D line drawing callbacks for debug drawing the entire octree in 3D.

// bool(const IVector3D& minBound, const IVector3D& maxBound)
using OcTreeFilter = std::function<bool(const IVector3D&, const IVector3D&)>;

// void(T& content, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound)
template <typename T>
using OcTreeLeafOperation = std::function<void(T&, const IVector3D, const IVector3D, const IVector3D)>;

template <typename T>
class OctreeLeaf {
public:
	T content;
	IVector3D origin, minBound, maxBound;
public:
	OctreeLeaf(const T& content, const IVector3D origin, const IVector3D minBound, const IVector3D maxBound)
	: content(content), origin(origin), minBound(minBound), maxBound(maxBound) {}
public:
	void find(const OcTreeFilter& boundFilter, const OcTreeLeafOperation<T>& leafOperation) {
		if (boundFilter(this->minBound, this->maxBound)) {
			leafOperation(this->content, this->origin, this->minBound, this->maxBound);
		}
	}
};

static const uint32_t Octree_MaskX = 1u;
static const uint32_t Octree_MaskY = 2u;
static const uint32_t Octree_MaskZ = 4u;

inline int Octree_getBranchIndex(bool pX, bool pY, bool pZ) {
	return (int)((pX ? Octree_MaskX : 0u) | (pY ? Octree_MaskY : 0u) | (pZ ? Octree_MaskZ : 0u));
}

struct IBox3D {
	IVector3D min, max;
	IBox3D(const IVector3D& min, const IVector3D& max) : min(min), max(max) {}
};

inline IBox3D splitBound(const IBox3D& parent, int branchIndex) {
	assert(branchIndex >= 0 && branchIndex < 8);
	IVector3D size = (parent.max - parent.min) / 2;
	assert(size.x > 0 && size.y > 0 && size.z > 0);
	IVector3D minBound = parent.min;
	if ((uint32_t)branchIndex & Octree_MaskX) { minBound.x += size.x; }
	if ((uint32_t)branchIndex & Octree_MaskY) { minBound.y += size.y; }
	if ((uint32_t)branchIndex & Octree_MaskZ) { minBound.z += size.z; }
	IVector3D maxBound = minBound + size;
	return IBox3D(minBound, maxBound);
}

template <typename T>
class OctreeNode {
public:
	// The ownership telling if a leaf of origin belongs here
	IVector3D minOwnedBound, maxOwnedBound;
	// The combined bounding box of all children, which may exceed the owned bound by the largest leaf radius measured from the origin
	IVector3D minLeafBound, maxLeafBound;
	// When divided, any added leaves will try to be inserted into child nodes when possible
	//   Leaves that are too large may stay at the parent node
	bool divided = false;
	// One optional child node for each of the 8 sections in the octree
	std::shared_ptr<OctreeNode<T>> childNodes[8]; // TODO: Use for branching when getting too crowded with direct leaves
	// The leaves that have not yet been assigned to a specific child node
	List<OctreeLeaf<T>> leaves;
public:
	// Copy constructor
	OctreeNode(const OctreeNode<T>& original)
	: minOwnedBound(original.minOwnedBound),
	  maxOwnedBound(original.maxOwnedBound),
	  minLeafBound(original.minLeafBound),
	  maxLeafBound(original.maxLeafBound),
	  divided(original.divided),
	  childNodes(original.childNodes),
	  leaves(original.leaves) {
		for (int n = 0; n < 8; n++) {
			this->childNodes[n] = original.childNodes[n];
		}
	}
	// Create from first leaf
	OctreeNode(const OctreeLeaf<T>& firstLeaf, const IVector3D& minOwnedBound, const IVector3D& maxOwnedBound)
	: minOwnedBound(minOwnedBound), maxOwnedBound(maxOwnedBound), minLeafBound(firstLeaf.minBound), maxLeafBound(firstLeaf.maxBound), divided(false) {
		this->leaves.push(firstLeaf);
	}
	// Create from first child node
	OctreeNode(const OctreeNode<T>& firstBranch, int firstBranchIndex, const IVector3D& minOwnedBound, const IVector3D& maxOwnedBound)
	: minOwnedBound(minOwnedBound), maxOwnedBound(maxOwnedBound), minLeafBound(firstBranch.minLeafBound), maxLeafBound(firstBranch.maxLeafBound), divided(true) {
		this->childNodes[firstBranchIndex] = std::make_shared<OctreeNode<T>>(firstBranch);
	}
public:
	bool insideLeafBound(const IVector3D& origin) {
		return origin.x >= this->minLeafBound.x && origin.y >= this->minLeafBound.y && origin.z >= this->minLeafBound.z
		    && origin.x <= this->maxLeafBound.x && origin.y <= this->maxLeafBound.y && origin.z <= this->maxLeafBound.z;
	}
	bool insideOwnedBound(const IVector3D& origin) {
		return origin.x >= this->minOwnedBound.x && origin.y >= this->minOwnedBound.y && origin.z >= this->minOwnedBound.z
		    && origin.x <= this->maxOwnedBound.x && origin.y <= this->maxOwnedBound.y && origin.z <= this->maxOwnedBound.z;
	}
	// Get the branch index closest to the origin
	int getInnerBranchIndex() {
		return Octree_getBranchIndex(
		  this->minOwnedBound.x + this->maxOwnedBound.x < 0,
		  this->minOwnedBound.y + this->maxOwnedBound.y < 0,
		  this->minOwnedBound.z + this->maxOwnedBound.z < 0
		);
	}
	// Returns true iff the given leaf is allowed to create a new branch
	bool mayBranch(const OctreeLeaf<T>& leaf) {
		IVector3D leafDimensions = leaf.maxBound - leaf.minBound;
		IVector3D maxDimensions = (this->maxOwnedBound - this->minOwnedBound) / 4;
		return this->divided == true
		    && leafDimensions.x <= maxDimensions.x
		    && leafDimensions.y <= maxDimensions.y
		    && leafDimensions.z <= maxDimensions.z;
	}
	void insert(const OctreeLeaf<T>& leaf) {
		// Update leaf bounds along each passed node along the way
		if (leaf.minBound.x < this->minLeafBound.x) { this->minLeafBound.x = leaf.minBound.x; }
		if (leaf.minBound.y < this->minLeafBound.y) { this->minLeafBound.y = leaf.minBound.y; }
		if (leaf.minBound.z < this->minLeafBound.z) { this->minLeafBound.z = leaf.minBound.z; }
		if (leaf.maxBound.x > this->maxLeafBound.x) { this->maxLeafBound.x = leaf.maxBound.x; }
		if (leaf.maxBound.y > this->maxLeafBound.y) { this->maxLeafBound.y = leaf.maxBound.y; }
		if (leaf.maxBound.z > this->maxLeafBound.z) { this->maxLeafBound.z = leaf.maxBound.z; }
		// Make sure that the origin is inside of the owned bound by creating new parents until the point is covered
		while (!this->insideOwnedBound(leaf.origin)) {
			if (this->minOwnedBound.x < -100000000 || this->maxOwnedBound.x > 100000000) {
				throwError("Cannot expand (", this->minOwnedBound, ")..(", this->maxOwnedBound, ") to include ", leaf.origin, "! The origin must be given to the correct side of the octree's root.\n");
			}
			// Create a new parent node containing the old one
			OctreeNode newParent = OctreeNode(*this, this->getInnerBranchIndex(), this->minOwnedBound * 2, this->maxOwnedBound * 2);
			// Replace the content of this node with the new parent so that pointers to the old node leads to it
			*this = newParent;
		}
		// Try inserting into any child node
		for (int n = 0; n < 8; n++) {
			if (this->childNodes[n].get() != nullptr && this->childNodes[n]->insideOwnedBound(leaf.origin)) {
				this->childNodes[n]->insert(leaf);
				return; // Avoid inserting into multiple nodes
			}
		}
		// If there's no matching branch that can contain it, check if a new branch should be created for it
		if (this->mayBranch(leaf)) {
			// Create a new branch for the leaf
			IVector3D middle = (this->minOwnedBound + this->maxOwnedBound) / 2;
			int newBranchIndex = Octree_getBranchIndex(leaf.origin.x >= middle.x, leaf.origin.y >= middle.y, leaf.origin.z >= middle.z);
			assert(this->childNodes[newBranchIndex].get() == nullptr);
			IBox3D childRegion = splitBound(IBox3D(this->minOwnedBound, this->maxOwnedBound), newBranchIndex);
			this->childNodes[newBranchIndex] = std::make_shared<OctreeNode<T>>(leaf, childRegion.min, childRegion.max);
		} else {
			// Add the leaf
			this->leaves.push(leaf);
			// Split when the direct leaves are too many
			if (this->leaves.length() > 64) {
				// Split the node into branches
				this->divided = true;
				List<OctreeLeaf<T>> oldLeaves = this->leaves;
				this->leaves.clear();
				for (int l = 0; l < oldLeaves.length(); l++) {
					this->insert(oldLeaves[l]);
				}
			}
		}
	}
	void find(const OcTreeFilter& boundFilter, const OcTreeLeafOperation<T>& leafOperation) {
		if (boundFilter(this->minLeafBound, this->maxLeafBound)) {
			for (int l = 0; l < this->leaves.length(); l++) {
				this->leaves[l].find(boundFilter, leafOperation);
			}
			for (int n = 0; n < 8; n++) {
				if (this->childNodes[n].get() != nullptr) {
					this->childNodes[n]->find(boundFilter, leafOperation);
				}
			}
		}
	}
};

template <typename T>
class Octree {
private:
	// One start node for each direction to simplify expansion
	std::shared_ptr<OctreeNode<T>> childNodes[8];
	// Settings
	int initialSize; // Should be around the average total world size to create the most balanced trees
public:
	explicit Octree(int initialSize)
	: initialSize(initialSize) {}
public:
	// Precondition: minBound <= origin <= maxBound
	void insert(const T& leaf, const IVector3D& origin, const IVector3D& minBound, const IVector3D& maxBound) {
		int sideIndex = Octree_getBranchIndex(origin.x >= 0, origin.y >= 0, origin.z >= 0);
		if (this->childNodes[sideIndex].get() == nullptr) {
			// Calculate minimum required size
			int requiredSize = std::max(abs(origin.x), std::max(abs(origin.y), abs(origin.z)));
			// Calculate final cube size to be stored directly inside of the root
			int size = this->initialSize;
			while(size < requiredSize) {
				size *= 2;
			}
			IVector3D minOwnedBound = IVector3D(
			  origin.x < 0 ? -size : 0,
			  origin.y < 0 ? -size : 0,
			  origin.z < 0 ? -size : 0
			);
			IVector3D maxOwnedBound = IVector3D(
			  origin.x < 0 ? 0 : size,
			  origin.y < 0 ? 0 : size,
			  origin.z < 0 ? 0 : size
			);
			this->childNodes[sideIndex] = std::make_shared<OctreeNode<T>>(OctreeLeaf<T>(leaf, origin, minBound, maxBound), minOwnedBound, maxOwnedBound);
		} else {
			this->childNodes[sideIndex]->insert(OctreeLeaf<T>(leaf, origin, minBound, maxBound));
		}
	}
	// Find leaves using a custom filter
	void map(const OcTreeFilter& boundFilter, const OcTreeLeafOperation<T>& leafOperation) {
		for (int sideIndex = 0; sideIndex < 8; sideIndex++) {
			if (this->childNodes[sideIndex].get() != nullptr) {
				this->childNodes[sideIndex]->find(boundFilter, leafOperation);
			}
		}
	}
	// Find leaves using an axis aligned search box
	//   Each leaf who's bounding box is touching the search box will be given as argument to the leafOperation callback
	void map(const IVector3D& searchMinBound, const IVector3D& searchMaxBound, const OcTreeLeafOperation<T>& leafOperation) {
		this->map([searchMinBound, searchMaxBound](const IVector3D& minBound, const IVector3D& maxBound){
			return searchMaxBound.x >= minBound.x && searchMinBound.x <= maxBound.x
			    && searchMaxBound.y >= minBound.y && searchMinBound.y <= maxBound.y
			    && searchMaxBound.z >= minBound.z && searchMinBound.z <= maxBound.z;
		}, leafOperation);
	}
};

}

#endif

