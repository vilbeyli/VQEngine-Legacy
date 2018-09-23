#include "Profiler.h"
#include "Log.h"

#include <numeric>
#include <sstream>
#include <iomanip>

#define DISABLE_CPU_PROFILER 0
#define DISABLE_GPU_PROFILER 0

#if DISABLE_CPU_PROFILER
#define CPU_PROFILER_ENABLE_CHECK return;
#else
#define CPU_PROFILER_ENABLE_CHECK
#endif

#if DISABLE_GPU_PROFILER
#define GPU_PROFILER_ENABLE_CHECK return;
#else
#define GPU_PROFILER_ENABLE_CHECK
#endif

//---------------------------------------------------------------------------------------------------------------------------
// CPU PROFILER
//---------------------------------------------------------------------------------------------------------------------------
void CPUProfiler::BeginProfile(const unsigned long long FRAME_NUMBER)
{
	CPU_PROFILER_ENABLE_CHECK
	auto& entryStack = mState.EntryNameStack;

	if (mState.bIsProfiling)
	{
		Log::Warning("Already began profiling! Current profiling entry: %s", entryStack.empty() ? "EMPTY" : entryStack.top().c_str());
	}


	mState.bIsProfiling = true;
	mPerfEntries.clear();
}

void CPUProfiler::EndProfile(const unsigned long long FRAME_NUMBER)
{
	CPU_PROFILER_ENABLE_CHECK
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
	mPerfEntryTree.Clear();
}



void CPUProfiler::BeginEntry(const std::string & entryName)
{
	CPU_PROFILER_ENABLE_CHECK
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
		entry.tag = entryName;

		// update hierarchy
		if (mPerfEntryTree.root.pData == nullptr)
		{	// first node
			mPerfEntryTree.root.pData = pCurrentPerfEntry;
			mState.pLastEntryNode = &mPerfEntryTree.root;
			assert(mState.pLastEntryNode);
		}
		else
		{	// rest of the nodes
			assert(mState.pLastEntryNode);
			mState.pLastEntryNode = mPerfEntryTree.AddChild(*mState.pLastEntryNode, pCurrentPerfEntry);
		}
	}

	// start the timer of the entry 
	entry.UpdateSampleBegin();
}

void CPUProfiler::EndEntry()
{
	CPU_PROFILER_ENABLE_CHECK
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
	
	// update last entry node with the parent of it
	if(mState.pLastEntryNode && mState.pLastEntryNode->pParent) mState.pLastEntryNode = mState.pLastEntryNode->pParent;
}



bool CPUProfiler::StateCheck() const
{
	// ensures entry name stack is empty
	bool bState = !AreThereAnyOpenEntries();
	if (!bState)
	{
		Log::Warning("There are still open profiling entries...");
	}

	return bState;
}


float CPUProfiler::GetEntryAvg(const std::string & entryName) const
{
	if (mPerfEntries.find(entryName) == mPerfEntries.end())
		return -1.0f;
	return mPerfEntries.at(entryName).GetAvg();
}

float CPUProfiler::GetRootEntryAvg() const
{
	if (mPerfEntries.empty() || mPerfEntryTree.root.pData == nullptr)
		return -1.0f;
	return mPerfEntryTree.root.pData->GetAvg();
}

void CPUProfiler::Clear()
{
	mPerfEntries.clear();
	mPerfEntryTree.Clear();
	bool bIsProfiling = mState.bIsProfiling;
	mState.Clear();
	mState.bIsProfiling = bIsProfiling;
}


//---------------------------------------------------------------------------------------------------------------------------



//---------------------------------------------------------------------------------------------------------------------------
// PERF ENTRY
//---------------------------------------------------------------------------------------------------------------------------
void CPUProfiler::PerfEntry::UpdateSampleEnd()
{
	timer.Stop();
	float dt = timer.DeltaTime();

	samples[currSampleIndex++ % samples.size()] = dt;
}
void CPUProfiler::PerfEntry::UpdateSampleBegin()
{
	timer.Reset();
	timer.Start();
}

// returns the index (i-1) in a ring-buffer fashion
size_t GetLastIndex(size_t i, size_t count) { return i == 0 ? count - 1 : ((i - 1) % count); }

void CPUProfiler::PerfEntry::PrintEntryInfo(bool bPrintAllEntries)
{
	size_t lastIndedx = GetLastIndex(currSampleIndex, samples.size());

	std::stringstream info;
	info.precision(5);
	info << std::fixed;
	info << "\nEntry\t\t\t: " + tag + "\n";
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
inline float CPUProfiler::PerfEntry::GetAvg() const
{
	return std::accumulate(samples.begin(), samples.end(), 0.0f) / samples.size();	
}
bool CPUProfiler::PerfEntry::operator<(const PerfEntry & other) const
{
	return GetAvg() > other.GetAvg() + 0.000001f;	// largest time measurement appears first
}

bool CPUProfiler::PerfEntry::IsStale() const
{
	return timer.GetStopDuration() > 5.0f;	// 5 second upper limit
}

#if 0
// todo: multi-threading this is not a good idea. use the worker thread later on as watcher on shader files for shader hotswap?
static bool bWantsExit = false;
void GPUProfilerWatcherThread(ID3D11DeviceContext* pContext, GPUProfiler::QueryData* pQueryData)
{
	while (!bWantsExit)
	{
		// Log::Info("[WatcherThread] Sleeping for 2 seconds...");
		// std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}
#endif

//---------------------------------------------------------------------------------------------------------------------------
// GPU PROFILER
//---------------------------------------------------------------------------------------------------------------------------
unsigned long long GPUProfiler::sCurrFrameNumber = 0;

void GPUProfiler::Init(ID3D11DeviceContext* pContext, ID3D11Device* pDevice)
{
	GPU_PROFILER_ENABLE_CHECK
	mpContext = pContext;
	mpDevice = pDevice;

	D3D11_QUERY_DESC queryDesc = {};
	queryDesc.MiscFlags = 0;

	for (size_t bufferIndex = 0; bufferIndex < FRAME_HISTORY; ++bufferIndex)
	{
		queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		pDevice->CreateQuery(&queryDesc, &pDisjointQuery[bufferIndex]);
	}
}

void GPUProfiler::Exit()
{
	GPU_PROFILER_ENABLE_CHECK
	for (auto it = mFrameQueries.begin(); it != mFrameQueries.end(); ++it)
	{
		for (size_t bufferIndex = 0; bufferIndex < FRAME_HISTORY; ++bufferIndex)
		{
			it->second.pTimestampQueryBegin[bufferIndex]->Release();
			it->second.pTimestampQueryEnd[bufferIndex]->Release();
		}
	}
	for (size_t bufferIndex = 0; bufferIndex < FRAME_HISTORY; ++bufferIndex)
		pDisjointQuery[bufferIndex]->Release();
}

void GPUProfiler::BeginProfile(const unsigned long long FRAME_NUMBER)
{
	GPU_PROFILER_ENABLE_CHECK
	sCurrFrameNumber = FRAME_NUMBER;
	mpContext->Begin(pDisjointQuery[FRAME_NUMBER % FRAME_HISTORY]);
}

void GPUProfiler::EndProfile(const unsigned long long FRAME_NUMBER)
{
	GPU_PROFILER_ENABLE_CHECK
	const unsigned long long PREV_FRAME_NUMBER = (FRAME_NUMBER - (FRAME_HISTORY-1));

	mpContext->End(pDisjointQuery[FRAME_NUMBER % FRAME_HISTORY]);

	// collect previous frame queries
	if (FRAME_NUMBER < FRAME_HISTORY-1) return;	// skip first queries.
	D3D10_QUERY_DATA_TIMESTAMP_DISJOINT tsDj = {};
	if (!GetQueryResultsOfFrame(PREV_FRAME_NUMBER, mpContext, tsDj))
	{
		Log::Warning("[GPUProfiler]: Bad Disjoint Query (Frame: %llu)", PREV_FRAME_NUMBER);
	}

	// Note:  Instead of pruning, skipping the 
	//		  rendering of stale data is the way to go for now
	//		  due to GPU data persisting for multiple frames.
	//		  We cant prune in the same frame - releasing resources
	//		  is also a problem. More state tracking is needed for
	//		  proper resource release. 
	//		  Instead: we'll skip rendering stale data in RenderTree().
#if 0
	this->mQueryDataTree.Prune([](const TreeNode<QueryData>& n)
	{
		return n.pData->IsStale();
	});
#endif
}

float GPUProfiler::GetEntryAvg(const std::string& tag) const
{	
	auto* pNode = mQueryDataTree.FindNode(tag);
	if (pNode == nullptr)
		return -1.0f;
	return pNode->pData->GetAvg();
}

float GPUProfiler::GetRootEntryAvg() const
{
	if (mFrameQueries.empty() || mQueryDataTree.root.pData == nullptr)
		return -1.0f;
	return mQueryDataTree.root.pData->GetAvg();
}

void GPUProfiler::Clear()
{
	//mQueryDataTree.Clear(); // nope, don't do this.
	//
	// Query Data is meaningful in a several frame span so we cannot
	// clear and continue execution. GPUProfiler entries have IsStale() 
	// member function to determine stales nodes and prune accordingly.
	//
	return;
}


void GPUProfiler::QueryData::Collect(float freq, ID3D11DeviceContext * pContext, size_t bufferIndex)
{
	GPU_PROFILER_ENABLE_CHECK
	UINT64 tsBegin, tsEnd;

	pContext->GetData(pTimestampQueryBegin[bufferIndex], &tsBegin, sizeof(UINT64), 0);
	pContext->GetData(pTimestampQueryEnd[bufferIndex], &tsEnd, sizeof(UINT64), 0);

	value[bufferIndex] = float(tsEnd - tsBegin) / freq;
}

bool GPUProfiler::GetQueryResultsOfFrame(const unsigned long long frameNumber, ID3D11DeviceContext* pContext, D3D10_QUERY_DATA_TIMESTAMP_DISJOINT & tsDj)
{
	const size_t bufferIndex = frameNumber % FRAME_HISTORY;

	pContext->GetData(pDisjointQuery[bufferIndex], &tsDj, sizeof(tsDj), 0);
	if (tsDj.Disjoint != 0)
	{
		return false;
	}

	std::for_each(mFrameQueries.begin(), mFrameQueries.end(), [&tsDj, &pContext, &bufferIndex](std::pair<const std::string, QueryData>& mapEntry)
	{
		mapEntry.second.Collect(static_cast<float>(tsDj.Frequency), pContext, bufferIndex);
	});
	return true;
}

void GPUProfiler::BeginEntry(const std::string & tag)
{
	GPU_PROFILER_ENABLE_CHECK
	const size_t frameQueryIndex = sCurrFrameNumber % FRAME_HISTORY;

	const bool bEntryExists = mFrameQueries.find(tag) != mFrameQueries.end();

	// hierarchy variables
	const QueryData* pParentPerfEntry = mState.EntryNameStack.empty() ? nullptr : &mFrameQueries.at(mState.EntryNameStack.top());

	// update state
	mState.EntryNameStack.push(tag);

	// setup the entry settings/data and update hierarchy if entry didn't exist
	QueryData& entry = mFrameQueries[tag];
	const QueryData* pCurrentPerfEntry = &mFrameQueries.at(tag);
	if (!bEntryExists)
	{
		// setup entry settings / aux data
		entry.tag = tag;

		// update hierarchy
		if (mQueryDataTree.root.pData == nullptr)
		{	// first node
			mQueryDataTree.root.pData = pCurrentPerfEntry;
			mState.pLastEntryNode = &mQueryDataTree.root;
		}
		else
		{	// rest of the nodes
			mState.pLastEntryNode = mQueryDataTree.AddChild(
				mState.pLastEntryNode == nullptr ? *mQueryDataTree.FindNode(pParentPerfEntry) : *mState.pLastEntryNode
				, pCurrentPerfEntry);
		}

		D3D11_QUERY_DESC queryDesc = {};
		queryDesc.Query = D3D11_QUERY_TIMESTAMP;
		for (size_t bufferIndex = 0; bufferIndex < FRAME_HISTORY; ++bufferIndex)
		{
			mpDevice->CreateQuery(&queryDesc, &entry.pTimestampQueryBegin[bufferIndex]);
			mpDevice->CreateQuery(&queryDesc, &entry.pTimestampQueryEnd[bufferIndex]);
		}
	}

	mpContext->End(entry.pTimestampQueryBegin[frameQueryIndex]);
	entry.lastQueryFrameNumber = sCurrFrameNumber;
}

void GPUProfiler::EndEntry()
{
	GPU_PROFILER_ENABLE_CHECK
	const size_t bufferIndex = sCurrFrameNumber % FRAME_HISTORY;

	std::string tag = mState.EntryNameStack.top();
	mState.EntryNameStack.pop();

	mpContext->End(mFrameQueries.at(tag).pTimestampQueryEnd[bufferIndex]);

	if (mState.pLastEntryNode)
		mState.pLastEntryNode = mState.pLastEntryNode->pParent;
}

GPUProfiler::QueryData::QueryData()
{
	for (size_t i = 0; i < FRAME_HISTORY; ++i)
	{
		pTimestampQueryEnd[i] = pTimestampQueryEnd[i] = nullptr;
		value[i] = 0.0f;
	}
}

float GPUProfiler::QueryData::GetAvg() const
{
	return std::accumulate(std::begin(value), std::end(value), 0.0f) / static_cast<float>(FRAME_HISTORY);
}


bool GPUProfiler::QueryData::operator<(const QueryData & other) const
{
	return GetAvg() > other.GetAvg() + 0.000001f; // largest time measurement appears first
}


// --------------------------------------------------------------------------------------------------------------
// RENDERING
// --------------------------------------------------------------------------------------------------------------

size_t GPUProfiler::RenderPerformanceStats(
	  TextRenderer* pTextRenderer
	, const vec2 & screenPosition
	, TextDrawDescription drawDesc
	, bool bSort)
{
	GPU_PROFILER_ENABLE_CHECK
		if (bSort)
			mQueryDataTree.Sort();
	return mQueryDataTree.RenderTree(pTextRenderer, screenPosition, drawDesc);
}

size_t CPUProfiler::RenderPerformanceStats(
	  TextRenderer* pTextRenderer
	, const vec2& screenPosition
	, TextDrawDescription drawDesc
	, bool bSortStats)
{
	CPU_PROFILER_ENABLE_CHECK

		// TODO: check for inactive queries and 0 them out 
		if (bSortStats)
		{
			mPerfEntryTree.Sort();
		}

	return mPerfEntryTree.RenderTree(pTextRenderer, screenPosition, drawDesc);
}

#include "Utilities/utils.h"


// todo: rename/remove the magic numbers.
//
// X axis:
//  - one way would be calculating how long a row would be 
//    while or before/after rendering the row/text
//    instead of using the 0.01f scale.
// 
// Y axis is covered and scales with the number of rows reasonably well.
//
constexpr float HARD_CODED_X_SCALE = 0.008f;	// this is supposed to be temporary.

template<class PerfEntry_t>
float GetLongestEntryLength(const std::vector<const TreeNode<PerfEntry_t>*>& nodes)
{
	size_t longest = 0;
	std::for_each(RANGE(nodes), [&](const TreeNode<PerfEntry_t>* const  pNode)
	{
		longest = std::max(longest, pNode->pData->tag.size());
	});
	return static_cast<float>(longest);
}

vec2 GPUProfiler::GetEntryAreaBounds(const vec2& screenSizeInPixels) const
{
	const auto nodes = mQueryDataTree.GetNonStaleNodes();
	const float rowCount = static_cast<float>(nodes.size());
	const float longestEntry = GetLongestEntryLength(nodes);

	// todo: rename/remove the magic numbers.
	const vec2 scale = vec2(HARD_CODED_X_SCALE, PERF_TREE_ENTRY_DRAW_Y_OFFSET_PER_LINE / screenSizeInPixels.y());
	return vec2(longestEntry, rowCount + 2) * scale;
}

vec2 CPUProfiler::GetEntryAreaBounds(const vec2& screenSizeInPixels) const
{
	const auto nodes = mPerfEntryTree.GetNonStaleNodes();
	const float rowCount = static_cast<float>(nodes.size());
	const float longestEntry = GetLongestEntryLength(nodes);

	// todo: rename/remove the magic numbers.
	const vec2 scale = vec2(HARD_CODED_X_SCALE, PERF_TREE_ENTRY_DRAW_Y_OFFSET_PER_LINE / screenSizeInPixels.y());
	return vec2(longestEntry, rowCount + 2) * scale;
}