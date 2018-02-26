#include "Profiler.h"
#include "Log.h"

#include <numeric>
#include <sstream>
#include <iomanip>


//---------------------------------------------------------------------------------------------------------------------------
// PROFILER
//---------------------------------------------------------------------------------------------------------------------------
void Profiler::BeginProfile()
{
	auto& entryStack = mState.EntryNameStack;

	if (mState.bIsProfiling)
	{
		Log::Warning("Already began profiling! Current profiling entry: %s", entryStack.empty() ? "EMPTY" : entryStack.top());
	}


	mState.bIsProfiling = true;
	mPerfEntries.clear();
}

void Profiler::EndProfile()
{
	auto& entryStack = mState.EntryNameStack;

	if (!mState.bIsProfiling)
	{
		Log::Warning("Haven't started profiling!");
	}

	if (!entryStack.empty())
	{
		Log::Warning("Begin/End Entry mismatch! Last profiling entry: %s", entryStack.top());
		while (!entryStack.empty()) entryStack.pop();
	}


	mState.bIsProfiling = false;
}



void Profiler::BeginEntry(const std::string & entryName)
{
	if (!mState.bIsProfiling)
	{
		Log::Error("Profiler::BeginProfile() hasn't been called.");
		return;
	}
		
	const bool bEntryExists = mPerfEntries.find(entryName) != mPerfEntries.end();

	// todo: setup hierarchy
	std::string parent = mState.EntryNameStack.empty() ? "root" : mState.EntryNameStack.top();
	std::string child = entryName;


	// update state
	mState.EntryNameStack.push(entryName);

	// start the timer of the entry
	PerfEntry& entry = mPerfEntries[entryName];
	if (!bEntryExists)
	{
		entry.samples.resize(mSettings.sampleCount, 0.0f);
		entry.name = entryName;
	}
	entry.UpdateSampleBegin();
}

void Profiler::EndEntry()
{
	if (!mState.bIsProfiling)
	{
		Log::Error("Profiler::BeginProfile() hasn't been called.");
		return;
	}

	std::string entryName = mState.EntryNameStack.top();
	mState.EntryNameStack.pop();

	// stop the timer of the entry
	PerfEntry& entry = mPerfEntries.at(entryName);
	entry.UpdateSampleEnd();
	

}



bool Profiler::StateCheck() const
{
	// ensures entry name stack is empty

	bool bState = !AreThereAnyOpenEntries();
	if (!bState)
	{
		Log::Warning("There are still open profiling entries...");
	}

	return bState;
}


//---------------------------------------------------------------------------------------------------------------------------
// PERF ENTRY
//---------------------------------------------------------------------------------------------------------------------------
void PerfEntry::UpdateSampleEnd()
{
	timer.Stop();
	float dt = timer.DeltaTime();
	timer.Reset();

	samples[currSampleIndex++ % samples.size()] = dt;
}
void PerfEntry::UpdateSampleBegin()
{
	timer.Start();
}

// returns the index (i-1) in a ring-buffer fashion
size_t GetLastIndex(size_t i, size_t count) { return i == 0 ? count - 1 : ((i - 1) % count); }

void PerfEntry::PrintEntryInfo(bool bPrintAllEntries)
{
	size_t lastIndedx = GetLastIndex(currSampleIndex, samples.size());

	std::stringstream info;
	info.precision(5);
	info << std::fixed;
	info << "\nEntry\t\t\t: " + name + "\n";
	info << "Last Sample\t\t: " << samples[lastIndedx] << "ms\n";
	info << "Avg  Sample\t\t: " << GetAvg() << "ms\n";
	if (bPrintAllEntries)
	{
		info << "----------\n";
		for (int i = 0; i < samples.size(); ++i)
			info << "\t" << i << "  - " << samples[i] * 1000.f << "ms\n";
		info << "----------\n";
	}
	Log::Info(info.str());
}
inline float PerfEntry::GetAvg() const
{
	return std::accumulate(samples.begin(), samples.end(), 0.0f) / samples.size();	
}