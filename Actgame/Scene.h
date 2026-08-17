#pragma once
class Scene {
protected:
public:
	Scene() {};
	virtual void update() = 0;
	virtual void draw() = 0;
};