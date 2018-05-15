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
#include "Engine/DataStructures.h"

#ifdef max
#undef max
#endif

class TextRenderer;
struct vec2;
struct ID3D11DeviceContext;
struct ID3D11Device;

struct ProfilerSettings
{	// UNUSED
	int sampleCount = 50;
	int refreshRate = 5;
};

class CPUProfiler
{

public:
	CPUProfiler(ProfilerSettings settings = ProfilerSettings()) : 	mSettings(settings) {}

	// Resets PerfEntryTable. BeginEntry()/EndEntry() must be called between BeginProfile() and EndProfile()
	//
	void BeginProfile();
	void EndProfile();

	// Adds PerfEntry to the PerfEntryTable if it doesn't already exist AND Starts the time sampling for the PerfEntry.
	// BeginEntry()/EndEntry() must be called between BeginProfile() and EndProfile()
	// *Assumes PerfEntry has a unique name. There won't be duplicate entries.*
	//
	void BeginEntry(const std::string& entryName);
	void EndEntry();

	// performs checks for state consistency (are there any open entries? etc.)
	//
	bool StateCheck() const;

	// prints states into debug console output
	//
	void PrintStats() const;	// todo: impl

	// renders performance stats tree, starting at @screenPosition using @drawDesc
	//
	void RenderPerformanceStats(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription drawDesc, bool bSortStats);


	inline bool AreThereAnyOpenEntries() const { return !mState.EntryNameStack.empty(); }
	inline float GetEntryAvg(const std::string& entryName) const { return mPerfEntries.at(entryName).GetAvg(); }

private:	// Internal Structs
	struct PerfEntry
	{
		std::string			tag;
		size_t				currSampleIndex; // [0, samples.size())
		std::vector<float>	samples;
		PerfTimer			timer;

	public:
		void UpdateSampleBegin();
		void UpdateSampleEnd();
		void PrintEntryInfo(bool bPrintAllEntries = false);
		inline float GetAvg() const;
		bool operator<(const PerfEntry& other) const;
		bool IsStale() const { return false; } 
	};
	using PerfEntryTable = std::unordered_map<std::string, PerfEntry>;


private:
	PerfEntryTable		mPerfEntries;		// where data lives (with entry name as unique keys)
	Tree<PerfEntry>		mPerfEntryTree;
	
	ProfilerSettings	mSettings;
	struct State
	{
		bool					bIsProfiling = false;
		std::stack<std::string> EntryNameStack;
		TreeNode<PerfEntry>*	pLastEntryNode = nullptr;	// used for setting up the hierarchy
	} mState;
};


//===============================================================================================================================================

class GPUProfiler
{	// src: http://reedbeta.com/blog/gpu-profiling-101/
	// "At this point, you should have all the information you'll need to go and write a GPU profiler of your own!"
	//
	// Me: Alright, here I go.
public:
	void Init(ID3D11DeviceContext* pContext, ID3D11Device* pDevice);
	void Exit();

	// Begins/Ends the disjoint query. This function pair has to be called for each frame.
	//
	void BeginFrame(const unsigned long long FRAME_NUMBER);
	void EndFrame(const unsigned long long FRAME_NUMBER);

	// Marks the Beginning/Ending of a GPU query with a tag.
	//
	void BeginQuery(const std::string& tag);
	void EndQuery();

	// Gets the entry with @tag
	//
	float GetEntry(const std::string& tag) const;

	// renders performance stats tree, starting at @screenPosition using @drawDesc
	//
	void RenderPerformanceStats(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription drawDesc, bool bSortStats);

private:	// Internal Structs
	static const size_t FRAME_HISTORY = 10;

	struct QueryData
	{
		std::string		tag;
		ID3D11Query*	pTimestampQueryBegin[FRAME_HISTORY];
		ID3D11Query*	pTimestampQueryEnd[FRAME_HISTORY];
		float			value[FRAME_HISTORY];
		long long		lastQueryFrameNumber;

		QueryData();

		float GetAvg() const;
		void Collect(float freq, ID3D11DeviceContext* pContext, size_t bufferIndex);
		bool operator<(const QueryData& other) const; 
		bool IsStale() const { return GPUProfiler::sCurrFrameNumber - lastQueryFrameNumber > FRAME_HISTORY; }
	};
	bool GetQueryResultsOfFrame(const unsigned long long frameNumber, ID3D11DeviceContext* pContext, D3D10_QUERY_DATA_TIMESTAMP_DISJOINT & tsDj);

	using FrameQueryTable = std::unordered_map<std::string, QueryData>;

	static unsigned long long		sCurrFrameNumber;
private:
	ID3D11DeviceContext *	mpContext = nullptr;
	ID3D11Device*			mpDevice = nullptr;
	
	ID3D11Query*			pDisjointQuery[FRAME_HISTORY];

	FrameQueryTable			mFrameQueries;

	Tree<QueryData>			mQueryDataTree;
	struct State
	{
		bool					bIsProfiling = false;
		std::stack<std::string> EntryNameStack;
		TreeNode<QueryData>*	pLastEntryNode = nullptr;	// used for setting up the hierarchy
	} mState;
};