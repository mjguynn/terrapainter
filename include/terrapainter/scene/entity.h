#pragma once

#include <vector>
#include <memory>

#include "terrapainter/math.h"

// TODO:
// This system is pretty simple because our app is pretty simple,
// but it's not very good for performance reasons.
// 
// In the future it would be good to do an ECS/data oriented model
// with components.

// TODO:
// move this
struct RenderCtx {
	mat4 viewProj;
	vec4 cullPlane;
	vec3 viewPos;
	vec3 sunDir;
	vec3 sunColor;
	ivec2 viewportSize;
	bool inWaterPass;
};
class Entity {
private:
	// Position aka translation
	vec3 mPosition;
	// XYZ angles (aka: rotation around X axis, Y axis, Z axis)
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

	// TODO: This is terrible
	void set_world_transform_dirty() {
		if (!mBakedWorldTransformDirty) {
			mBakedWorldTransformDirty = true;
			for (auto& child : mChildren) {
				child->set_world_transform_dirty();
			}
		}
	}
protected:
	Entity(vec3 position, vec3 angles, vec3 scale);

public:
	virtual ~Entity() noexcept;
	// TODO: Feels weird to put this here. Kind of a hack.
	virtual void draw(const RenderCtx& c) const {}

	// Returns the matrix mapping a scene node to the world.
	// Each scene node has some associated transform, even
	// if it doesn't make any sense (like the canvas)
	mat4 world_transform() const {
		bake_world_transform();
		return mBakedWorldTransform;
	}

	vec3 position() const { return mPosition; }
	void set_position(vec3 position) {
		mPosition = position;
		set_world_transform_dirty();
	}

	// XYZ angles
	vec3 angles() const { return mAngles; };
	void set_angles(vec3 angles) { 
		mAngles = angles; 
		set_world_transform_dirty();
	};

	vec3 scale() const { return mScale; }
	void set_scale(vec3 scale) { 
		mScale = scale;
		set_world_transform_dirty();
	}

	Entity const* parent() const { return mParent; }

	// This is the wrong type, it should be a newtyped iterator
	// But iterators in C++ are painful.
	const std::vector<std::unique_ptr<Entity>>& children() const { return mChildren; }

	// Removes `id` from the child list, updating its parent pointer,
	// and returns an owning pointer to the child.
	// If the child wasn't found, returns a null pointer.
	std::unique_ptr<Entity> remove_child(Entity* child);

	// Sets this entity as `child`s parent.
	// The entity must not be currently parented.
	// (if it was, how did you get the unique pointer?)
	void add_child(std::unique_ptr<Entity> child);
};