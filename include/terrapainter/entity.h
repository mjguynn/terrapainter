#pragma once

#include <vector>
#include "terrapainter/math.h"

// Potential idea for future:
// One-parent/many-dependent relationship, where a dependency is either:
// 1. child: you own the dependency and delete it when you're deleted
//			 child's dependency is relative to you, you are the childs parent
// 2. mutual: neither owns the other, separate transforms
//			  whenever one is modified they broadcast a signal to mutuals
// I need to think this out and it might actually be a horrible no good
// very bad idea for cache locality and general code maintenance reasons
// (spaghetti code)
// ...but I feel like in any system of sufficient complexity you inevitably
// have a dependency between the queen of England and the hounds of hell

class Entity {
private:
	// Position aka translation
	vec3 mPosition;
	// Euler angles
	vec3 mAngles;
	// Axis scale
	vec3 mScale;

	// The entity's baked transformation matrix
	mutable mat4 mBakedWorldTransform;
	// Indicates whether mBakedTransform must be rebuilt to
	// reflect changes
	mutable bool mBakedWorldTransformDirty;

	// A pointer to the entity's parent in the scenegraph.
	// If this is null, the entity is a scene root.
	Entity* mParent;
	// The entity's children in the scenegraph.
	// Note that an entity owns its children.
	std::vector<std::unique_ptr<Entity>> mChildren;

private:
	// Logically const, but actually re-bakes the transform
	// This is recursive right now
	void bake_world_transform() const;

protected:
	Entity(vec3 position, vec3 euler_angles, vec3 scale);

public:
	virtual ~Entity();

	// Returns the matrix mapping a scene node to the world.
	// Each scene node has some associated transform, even
	// if it doesn't make any sense (like the canvas)
	mat4 world_transform() const {
		if (mBakedWorldTransformDirty) bake_world_transform();
		return mBakedWorldTransform;
	}

	vec3 position() const { return mPosition; }
	void set_position(vec3 position) {
		mPosition = position;
		mBakedWorldTransformDirty = true;
	}

	vec3 euler_angles() const { return mAngles; };
	void set_euler_angles(vec3 angles) { 
		mAngles = angles; 
		mBakedWorldTransformDirty = true;
	};

	vec3 scale() const { return mScale; }
	void set_scale(vec3 scale) { 
		mScale = scale;
		mBakedWorldTransformDirty = true;
	}

	Entity const* parent() const { return mParent; }
	auto children() const { return mChildren.begin(); }

	// Removes `id` from the child list, updating its parent pointer,
	// and returns an owning pointer to the child.
	// If the child wasn't found, returns a null pointer.
	std::unique_ptr<Entity> remove_child(Entity* child);

	// Sets this entity as `child`s parent.
	// The entity must not be currently parented.
	// (if it was, how did you get the unique pointer?)
	void add_child(std::unique_ptr<Entity> child);

};