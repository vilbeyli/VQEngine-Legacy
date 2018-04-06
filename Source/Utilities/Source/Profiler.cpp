#include "Profiler.h"
#include "Log.h"

#include <numeric>
#include <sstream>
#include <iomanip>


//---------------------------------------------------------------------------------------------------------------------------
// CPU PROFILER
//---------------------------------------------------------------------------------------------------------------------------
void CPUProfiler::BeginProfile()
{
	auto& entryStack = mState.EntryNameStack;

	if (mState.bIsProfiling)
	{
		Log::Warning("Already began profiling! Current profiling entry: %s", entryStack.empty() ? "EMPTY" : entryStack.top());
	}


	mState.bIsProfiling = true;
	mPerfEntries.clear();
}

void CPUProfiler::EndProfile()
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
	mPerfEntryTree = PerfEntryTree();
}



void CPUProfiler::BeginEntry(const std::string & entryName)
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
		if (mPerfEntryTree.root.pPerfEntry == nullptr)
		{	// first node
			mPerfEntryTree.root.pPerfEntry = pCurrentPerfEntry;
			mState.pLastEntryNode = &mPerfEntryTree.root;
		}
		else
		{	// rest of the nodes
			mState.pLastEntryNode = mPerfEntryTree.AddChild(*mState.pLastEntryNode, pCurrentPerfEntry);
		}
	}

	// start the timer of the entry 
	entry.UpdateSampleBegin();
}

void CPUProfiler::EndEntry()
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
	
	// update last entry node with the parent of it
	if(mState.pLastEntryNode) mState.pLastEntryNode = mState.pLastEntryNode->pParent;
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



void CPUProfiler::RenderPerformanceStats(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription drawDesc, bool bSort)
{
	if(bSort) mPerfEntryTree.Sort();
	mPerfEntryTree.RenderTree(pTextRenderer, screenPosition, drawDesc);
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
// PERF ENTRY TREE
//---------------------------------------------------------------------------------------------------------------------------
CPUProfiler::PerfEntryNode* CPUProfiler::PerfEntryTree::AddChild(PerfEntryNode& parentNode, const PerfEntry * pPerfEntry)
{
	PerfEntryNode newNode = { pPerfEntry, &parentNode, std::vector<PerfEntryNode>() };
	parentNode.children.push_back(newNode);
	return &parentNode.children.back();	// is this safe? this might not be safe...
}

void CPUProfiler::PerfEntryTree::Sort()
{
	SortSubTree(root);
}

void CPUProfiler::PerfEntryTree::RenderTree(TextRenderer * pTextRenderer, const vec2 & screenPosition, TextDrawDescription & drawDesc)
{
	// set stream properties once, and pass it to recursive function
	std::ostringstream stats;
	stats.precision(2);
	stats << std::fixed;

	RenderSubTree(root, pTextRenderer, screenPosition, drawDesc, stats);
}

CPUProfiler::PerfEntryNode* CPUProfiler::PerfEntryTree::FindNode(const PerfEntry * pNode)
{
	if (root.pPerfEntry == pNode) return &root;
	return SearchSubTree(root, pNode);
}

CPUProfiler::PerfEntryNode* CPUProfiler::PerfEntryTree::SearchSubTree(const PerfEntryNode & node, const PerfEntry * pSearchEntry)
{
	PerfEntryNode* pSearchResult = nullptr;

	// visit children | depth first
	for (size_t i = 0; i < node.children.size(); i++)
	{
		PerfEntryNode* pEntryNode = &root.children[i];
		if (pSearchEntry == pEntryNode->pPerfEntry)
			return pEntryNode;

		pSearchResult = SearchSubTree(*pEntryNode, pSearchEntry);
		if (pSearchResult && pSearchResult->pPerfEntry == pSearchEntry)
			return pSearchResult;
	}

	return pSearchResult;
}
void CPUProfiler::PerfEntryTree::RenderSubTree(const PerfEntryNode & node, TextRenderer * pTextRenderer, const vec2 & screenPosition, TextDrawDescription & drawDesc, std::ostringstream & stats)
{
	const int X_OFFSET = 20;
	const int Y_OFFSET = 22;

	// clear & populate text
	stats.clear(); stats.str("");
	stats << node.pPerfEntry->name << "  " << node.pPerfEntry->GetAvg() * 1000 << " ms";

	drawDesc.screenPosition = screenPosition;
	drawDesc.text = stats.str();
	pTextRenderer->RenderText(drawDesc);
	
	size_t row_count = 0;
	for (size_t i = 0; i < node.children.size(); i++)
	{
		row_count += i == 0 ? 0 : node.children[i - 1].children.size();
		RenderSubTree(node.children[i], pTextRenderer, { screenPosition.x() + X_OFFSET, screenPosition.y() + Y_OFFSET * (i+1+ row_count)}, drawDesc, stats);
	}
}
void CPUProfiler::PerfEntryTree::SortSubTree(PerfEntryNode & node)
{
	std::sort(node.children.begin(), node.children.end(), [](const PerfEntryNode& l, const PerfEntryNode& r) { return l.pPerfEntry->GetAvg() >= r.pPerfEntry->GetAvg(); });
	std::for_each(node.children.begin(), node.children.end(), [this](PerfEntryNode& n) { SortSubTree(n); });
}

//---------------------------------------------------------------------------------------------------------------------------

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
void GPUProfiler::Init(ID3D11DeviceContext* pContext, ID3D11Device* pDevice)
{
	mpContext = pContext;
	mpDevice = pDevice;

	mPingPongBuffer.Init(pDevice);
}

void GPUProfiler::Exit()
{
	mPingPongBuffer.Exit();
}

void GPUProfiler::BeginFrame(const unsigned long long FRAME_NUMBER)
{
	mPingPongBuffer.ResetFrameQueries(FRAME_NUMBER);
	mpContext->Begin(mPingPongBuffer.pDisjointQuery[FRAME_NUMBER % 2]);
}

void GPUProfiler::EndFrame(const unsigned long long FRAME_NUMBER)
{
	mpContext->End(mPingPongBuffer.pDisjointQuery[FRAME_NUMBER % 2]);
}

float GPUProfiler::GetEntry() const
{
	float time = 0.0f;

	return time;
}

void GPUProfiler::RenderPerformanceStats(TextRenderer * pTextRenderer, const vec2 & screenPosition, TextDrawDescription drawDesc, bool bSort)
{
	// cpu code for reference
	//
	//if (bSort) 
	//	mPerfEntryTree.Sort();
	//mPerfEntryTree.RenderTree(pTextRenderer, screenPosition, drawDesc);

	const int X_OFFSET = 20;
	const int Y_OFFSET = 22;

	std::ostringstream stats;
	stats.precision(2);
	stats << std::fixed;

	auto pingPongIndex = ((mPingPongBuffer.currFrameNumber - 1) % 2);
	const QueryData& qData = mPingPongBuffer.perFrameQueries[pingPongIndex].front();
	stats << qData.tag << "  " << qData.value << " ms";

	drawDesc.screenPosition = screenPosition;
	drawDesc.text = stats.str();
	pTextRenderer->RenderText(drawDesc);
}

void GPUProfiler::CollectTimestamps(const unsigned long long FRAME_NUMBER)
{
	if (FRAME_NUMBER == 0) return;	// skip first frame.

	const unsigned long long PREV_FRAME_NUMBER = (FRAME_NUMBER - 1);

	D3D10_QUERY_DATA_TIMESTAMP_DISJOINT tsDj = {};
	if ( !mPingPongBuffer.GetQueryResultsOfFrame(PREV_FRAME_NUMBER, mpContext, tsDj) )
	{
		Log::Warning("[GPUProfiler]: Bad Disjoint Query (Frame: %llu)", PREV_FRAME_NUMBER);
	}
}

void GPUProfiler::QueryData::Collect(float freq, ID3D11DeviceContext* pContext)
{
	UINT64 tsBegin, tsEnd;

#if 0
	bool bDataRetrieved = true;
	int attempt = 0;
	do 
	{
		bDataRetrieved &= FAILED(pContext->GetData(pTimestampQueryBegin, &tsBegin, sizeof(UINT64), 0));
		bDataRetrieved &= FAILED(pContext->GetData(pTimestampQueryEnd, &tsEnd, sizeof(UINT64), 0));
		Sleep(1);
	} while(!bDataRetrieved && (++attempt < 5));
#else
	pContext->GetData(pTimestampQueryBegin, &tsBegin, sizeof(UINT64), 0);
	pContext->GetData(pTimestampQueryEnd, &tsEnd, sizeof(UINT64), 0);
#endif

	value = float(tsEnd - tsBegin) / freq * 1000.0f;
}

bool GPUProfiler::QueryPingPong::GetQueryResultsOfFrame(const unsigned long long frameNumber, ID3D11DeviceContext* pContext, D3D10_QUERY_DATA_TIMESTAMP_DISJOINT & tsDj)
{
	const size_t bufferIndex = frameNumber % 2;

	pContext->GetData(pDisjointQuery[bufferIndex], &tsDj, sizeof(tsDj), 0);
	if (tsDj.Disjoint != 0)
	{
		return false;
	}
	for (size_t i = 0; i < currQueryIndex; ++i)
	{
		perFrameQueries[bufferIndex][i].Collect(static_cast<float>(tsDj.Frequency), pContext);
	}
	return true;
}

void GPUProfiler::QueryPingPong::ResetFrameQueries(unsigned long long frameNum)
{
	currFrameNumber = frameNum;
	currQueryIndex = 0;
}

void GPUProfiler::QueryPingPong::BeginQuery(ID3D11DeviceContext* pContext, const std::string & tag)
{
	QueryData& query = perFrameQueries[currFrameNumber % 2][currQueryIndex];
	pContext->End(query.pTimestampQueryBegin);
	query.tag = tag;
}

void GPUProfiler::QueryPingPong::EndQuery(ID3D11DeviceContext* pContext)
{
	pContext->End(perFrameQueries[currFrameNumber % 2][currQueryIndex].pTimestampQueryEnd);
	++currQueryIndex;
}

void GPUProfiler::QueryPingPong::Init(ID3D11Device * pDevice)
{
	D3D11_QUERY_DESC queryDesc = {};
	queryDesc.MiscFlags = 0;

	queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	pDevice->CreateQuery(&queryDesc, &pDisjointQuery[0]);
	pDevice->CreateQuery(&queryDesc, &pDisjointQuery[1]);

	queryDesc.Query = D3D11_QUERY_TIMESTAMP;
	for (size_t pingPong = 0; pingPong < 2; ++pingPong)
	{
		for (size_t i = 0; i < PER_FRAME_QUERY_BUFFER_SIZE; ++i)
		{
			pDevice->CreateQuery(&queryDesc, &perFrameQueries[pingPong][i].pTimestampQueryBegin);
			pDevice->CreateQuery(&queryDesc, &perFrameQueries[pingPong][i].pTimestampQueryEnd);
		}
	}
}

void GPUProfiler::QueryPingPong::Exit()
{
	for (size_t pingPong = 0; pingPong < 2; ++pingPong)
	{
		for (size_t i = 0; i < PER_FRAME_QUERY_BUFFER_SIZE; ++i)
		{
			perFrameQueries[pingPong][i].pTimestampQueryBegin->Release();
			perFrameQueries[pingPong][i].pTimestampQueryEnd->Release();
		}
		pDisjointQuery[pingPong]->Release();
	}
}
