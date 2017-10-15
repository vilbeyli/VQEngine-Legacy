#pragma once
#include "Scene.h"
class IBLTestScene : public Scene
{
public:

public:
	void Load(SerializedScene& scene) override;
	void Update(float dt) override;
	void Render(const SceneView& sceneView, bool bSendMaterialData) const override;

	void GetShadowCasters(std::vector<const GameObject*>& casters) const override;	// todo: rename this... decide between depth and shadows
	void GetSceneObjects(std::vector<const GameObject*>&) const override;

	IBLTestScene(SceneManager& sceneMan, std::vector<Light>& lights);
	~IBLTestScene() = default;

private:
	std::vector<GameObject> spheres;
};

