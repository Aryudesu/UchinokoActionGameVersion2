#include "Object.h"
#include "Block.h"
#include "Action.h"
#include "Singleton.h"

#pragma once

class ObjectFactory : public Singleton<ObjectFactory>{
public:
	Object * Factory(int ID, double initx, double inity);
};

class EffectFactory : public Singleton<EffectFactory> {
public:
	Object * Factory(int ID,double initx, double inity);
};

class EffectObjFactory : public Singleton<EffectObjFactory> {
public:
	Object* Factory(int ID, double initx, double inity);
};


class ScoreEffectFactory : public Singleton<ScoreEffectFactory> {
public:
	Object * Factory(int ID,double initx, double inity);
};

class BlockFragmentFactory : public Singleton<BlockFragmentFactory> {
public:
	Object* Factory(int ID, double initx, double inity);
};
