#include "terrapainter/entity.h"
#include <algorithm>

Entity::Entity(vec3 position, vec3 euler_angles, vec3 scale) :
	mPosition(position),
	mAngles(euler_angles),
	mScale(scale),
	mParent(nullptr),
	mChildren(),
	mBakedWorldTransform(),
	mBakedWorldTransformDirty(true) 
{}
void Entity::bake_world_transform() const {
	mat4 s = mat4::diag(mScale.x, mScale.y, mScale.z, 1);
	mat4 r = mat3::euler(mAngles.x, mAngles.y, mAngles.z).hmg();
	mat4 t = mat4::translate_hmg(mPosition);
	mat4 localTransform = t * r * s;
	mBakedWorldTransform = mParent->world_transform() * localTransform;
	mBakedWorldTransformDirty = false;
}
std::unique_ptr<Entity> Entity::remove_child(Entity* child) {
	for (size_t i = 0; i < mChildren.size(); ++i) {
		if (mChildren[i].get() != child) continue;
		std::unique_ptr<Entity> stolen = std::move(mChildren[i]);
		stolen->mParent = nullptr;
		std::swap(mChildren[i], mChildren.back());
		return stolen;
	}
	return nullptr;
}
void Entity::add_child(std::unique_ptr<Entity> child) {
	assert(child->mParent == nullptr);
	child->mParent = this;
	mChildren.emplace_back(std::move(child));
}
Entity::~Entity() {
	// We don't have to worry about our children.
	// They will be killed by the vector's destructor.
	// 
	// We do have to worry about our parent, if we have one...
	// but wait! The parent holds the unique_ptr to us.
	// So either the parent is being destroyed itself, or we
	// were stripped from our parent by remove_parent.
	// In either case we have nothing to do.
}