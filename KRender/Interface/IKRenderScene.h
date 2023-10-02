#pragma once
#include "KBase/Interface/Entity/IKEntity.h"
#include "KRender/Interface/IKTerrain.h"
#include "KRender/Publish/KCamera.h"
#include "glm/glm.hpp"

enum SceneManagerType
{
	SCENE_MANGER_TYPE_OCTREE
};

enum EntitySceneOp
{
	ESO_ADD,
	ESO_REMOVE,
	ESO_TRANSFORM
};

typedef std::function<void(EntitySceneOp, IKEntity*)> EntityObserverFunc;

struct IKRenderScene;

struct IKRayTraceScene
{
	virtual ~IKRayTraceScene() {}
	virtual bool Init(IKRenderScene* scene, const KCamera* camera) = 0;
	virtual bool UnInit() = 0;
	virtual bool UpdateCamera() = 0;
	virtual bool EnableDebugDraw() = 0;
	virtual bool DisableDebugDraw() = 0;
	virtual bool EnableAutoUpdateImageSize(float scale) = 0;
	virtual bool EnableCustomImageSize(uint32_t width, uint32_t height) = 0;
	virtual bool DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer) = 0;
	virtual bool Execute(IKCommandBufferPtr primaryBuffer) = 0;
	virtual bool AddRaytracePipeline(IKRayTracePipelinePtr& pipeline) = 0;
	virtual bool RemoveRaytracePipeline(IKRayTracePipelinePtr& pipeline) = 0;
	virtual const KCamera* GetCamera() = 0;
	virtual IKAccelerationStructurePtr GetTopDownAS() = 0;
};
typedef std::shared_ptr<IKRayTraceScene> IKRayTraceScenePtr;

struct IKVirtualGeometryScene
{
	virtual ~IKVirtualGeometryScene() {}
	virtual bool Init(IKRenderScene* scene, const KCamera* camera) = 0;
	virtual bool UnInit() = 0;
	virtual bool ExecuteMain(IKCommandBufferPtr primaryBuffer) = 0;
	virtual bool ExecutePost(IKCommandBufferPtr primaryBuffer) = 0;
	virtual bool BasePass(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer) = 0;
	virtual bool DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer) = 0;
};
typedef std::shared_ptr<IKVirtualGeometryScene> IKVirtualGeometryScenePtr;

struct IKRenderScene
{
	virtual ~IKRenderScene() {}

	virtual bool Init(const std::string& name, SceneManagerType type, float initialSize, const glm::vec3& initialPos) = 0;
	virtual bool UnInit() = 0;

	virtual bool InitRenderResource(const KCamera* camera) = 0;
	virtual bool UnInitRenderResource() = 0;

	virtual const std::string& GetName() const = 0;

	virtual bool Add(IKEntity* entity) = 0;
	virtual bool Remove(IKEntity* entity) = 0;
	virtual bool Transform(IKEntity* entity) = 0;

	virtual bool CreateTerrain(const glm::vec3& center, float size, float height, const KTerrainContext& context) = 0;
	virtual bool DestroyTerrain() = 0;
	virtual IKTerrainPtr GetTerrain() = 0;

	virtual bool RegisterEntityObserver(EntityObserverFunc* func) = 0;
	virtual bool UnRegisterEntityObserver(EntityObserverFunc* func) = 0;

	virtual bool GetSceneObjectBound(KAABBBox& box) = 0;

	virtual bool Pick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, std::vector<IKEntity*>& result) = 0;
	virtual bool CloestPick(const KCamera& camera, size_t x, size_t y,
		size_t screenWidth, size_t screenHeight, IKEntity*& result) = 0;

	virtual bool RayPick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntity*>& result) = 0;
	virtual bool CloestRayPick(const glm::vec3& origin, const glm::vec3& dir, IKEntity*& result) = 0;

	virtual bool GetAllEntities(std::vector<IKEntity*>& result) = 0;

	virtual bool GetVisibleEntities(const KCamera& camera, std::vector<IKEntity*>& result) = 0;
	virtual bool GetVisibleEntities(const KAABBBox& bound, std::vector<IKEntity*>& result) = 0;

	virtual IKRayTraceScenePtr GetRayTraceScene() = 0;
	virtual IKVirtualGeometryScenePtr GetVirtualGeometryScene() = 0;
};

typedef std::unique_ptr<IKRenderScene> IKRenderScenePtr;

EXPORT_DLL IKRenderScenePtr CreateRenderScene();