//	DX11Renderer - VDemo | DirectX11 Renderer
//	Copyright(C) 2016  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com
#pragma once

#include "Utilities/vectormath.h"

#include <array>
#include <sstream>
#include <iomanip>

struct PointLightGPU	// 48 Bytes | 3 registers
{	
	vec3 position;
	float  range;
	vec3 color;
	float  brightness;
	vec2 attenuation;
	vec2 padding;
};

struct SpotLightGPU		// 48 bytes | 3 registers
{
	vec3 position;
	float  halfAngle;
	vec3 color;
	float  brightness;
	vec3 spotDir;
	float padding;
};

struct DirectionalLightGPU // 28(+4) Bytes | 2 registers
{
	vec3 lightDirection;
	float  brightness;
	vec3 color;
};

//struct ShadowView
//{
//	TextureID shadowMap = -1;
//	SamplerID shadowSampler = -1;
//	XMMATRIX  lightSpaceMatrix;
//};

#define NUM_POINT_LIGHT 100
#define NUM_SPOT_LIGHT 20
#define NUM_DIRECTIONAL_LIGHT 4
using PointLightDataArray		= std::array<PointLightGPU, NUM_POINT_LIGHT>;
using SpotLightDataArray		= std::array<SpotLightGPU, NUM_SPOT_LIGHT>;
using DirectionalLightDataArray = std::array<DirectionalLightGPU, NUM_DIRECTIONAL_LIGHT>;

#define NUM_POINT_LIGHT_SHADOW 5
#define NUM_SPOT_LIGHT_SHADOW 5
#define NUM_DIRECTIONAL_LIGHT_SHADOW 4
using ShadowingPointLightDataArray			= std::array<PointLightGPU, NUM_POINT_LIGHT_SHADOW>;
using ShadowingSpotLightDataArray			= std::array<SpotLightGPU, NUM_SPOT_LIGHT_SHADOW>;
using ShadowingDirectionalLightDataArray	= std::array<DirectionalLightGPU, NUM_DIRECTIONAL_LIGHT_SHADOW>;

using SpotShadowViewArray = std::array<XMMATRIX, NUM_SPOT_LIGHT_SHADOW>;

struct SceneLightingData
{
	struct cb{	// shader constant buffer
		int       pointLightCount;
		int        spotLightCount;
		int directionalLightCount;

		int       pointLightCount_shadow;
		int        spotLightCount_shadow;
		int directionalLightCount_shadow;
		int _pad0, _pad1;

		PointLightDataArray pointLights;
		ShadowingPointLightDataArray pointLightsShadowing;

		SpotLightDataArray spotLights;
		ShadowingSpotLightDataArray spotLightsShadowing;

		// todo directional
		// todo directional shadowing

		SpotShadowViewArray shadowViews;
	} _cb;


	inline void ResetCounts() {
		_cb.pointLightCount = _cb.spotLightCount = _cb.directionalLightCount =
		_cb.pointLightCount_shadow = _cb.spotLightCount_shadow = _cb.directionalLightCount_shadow = 0;
	}
};

class TextRenderer;
struct TextDrawDescription;

struct RendererStats
{
	union
	{
		struct  
		{
			int numVertices;
			int numIndices;
			int numDrawCalls;
		};
		int arr[3];
	};
};
struct FrameStats
{
	static const size_t numStat = 5;
	union 
	{
		struct
		{
			int numSceneObjects;
			int numCulledObjects;
			int numDrawCalls;
			int numVertices;
			int numIndices;
		};
		struct
		{
			int numSceneObjects;
			int numCulledObjects;
			RendererStats rstats;
		};
		int stats[numStat];
	};

	bool bShow;
	void Render(TextRenderer* pTextRenderer, const vec2& screenPosition, const TextDrawDescription& drawDesc);	// implementation in Engine.cpp
	static const char* statNames[numStat];
};


// TEMPLATE TREENODE AND TREE HEADER
//================================================================================================================================================
template<class T> struct TreeNode
{
	const T*				pData;
	TreeNode*				pParent;
	std::vector<TreeNode>	children;

	bool operator<(const TreeNode<T>& other) { return pData->operator<(*other.pData); }
};

template<class T> struct Tree
{
	template<class T>
	using pFnPredicate = bool (*)(const TreeNode<T>&, const TreeNode<T>&);

public:	// INTERFACE
	// Adds a child node with pData to the given parent
	//
	TreeNode<T>* AddChild(TreeNode<T>& parent, const T* pData);

	// Sorts the tree on each level based on the comparison function
	//
	void Sort(pFnPredicate<T> fnLessThanComparison);
	void Sort();

	// Renders the tree.
	//
	void RenderTree(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription& drawDesc);

	// Returns a const pointer to TreeNode with the given tag
	//
	const TreeNode<T>* FindNode(const std::string& tag) const;

public:	// DATA
	TreeNode<T> root = { nullptr, nullptr, std::vector<TreeNode<T>>() };


private:
	// Finds the TreeNode given address of the TreeNode's data.
	//
	TreeNode<T>* FindNode(const T* pData) const;

	// searches among children | depth first, returns nullptr if not found
	//
	TreeNode<T>* SearchSubTree(const TreeNode<T>& node, const T* pSearchEntry) const;
	const TreeNode<T>* SearchSubTree(const TreeNode<T>& node, const std::string& tag) const;

	void RenderSubTree(
		const TreeNode<T>&		node, 
		TextRenderer *			pTextRenderer, 
		const vec2 &			screenPosition, 
		TextDrawDescription&	drawDesc, 
		std::ostringstream&		stats
	);

	void SortSubTree(TreeNode<T>& node, pFnPredicate<T> fnLessThanComparison);
	void SortSubTree(TreeNode<T>& node);
};


// TEMPLATE TREENODE AND TREE IMPLEMENTATION
// ===============================================================================================================================================
template<class T>
inline TreeNode<T> * Tree<T>::AddChild(TreeNode<T>& parent, const T * pData)
{
	TreeNode<T> newNode = { pData, &parent, std::vector<TreeNode<T>>() };
	parent.children.push_back(newNode);
	return &parent.children.back();	// is this safe? this might not be safe...
}

template<class T>
inline void Tree<T>::Sort(pFnPredicate<T> fnLessThanComparison)
{ 
	SortSubTree(root, fnLessThanComparison);
}

template<class T>
inline void Tree<T>::Sort()
{
	SortSubTree(root);
}

template<class T>
inline void Tree<T>::RenderTree(TextRenderer * pTextRenderer, const vec2 & screenPosition, TextDrawDescription & drawDesc)
{
	std::ostringstream stats;	// set stream properties once, 
	stats.precision(2);			// and pass it to the recursive 
	stats << std::fixed;		// function.
	RenderSubTree(root, pTextRenderer, screenPosition, drawDesc, stats);
}

template<class T>
inline TreeNode<T>* Tree<T>::FindNode(const T * pData) const
{
	if (root.pData == pData) return &root;
	return SearchSubTree(root, pData);
}
template<class T>
inline const TreeNode<T>* Tree<T>::FindNode(const std::string& tag) const
{
	if (root.pData->tag == tag) return &root;
	return SearchSubTree(root, tag);
}


template<class T>
inline TreeNode<T>* Tree<T>::SearchSubTree(const TreeNode<T>& node, const T * pSearchEntry) const
{
	TreeNode<T>* pSearchResult = nullptr;

	// visit children | depth first
	for (size_t i = 0; i < node.children.size(); i++)
	{
		TreeNode<T>* pEntryNode = &root.children[i];
		if (pSearchEntry == pEntryNode->pData)
			return pEntryNode;

		pSearchResult = SearchSubTree(*pEntryNode, pSearchEntry);
		if (pSearchResult && pSearchResult->pData == pSearchEntry)
			return pSearchResult;
	}

	return pSearchResult;
}
template<class T>
inline const TreeNode<T>* Tree<T>::SearchSubTree(const TreeNode<T>& node, const std::string& tag) const
{
	const TreeNode<T>* pSearchResult = nullptr;

	// visit children | depth first
	for (size_t i = 0; i < node.children.size(); i++)
	{
		const TreeNode<T>* pEntryNode = &root.children[i];
		if (tag == pEntryNode->pData->tag)
			return pEntryNode;

		pSearchResult = SearchSubTree(*pEntryNode, tag);
		if (pSearchResult && pSearchResult->pData->tag == tag)
			return pSearchResult;
	}

	return pSearchResult;
}

template<class T>
inline void Tree<T>::RenderSubTree(const TreeNode<T>& node, TextRenderer * pTextRenderer, const vec2 & screenPosition, TextDrawDescription & drawDesc, std::ostringstream & stats)
{
	const int X_OFFSET = 20;	// todo: DrawSettings struct for the tree?
	const int Y_OFFSET = 22;

	// clear & populate text
	stats.clear(); stats.str("");
	stats << node.pData->tag << "  " << node.pData->GetAvg() * 1000 << " ms";

	drawDesc.screenPosition = screenPosition;
	drawDesc.text = stats.str();
	pTextRenderer->RenderText(drawDesc);

	size_t row_count = 0;
	for (size_t i = 0; i < node.children.size(); i++)
	{
		row_count += i == 0 ? 0 : node.children[i - 1].children.size();
		RenderSubTree(node.children[i], pTextRenderer, { screenPosition.x() + X_OFFSET, screenPosition.y() + Y_OFFSET * (i + 1 + row_count) }, drawDesc, stats);
	}
}

template<class T>
inline void Tree<T>::SortSubTree(TreeNode<T>& node, pFnPredicate<T> fnLessThanComparison)
{
	std::sort(node.children.begin(), node.children.end(), fnLessThanComparison);
	std::for_each(node.children.begin(), node.children.end(), [this, fnLessThanComparison](TreeNode<T>& n) { SortSubTree(n, fnLessThanComparison); });
}


template<class T>
inline void Tree<T>::SortSubTree(TreeNode<T>& node)
{
	std::sort(node.children.begin(), node.children.end());
	std::for_each(node.children.begin(), node.children.end(), [this](TreeNode<T>& n) { SortSubTree(n); });
}
