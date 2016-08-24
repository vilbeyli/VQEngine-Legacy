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

#pragma once

// Borrowed from Frank Luna's DirectX11 book's GameTimer class
// https://www.amazon.com/Introduction-3D-Game-Programming-DirectX/dp/1936420228
class PerfTimer
{
public:
	PerfTimer();
	~PerfTimer();

	float TotalTime() const;
	float DeltaTime() const;

	void Reset();
	void Start();
	void Stop();
	void Tick();	

private:
	double m_secPerCount;
	double m_dt;

	long m_baseTime;
	long m_pausedTime;
	long m_stopTime;
	long m_prevTime;
	long m_currTime;

	bool m_stopped;
};

