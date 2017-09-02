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

#include "utils.h"

template<typename T>
class Track
{
public:
	Track() = delete;
	Track(T* data, const T& beginVal, const T& endVal, float period)
		:
		_totalTime(0.0f),
		_period(period),
		_data(data),
		_valueBegin(beginVal),
		_valueEnd(endVal)
	{}

	void Update(float dt);

private:
	float _totalTime;
	float _period;

	T* _data;
	T _valueBegin;
	T _valueEnd;
};

struct Animation
{
	// lerping tracks
	std::vector<Track<float>> _fTracks;
	std::vector<Track<vec3>>  _vTracks;

	void Update(float dt)
	{
		for (auto& t : _fTracks) t.Update(dt);
		for (auto& t : _vTracks) t.Update(dt);
	}
};
