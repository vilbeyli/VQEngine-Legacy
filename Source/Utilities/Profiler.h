#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <stack>

#include "PerfTimer.h"
#include "Renderer/TextRenderer.h"


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
	size_t				currSampleIndex;
	std::vector<float>	samples;
	PerfTimer			timer;

public:
	void UpdateSampleBegin();
	void UpdateSampleEnd();
	void PrintEntryInfo(bool bPrintAllEntries = false);
	inline float GetAvg() const;
};

class Profiler
{
// Tree Hierarchy for PerfEntries & State structs
//==================================================================================================================
	using PerfEntryTable = std::unordered_map<std::string, PerfEntry>;

private:
	struct PerfEntryNode
	{
		// PerfEntryNode()
		const PerfEntry /*const*/*		pPerfEntry;
		PerfEntryNode   /*const*/*		pParent;
		std::vector<PerfEntryNode>	children;
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
	Profiler(ProfilerSettings settings = ProfilerSettings()) : 	mSettings(settings) {}

	// Resets PerfEntryTable. BeginEntry()/EndEntry() must be called between BeginProfile() and EndProfile()
	void BeginProfile();
	void EndProfile();

	// Adds PerfEntry to the PerfEntryTable if it doesn't already exist AND Starts the time sampling for the PerfEntry.
	// BeginEntry()/EndEntry() must be called between BeginProfile() and EndProfile()
	// *Assumes PerfEntry has a unique name. There won't be duplicate entries.*
	void BeginEntry(const std::string& entryName);
	void EndEntry();

	inline bool AreThereAnyOpenEntries() const { return !mState.EntryNameStack.empty(); }
	inline float GetEntryAvg(const std::string& entryName) const { return mPerfEntries.at(entryName).GetAvg(); }


	// performs checks for state consistency (are there any open entries? etc.)
	bool StateCheck() const;

	// prints states into debug console output
	void PrintStats() const;

	// renders performance stats tree, starting at @screenPosition using @drawDesc
	void RenderPerformanceStats(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription drawDesc, bool bSortStats);
	
private:
	PerfEntryTable		mPerfEntries;		// where data lives (with entry name as unique keys)
	PerfEntryTree		mPerfEntryTree;		// pointers to data in a tree hierarchy
	State				mState;
	ProfilerSettings	mSettings;
};