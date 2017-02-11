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

#include "PerfTimer.h"
#include <Windows.h>

#include <string>

PerfTimer::PerfTimer()
	:
	m_secPerCount(0),
	m_dt(-1),
	m_baseTime(0),
	m_pausedTime(0),
	m_prevTime(0),
	m_currTime(0),
	m_stopped(false)
{
	__int64 countsPerSec;	// use this data type for querying - otherwise stack corruption!
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	m_secPerCount = 1.0 / (double)(countsPerSec);
}


PerfTimer::~PerfTimer()
{}


float PerfTimer::DeltaTime() const
{
	return static_cast<float>(m_dt);
}

void PerfTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	m_baseTime = currTime;
	m_prevTime = currTime;
	m_stopTime = 0;
	m_stopped = false;
}

// accumulates paused time & starts the timer
void PerfTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (m_stopped)
	{
		m_pausedTime = (startTime - m_stopTime);
		m_prevTime = startTime;
		m_stopTime = 0;
		m_stopped = false;
	}
}

// stops the timer
void PerfTimer::Stop()
{
	if (!m_stopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		m_stopTime = currTime;
		m_stopped = true;
	}
}

// returns time elapsed since Reset(), not counting paused time in between
float PerfTimer::TotalTime() const
{
	long timeLength = 0;
	if (m_stopped)
	{
		//					TIME
		// Base	  Stop		Start	 Stop	Curr
		//--*-------*----------*------*------|
		//			<---------->
		//			   Paused
		timeLength = (m_stopTime - m_pausedTime) - m_baseTime;
	}
	else
	{
		//					TIME
		// Base			Stop	  Start			Curr
		//--*------------*----------*------------|
		//				 <---------->
		//					Paused
		timeLength = (m_currTime - m_pausedTime) - m_baseTime;		
	}

	return static_cast<float>(timeLength * m_secPerCount);
}

double PerfTimer::CurrentTime()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_currTime = currTime;
	return m_currTime * m_secPerCount;
}

void PerfTimer::Tick()
{
	if (m_stopped)
	{
		m_dt = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_currTime = currTime;
	m_dt = (m_currTime - m_prevTime) * m_secPerCount;

	m_prevTime = m_currTime;
	m_dt = m_dt < 0.0 ? 0.0 : m_dt;		// m_dt => 0
}
