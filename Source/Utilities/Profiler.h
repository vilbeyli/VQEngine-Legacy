#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <stack>

#include "PerfTimer.h"

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

using PerfEntryTable = std::unordered_map<std::string, PerfEntry>;


class Profiler
{
public:
	Profiler(ProfilerSettings settings = ProfilerSettings()) : 	mSettings(settings) {}

	// Resets PerfEntryTable. BeginEntry()/EndEntry() must be called between BeginProfile() and EndProfile()
	void BeginProfile();
	void EndProfile();

	// Adds PerfEntry to the PerfEntryTable if it doesn't already exist AND Starts the time sampling for the PerfEntry.
	// Assumes PerfEntry has a unique name. There won't be duplicate entries.
	// BeginEntry()/EndEntry() must be called between BeginProfile() and EndProfile()
	void BeginEntry(const std::string& entryName);
	void EndEntry();

	// performs checks for state consistency (are there any open entries? etc.)
	bool StateCheck() const;
	inline bool AreThereAnyOpenEntries() const { return !mState.EntryNameStack.empty(); }


	inline float GetEntryAvg(const std::string& entryName) const { return mPerfEntries.at(entryName).GetAvg(); }
	void PrintStats() const;

private:
	struct PerfEntryNode 
	{
		std::string name;

		std::vector<PerfEntryNode> children;
	};
	struct PerfEntryHierarchy
	{
		PerfEntryNode root = { "root", std::vector<PerfEntryNode>() };
	};

	struct State
	{
		bool					bIsProfiling = false;
		std::stack<std::string> EntryNameStack;
	};

	PerfEntryTable		mPerfEntries;
	PerfEntryHierarchy	mPerfEntryHierarchy;
	State				mState;

	ProfilerSettings	mSettings;
};