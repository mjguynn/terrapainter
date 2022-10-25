#pragma once

#include <vector>
#include "terrapainter/math.h"

// TODO:
// This system is pretty simple because our app is pretty simple,
// but it's not very good for performance reasons.
// 
// In the future it would be good to do an ECS/data oriented model
// with components.

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
	// Other entities may hold pointers to this entity,
	// so let's not allow moving it or copying it
	Entity(const Entity&) = delete;
	Entity& operator= (const Entity&) = delete;
	Entity(Entity&&) = delete;
	Entity& operator= (Entity&&) = delete;

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

class World : public Entity {
public:
	World() : Entity(vec3::zero(), vec3::zero(), vec3::splat(1)) {}
};