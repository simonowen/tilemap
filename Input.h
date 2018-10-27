#pragma once

using KeyMatrix = std::array<uint8_t, 8>;

void Input_Init(HWND hwnd, HINSTANCE hinst, KeyMatrix &matrix);
void Input_Exit();
void Input_ReleaseKeys(KeyMatrix &matrix);
void Input_ReadKeyboard(KeyMatrix &matrix);
