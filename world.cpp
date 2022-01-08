#include "world.h"

World::World() {

}

void World::Split(KDNode* node, int depth) {

}

void World::Clear(KDNode* node) {

}

bool World::IntersectHelper(const Ray &ray, KDNode *node, HitResult &hitResult, bool shadow) {

}

void World::ReadObjects(const std::string filename) {

}

void World::AddObjects(const Object &obj) {

}

void World::ClearObjects() {

}

void World::Build() {

}

bool World::Intersect(const Ray &ray, HitResult &hitResult, Object &hitObject, bool shadow) {

}

Object& World::GetObjectRef(int i) {

}
