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

#define KEY_COUNT 256
#define ENABLE_RAW_INPUT

typedef unsigned int KeyCode;

class Input
{
public:
	Input();
	Input(const Input&);
	~Input();

	void Init();

	// update state
	void KeyDown(KeyCode);
	void KeyUp(KeyCode);
	void ButtonDown(KeyCode);
	void ButtonUp(KeyCode);
	void UpdateMousePos(long x, long y);

	// quert state
	bool IsKeyDown(KeyCode) const;
	bool IsMouseDown(KeyCode) const;
	bool IsKeyTriggered(KeyCode) const;
	int  MouseDeltaX() const;
	int  MouseDeltaY() const;

	void Update();
	const long* GetDelta() const;
	bool IsConsumed() const;

private:
	// keyboard
	bool m_keys[KEY_COUNT];
	bool m_prevKeys[KEY_COUNT];

	// mouse
	bool m_buttons[17];
	long m_mouseDelta[2];
	long m_mousePos[2];
	bool m_isConsumed;	// experimental
};

