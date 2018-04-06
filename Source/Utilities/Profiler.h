#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <thread>
#include <limits>
#include <array>

#include "PerfTimer.h"
#include "Renderer/TextRenderer.h"

#ifdef max
#undef max
#endif

class TextRenderer;
struct vec2;

struct ProfilerSettings
{
	int sampleCount = 50;	// 
	int refreshRate = 5;	// every 5 frames
};

struct PerfEntry
{
	std::string			name;
	size_t				currSampleIndex; // [0, samples.size())
	std::vector<float>	samples;
	PerfTimer			timer;

public:
	void UpdateSampleBegin();
	void UpdateSampleEnd();
	void PrintEntryInfo(bool bPrintAllEntries = false);
	inline float GetAvg() const;
};

class CPUProfiler
{
// Tree Hierarchy for PerfEntries & State structs
//==================================================================================================================
	using PerfEntryTable = std::unordered_map<std::string, PerfEntry>;

private:
	struct PerfEntryNode
	{
		// PerfEntryNode()
		const PerfEntry /*const*/*		pPerfEntry;	// payload
		PerfEntryNode   /*const*/*		pParent;	// parent*
		std::vector<PerfEntryNode>		children;	// children
	};

	struct PerfEntryTree
	{
	public:	// INTERFACE
		// constructs PerfEntryNode for pPerfEntry and adds it to parentNode's children container
		PerfEntryNode * AddChild(PerfEntryNode& parentNode, const PerfEntry * pPerfEntry);

		// sorts children based on average of the samples in pPerfEntry
		void Sort();

		// renders the tree
		void RenderTree(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription& drawDesc);

	public:	// DATA
		PerfEntryNode root = { nullptr, nullptr, std::vector<PerfEntryNode>() };


	private:
		// returns PerfEntryNode pointer which holds the pNode in the parameter as its pNode (member)
		PerfEntryNode * FindNode(const PerfEntry* pNode);

		// searches among children | depth first, returns nullptr if not found
		PerfEntryNode* SearchSubTree(const PerfEntryNode& node, const PerfEntry* pSearchEntry);

		void RenderSubTree(const PerfEntryNode& node, TextRenderer * pTextRenderer, const vec2 & screenPosition, TextDrawDescription & drawDesc, std::ostringstream& stats);
		void SortSubTree(PerfEntryNode& node);

	};

	struct State
	{
		bool					bIsProfiling = false;
		std::stack<std::string> EntryNameStack;
		PerfEntryNode*			pLastEntryNode = nullptr;	// used for setting up the hierarchy
	};



// Profiler
//==================================================================================================================
public:
	CPUProfiler(ProfilerSettings settings = ProfilerSettings()) : 	mSettings(settings) {}

	// Resets PerfEntryTable. BeginEntry()/EndEntry() must be called between BeginProfile() and EndProfile()
	void BeginProfile();
	void EndProfile();

	// Adds PerfEntry to the PerfEntryTable if it doesn't already exist AND Starts the time sampling for the PerfEntry.
	// BeginEntry()/EndEntry() must be called between BeginProfile() and EndProfile()
	// *Assumes PerfEntry has a unique name. There won't be duplicate entries.*
	//
	void BeginEntry(const std::string& entryName);
	void EndEntry();

	inline bool AreThereAnyOpenEntries() const { return !mState.EntryNameStack.empty(); }
	inline float GetEntryAvg(const std::string& entryName) const { return mPerfEntries.at(entryName).GetAvg(); }


	// performs checks for state consistency (are there any open entries? etc.)
	//
	bool StateCheck() const;

	// prints states into debug console output
	//
	void PrintStats() const;

	// renders performance stats tree, starting at @screenPosition using @drawDesc
	//
	void RenderPerformanceStats(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription drawDesc, bool bSortStats);
	
private:
	PerfEntryTable		mPerfEntries;		// where data lives (with entry name as unique keys)
	PerfEntryTree		mPerfEntryTree;		// pointers to data in a tree hierarchy
	State				mState;
	ProfilerSettings	mSettings;
};

struct ID3D11DeviceContext;
struct ID3D11Device;
class GPUProfiler
{	// src: http://reedbeta.com/blog/gpu-profiling-101/
	// "At this point, you should have all the information you'll need to go and write a GPU profiler of your own!"
	//
	// Here I Go.
public:
	void Init(ID3D11DeviceContext* pContext, ID3D11Device* pDevice);
	void Exit();

	void BeginFrame(const unsigned long long FRAME_NUMBER);
	void EndFrame(const unsigned long long FRAME_NUMBER);

	inline void BeginQuery(const std::string& tag) { mPingPongBuffer.BeginQuery(mpContext, tag); }
	inline void EndQuery() { mPingPongBuffer.EndQuery(mpContext); }

	// collects previous frame's queries
	//
	void CollectTimestamps(const unsigned long long FRAME_NUMBER);

	// todo: lookup?
	float GetEntry() const;


	// renders performance stats tree, starting at @screenPosition using @drawDesc
	//
	void RenderPerformanceStats(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription drawDesc, bool bSortStats);

private:	// Internal Structs
	struct QueryData
	{
		std::string		tag;
		ID3D11Query*	pTimestampQueryBegin = nullptr;
		ID3D11Query*	pTimestampQueryEnd = nullptr;
		float			value = -1.0f;

		void Collect(float freq, ID3D11DeviceContext* pContext);
	};
	struct QueryPingPong
	{
		friend class GPUProfiler;

		static const size_t PER_FRAME_QUERY_BUFFER_SIZE = 10;

		using QueryBuffer = std::array<QueryData, PER_FRAME_QUERY_BUFFER_SIZE>;
		
		void Init(ID3D11Device* pContext);
		void Exit();

		bool GetQueryResultsOfFrame(const unsigned long long frameNumber, ID3D11DeviceContext* pContext, D3D10_QUERY_DATA_TIMESTAMP_DISJOINT & tsDj);

		void ResetFrameQueries(unsigned long long frameNum);

		void BeginQuery(ID3D11DeviceContext* pContext, const std::string & tag);
		void EndQuery(ID3D11DeviceContext* pContext);

	private:
		size_t				currQueryIndex = 0;
		unsigned long long	currFrameNumber = 0;

		ID3D11Query*	pDisjointQuery[2] = { nullptr, nullptr };	// per frame
		QueryBuffer		perFrameQueries[2];
	};

private:
	ID3D11DeviceContext *	mpContext = nullptr;
	ID3D11Device*			mpDevice = nullptr;
	
	QueryPingPong			mPingPongBuffer;
};