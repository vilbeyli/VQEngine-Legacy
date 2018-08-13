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


class Profiler
{
public:
	// Defines the scope of a CPU/GPU profiling - should be called once per frame
	//
	virtual void BeginProfile(const unsigned long long FRAME_NUMBER = 0) = 0;
	virtual void EndProfile(const unsigned long long FRAME_NUMBER = 0) = 0;

	// Defines the scope of a CPU/GPU profiling entry
	//
	virtual void BeginEntry(const std::string& tag) = 0;
	virtual void EndEntry() = 0;

	// Gets the averaged value of the root node's perf entry data
	//
	virtual float GetRootEntryAvg() const = 0;

	// Gets the averaged value of the node with @tag
	//
	virtual float GetEntryAvg(const std::string& tag) const = 0;

	// renders performance stats tree, starting at @screenPosition using @drawDesc
	//
	virtual size_t RenderPerformanceStats(
		  TextRenderer*			pTextRenderer
		, const vec2&			screenPosition
		, TextDrawDescription	drawDesc
		, bool					bSortStats
	) = 0;

	// clears the perf entry data
	//
	virtual void Clear() = 0;

	virtual vec2 GetEntryAreaBounds(const vec2& screenSizeInPixels) const = 0;
};

class CPUProfiler : public Profiler
{

public:
	CPUProfiler(ProfilerSettings settings = ProfilerSettings()) : 	mSettings(settings) {}

	// Resets the mPerfEntryTable - must be called at the beginning and end of each frame. 
	//
	void BeginProfile(const unsigned long long FRAME_NUMBER = 0) override;
	void EndProfile(const unsigned long long FRAME_NUMBER = 0) override;

	// Adds PerfEntry to the PerfEntryTable if it doesn't already exist AND Starts the time sampling for the PerfEntry.
	// BeginEntry()/EndEntry() must be called between BeginProfile() and EndProfile()
	// *Assumes PerfEntry has a unique name. There won't be duplicate entries.*
	//
	void BeginEntry(const std::string& entryName) override;
	void EndEntry() override;

	float GetEntryAvg(const std::string& tag) const override;
	float GetRootEntryAvg() const override;

	size_t RenderPerformanceStats(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription drawDesc, bool bSortStats) override;
	void Clear() override;
	vec2 GetEntryAreaBounds(const vec2& screenSizeInPixels) const override;

	// DERIVED INTERFACE -------------------------------------------

	inline bool AreThereAnyOpenEntries() const { return !mState.EntryNameStack.empty(); }

	// performs checks for state consistency (are there any open entries? etc.)
	//
	bool StateCheck() const;

	// prints states into debug console output
	//
	void PrintStats() const;	// todo: impl


	//void Capture()

private:	// Internal Structs
	struct PerfEntry;
	struct State
	{
		bool					bIsProfiling = false;
		bool					bCaptureInProgress = false;
		std::stack<std::string> EntryNameStack;
		TreeNode<PerfEntry>*	pLastEntryNode = nullptr;	// used for setting up the hierarchy
		inline void Clear()
		{
			bIsProfiling = false;
			pLastEntryNode = nullptr;
			while (!EntryNameStack.empty()) EntryNameStack.pop();
		}
	};
	using PerfEntryTable = std::unordered_map<std::string, PerfEntry>;


private:
	PerfEntryTable		mPerfEntries;		// where data lives (with entry name as unique keys)
	Tree<PerfEntry>		mPerfEntryTree;
	
	ProfilerSettings	mSettings;
	State				mState;

	//------------------------------------------------------

	struct PerfEntry
	{
		std::string			tag;
		size_t				currSampleIndex; // [0, samples.size())
		std::vector<float>	samples;
		PerfTimer			timer;

		void UpdateSampleBegin();
		void UpdateSampleEnd();
		void PrintEntryInfo(bool bPrintAllEntries = false);
		inline float GetAvg() const;
		bool operator<(const PerfEntry& other) const;
		bool IsStale() const;
	};
};





class GPUProfiler : public Profiler
{	
	// src: http://reedbeta.com/blog/gpu-profiling-101/
	// "At this point, you should have all the information you'll need to go and write a GPU profiler of your own!"
	//
public:
	// Begins/Ends the disjoint query. This function pair has to be called for each frame.
	//
	void BeginProfile(const unsigned long long FRAME_NUMBER) override;
	void EndProfile(const unsigned long long FRAME_NUMBER) override;

	// Marks the Beginning/Ending of a GPU query with a tag.
	//
	void BeginEntry(const std::string& tag) override;
	void EndEntry() override;

	float GetEntryAvg(const std::string& tag) const override;
	float GetRootEntryAvg() const override;

	size_t RenderPerformanceStats(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription drawDesc, bool bSortStats) override;
	void Clear() override;
	vec2 GetEntryAreaBounds(const vec2& screenSizeInPixels) const override;

	// DERIVED INTERFACE -------------------------------------------

	void Init(ID3D11DeviceContext* pContext, ID3D11Device* pDevice);
	void Exit();

private:
	struct QueryData;
	struct State
	{
		bool					bIsProfiling = false;
		std::stack<std::string> EntryNameStack;
		TreeNode<QueryData>*	pLastEntryNode = nullptr;	// used for setting up the hierarchy
	};

	static const size_t FRAME_HISTORY = 10;
	using FrameQueryTable = std::unordered_map<std::string, QueryData>;

	ID3D11DeviceContext *	mpContext = nullptr;
	ID3D11Device*			mpDevice = nullptr;
	
	ID3D11Query*			pDisjointQuery[FRAME_HISTORY];

	FrameQueryTable			mFrameQueries;

	Tree<QueryData>			mQueryDataTree;
	State					mState;

	//------------------------------------------------------


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

	static unsigned long long		sCurrFrameNumber;
};