#pragma once
class Scene {
protected:
public:
	Scene() {};
	virtual ~Scene() = default;
	virtual void update() = 0;
	virtual void draw() = 0;
};
