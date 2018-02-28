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
	mPerfEntryHierarchy = PerfEntryHierarchy();
}



void Profiler::BeginEntry(const std::string & entryName)
{
	if (!mState.bIsProfiling)
	{
		Log::Error("Profiler::BeginProfile() hasn't been called.");
		return;
	}
		
	const bool bEntryExists = mPerfEntries.find(entryName) != mPerfEntries.end();
	
	// hierarchy variables
	const PerfEntry* pParentPerfEntry = mState.EntryNameStack.empty() ? nullptr : &mPerfEntries.at(mState.EntryNameStack.top());

	// update state
	mState.EntryNameStack.push(entryName);

	// setup the entry settings/data and update hierarchy if entry didn't exist
	PerfEntry& entry = mPerfEntries[entryName];
	const PerfEntry* pCurrentPerfEntry = &mPerfEntries.at(entryName);
	if (!bEntryExists)
	{
		// setup entry settings / aux data
		entry.samples.resize(mSettings.sampleCount, 0.0f);
		entry.name = entryName;

		// update hierarchy
		if (mPerfEntryHierarchy.root.pPerfEntry == nullptr)
		{	// first node
			mPerfEntryHierarchy.root.pPerfEntry = pCurrentPerfEntry;
			mState.pLastEntryNode = &mPerfEntryHierarchy.root;
		}
		else
		{	// rest of the nodes
			mState.pLastEntryNode = mPerfEntryHierarchy.AddChild(*mState.pLastEntryNode, pCurrentPerfEntry);
		}
	}

	// start the timer of the entry 
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
	
	mState.pLastEntryNode = mState.pLastEntryNode->pParent;
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

void Profiler::RenderPerformanceStats(TextRenderer* pTextRenderer, const vec2& hierarchyScreenPosition, TextDrawDescription drawDesc)
{
	std::ostringstream stats;
	

	drawDesc.screenPosition = hierarchyScreenPosition;
	stats.precision(2);
	stats << std::fixed;

	//for (const PerfEntryNode& node : mPerfEntryHierarchy.root.children)
	//{
	//
	//}
}
//---------------------------------------------------------------------------------------------------------------------------



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
//---------------------------------------------------------------------------------------------------------------------------



//---------------------------------------------------------------------------------------------------------------------------
// PERF ENTRY HIERARCHY
//---------------------------------------------------------------------------------------------------------------------------
Profiler::PerfEntryNode* Profiler::PerfEntryHierarchy::AddChild(PerfEntryNode& parentNode, const PerfEntry * pPerfEntry)
{
	PerfEntryNode newNode = { pPerfEntry, &parentNode, std::vector<PerfEntryNode>() };
	parentNode.children.push_back(newNode);
	return &parentNode.children.back();	// is this safe? this might not be safe...
}



Profiler::PerfEntryNode* Profiler::PerfEntryHierarchy::FindNode(const PerfEntry * pNode)
{
	if (root.pPerfEntry == pNode) return &root;
	return SearchSubTree(root, pNode);
}

Profiler::PerfEntryNode* Profiler::PerfEntryHierarchy::SearchSubTree(const PerfEntryNode & node, const PerfEntry * pSearchEntry)
{
	PerfEntryNode* pSearchResult = nullptr;

	for (size_t i = 0; i < node.children.size(); i++)
	{
		PerfEntryNode* pEntryNode = &root.children[i];

		if (pSearchEntry == pEntryNode->pPerfEntry)
			return pEntryNode;

		PerfEntryNode* pResult = SearchSubTree(*pEntryNode, pSearchEntry);
		if (pResult && pResult->pPerfEntry == pSearchEntry)
			return pResult;
	}

	return pSearchResult;
}
//---------------------------------------------------------------------------------------------------------------------------
