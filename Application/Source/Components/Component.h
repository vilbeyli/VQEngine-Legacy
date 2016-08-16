// ---------------------------------------------------------------------------
// Author			:	Volkan Ilbeyli
// Creation Date	:	2015/10/27
// Purpose			:	Base class for components to be used in game objects
// ---------------------------------------------------------------------------

#ifndef COMPONENT_H
#define COMPONENT_H

#include <string>
#include <algorithm>
#include <iostream>


class GameObject;

class Component
{
public:
	/*
		HOW TO ADD NEW COMPONENT
		 - Update enum ComponentType (just below)
		 - Update Components.h - #include "NewComponentName.h"
		 - Update DeSerializeComponent() function in Component.cpp
		 - Define a Deserialize() function in the new component (see transform for e.g.)
	*/
	enum ComponentType
	{			
		// core system components
		TRANSFORM = 0,
		MODELCOMPONENT,
		CAMERA_COMPONENT,
		LIGHT,
		FREECAM,
		//PHYSICS,
		
		// behavioral components
		PLAYER_CONTROLLER,

		COMPONENT_COUNT
	};

	//-----------------------------------------------------------------------------------


	//static Component* Component::DeSerializeComponent(std::string name, Document & doc);
	//virtual void Serialize(Value& componentJSONObject, Document::AllocatorType& allocator){};

	virtual					~Component() { /*std::cout << "component dtor" << std::endl;*/ }
	virtual void			SetOwner(GameObject* obj)	{ mOwner_ = obj; }
	virtual GameObject*		GetOwner() const			{ return  mOwner_; }
	virtual std::string		Name() const				{ return mName_; }
	virtual void			Update()					{};
	virtual void			Clean()						{};
			ComponentType	GetType()					{ return mType_; }
	
	template <typename ComponentName>
	static ComponentName* CopyComponent(ComponentName*);

protected:
	Component(ComponentType type, std::string name) : mType_(type), mOwner_(nullptr), mName_(name){}

protected:	
	ComponentType mType_;
	GameObject * mOwner_;
	std::string mName_;
};

template <typename ComponentName>
ComponentName* Component::CopyComponent(ComponentName* c)
{
	return new ComponentName(c);
}
#endif