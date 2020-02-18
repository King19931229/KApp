#include "KOctreeSceneManager.h"

KOctreeSceneManager::KOctreeSceneManager()
	: m_Root(nullptr)
{
}

KOctreeSceneManager::~KOctreeSceneManager()
{
	assert(!m_Root);
}

bool KOctreeSceneManager::Init(float baseLengthVal, float minSizeVal, float loosenessVal, const glm::vec3& centerVal)
{
	UnInit();
	m_Root = new KOctreeNode(baseLengthVal, minSizeVal, loosenessVal, centerVal);
	return true;
}

bool KOctreeSceneManager::UnInit()
{
	SAFE_DELETE(m_Root);
	return true;
}

bool KOctreeSceneManager::Add(KEntity* entity)
{
	// TODO
	return false;
}

bool KOctreeSceneManager::Remove(KEntity* entity)
{
	// TODO
	return false;
}

bool KOctreeSceneManager::Move(KEntity* entity)
{
	// TODO
	return false;
}

bool KOctreeSceneManager::GetVisibleEntity(const KCamera* camera, std::vector<KEntity*>& visibles)
{
	// TODO
	return false;
}