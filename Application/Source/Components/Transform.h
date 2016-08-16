#pragma once
#include "Component.h"
#include <DirectXMath.h>

using namespace DirectX;

class Transform : public Component
{
public:
	//static Transform* Deserialize(Document & params);
	//virtual void		Serialize(Value& val, Document::AllocatorType& allocator) override;
	static const ComponentType ComponentId = Component::TRANSFORM;

	static const XMVECTOR Right		;//= XMVectorSet(+1.0f, +0.0f, +0.0f, 0.0f);
	static const XMVECTOR Left		;//= XMVectorSet(-1.0f, +0.0f, +0.0f, 0.0f);
	static const XMVECTOR Up		;//= XMVectorSet(+0.0f, +1.0f, +0.0f, 0.0f);
	static const XMVECTOR Down		;//= XMVectorSet(+0.0f, -1.0f, +0.0f, 0.0f);
	static const XMVECTOR Forward	;//= XMVectorSet(+0.0f, +0.0f, +1.0f, 0.0f);
	static const XMVECTOR Backward	;//= XMVectorSet(+0.0f, +0.0f, -1.0f, 0.0f);

	// CONSTRUCTOR / DESTRUCTOR
	//-----------------------------------------------------------------------------------
	Transform(	const XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f),
				const XMFLOAT3 rotation = XMFLOAT3(0.0f, 0.0f, 0.0f),
				const XMFLOAT3 scale	= XMFLOAT3(1.0f, 1.0f, 1.0f));
	~Transform();

	// GETTERS & SETTERS
	//-----------------------------------------------------------------------------------
	inline XMVECTOR GetPosition() { return XMLoadFloat3(&m_position); }
	inline XMVECTOR GetRotation() { return XMLoadFloat3(&m_rotation); }
	inline XMVECTOR GetScale()    { return XMLoadFloat3(&m_scale   ); }
	//inline glm::quat GetOrientation() { return mOrientation_; }

	inline void SetRotation	(XMFLOAT3 rot)				{ m_rotation = rot; }
	inline void SetScale	(XMFLOAT3 scale)			{ m_scale = scale;	}
	inline void SetPosition	(XMFLOAT3 val)				{ m_position = val; }
	inline void SetPosition(float x, float y, float z)	{ m_position = XMFLOAT3(x, y, z); }
	//inline void SetOrientation(glm::quat orient) { mOrientation_ = orient; }

	// MOVEMENT
	//-----------------------------------------------------------------------------------
	// Translates the position by the provided vector
	void Translate(XMVECTOR translation);

	// Rotates the transform by provided vector in the selected axis (0:x 1:y 2:z)
	void Rotate(XMVECTOR rotation);	// TODO: definition doesn't match explanation. rework Rotate();

	void Scale(XMVECTOR scl);

	// Returns the Model Transformation Matrix
	XMMATRIX ModelTransformationMatrix() const;
	XMMATRIX RotationMatrix() const;

	// transforms a vector from local to global space
	XMFLOAT3 TransfromVector(XMFLOAT3 v);

private:
	XMFLOAT3 m_position;			
	XMFLOAT3 m_rotation;			
	XMFLOAT3 m_scale;				

	//	glm::quat mOrientation_;	
};

