#pragma once

bool Audio_Init(HWND hWnd);
void Audio_Exit();
void SilenceAudio();
bool Audio_AddData(BYTE *pbData_, int nLength_);
void SoundOut(uint8_t val, int cycles);
bool Audio_EndFrame(uint64_t frame_end_cycles);
