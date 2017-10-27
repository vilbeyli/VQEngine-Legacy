// ---------------------------------------------------------------------------
// Author			:	Volkan Ilbeyli
// Creation Date	:	2016/1/14
// Purpose			:	Base class for components to inherit from and implement interfaces
// ---------------------------------------------------------------------------

//#include "Components.h"
#include <iostream>

//Component* Component::DeSerializeComponent(std::string name, Document & doc)
//{
//	// relevant SO question
//	// http://stackoverflow.com/questions/582331/is-there-a-way-to-instantiate-objects-from-a-string-holding-their-class-name
//	
//	Component * comp = NULL;
//
//	if (name == "transform")					comp =			   Transform::Deserialize(doc);
//	else if (name == "modelcomponent")			comp =			   ModelComponent::Deserialize(doc);
//	else if (name == "playercontroller")		comp =			   PlayerController::Deserialize(doc);
//	else if (name == "physicscomponent")		comp =			   PhysicsComponent::Deserialize(doc);
//	else if (name == "cameracomponent")			comp =			   CameraComponent::Deserialize(doc);
//	else if (name == "light")					comp =			   Light::Deserialize(doc);
//	else if (name == "freecam")					comp =			   FreeCam::Deserialize(doc);
//	else std::cout << "WARNING: Trying to create an unknown type of component: " << name << std::endl;
//
//	return comp;
//}
