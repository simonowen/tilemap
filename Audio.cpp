#include "stdafx.h"
#include "BlipBuffer.h"

#include <mmsystem.h>
#include <dsound.h>

// Direct sound interface and primary buffer pointers
static IDirectSound *pds;
static IDirectSoundBuffer *pdsb;
static int nSampleBufferSize;
static DWORD dwWriteOffset;

static HANDLE hEvent;
static MMRESULT hTimer;
static DWORD dwTimer;

typedef HRESULT(WINAPI *PFNDIRECTSOUNDCREATE) (LPGUID, LPDIRECTSOUND*, LPUNKNOWN);
static HINSTANCE g_hinstDSound;
static PFNDIRECTSOUNDCREATE pfnDirectSoundCreate;

static bool InitDirectSound(HWND hWnd);
static void ExitDirectSound();
static void CALLBACK TimeCallback(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

static Blip_Buffer g_buf{};
static Blip_Synth<blip_med_quality, 256> g_synth{};
static BYTE* g_pbFrameSample;

////////////////////////////////////////////////////////////////////////////////

void Audio_Exit()
{
	ExitDirectSound();

	if (hTimer) timeKillEvent(hTimer), hTimer = 0;
	if (hEvent) CloseHandle(hEvent), hEvent = nullptr;

	delete[] g_pbFrameSample;
	g_pbFrameSample = nullptr;
}

bool Audio_Init (HWND hWnd)
{
	Audio_Exit();
	InitDirectSound(hWnd);

	// If we've no sound, fall back on a timer for running speed
	if (!pdsb)
	{
		// Create an event to trigger when the next frame is due
		hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	int nSamplesPerFrame = (44100 / 50) + 1;
	int nSize = nSamplesPerFrame * (16 * 2 / 8);
	g_pbFrameSample = new BYTE[nSize];

	g_buf.clock_rate(70000 * 50);
	g_buf.set_sample_rate(44100);

	g_synth.output(&g_buf);
	g_synth.volume(1.0);

	return true;
}

void SilenceAudio()
{
	if (!pdsb)
		return;

	PVOID pvWrite;
	DWORD dwLength;
	if (SUCCEEDED(pdsb->Lock(0, 0, &pvWrite, &dwLength, nullptr, nullptr, DSBLOCK_ENTIREBUFFER)))
	{
		// Silence it to prevent unwanted sound looping
		memset(pvWrite, 0x00, dwLength);
		pdsb->Unlock(pvWrite, dwLength, nullptr, 0);
	}

	DWORD dwPlayCursor;
	pdsb->GetCurrentPosition(&dwPlayCursor, nullptr);
	dwWriteOffset = dwPlayCursor;

	g_buf.clear();
}

bool Audio_AddData(BYTE *pbData_, int nLength_)
{
	LPVOID pvWrite1, pvWrite2;
	DWORD dwLength1, dwLength2;
	HRESULT hr;

	// No DirectSound buffer?
	if (!pdsb)
	{
		// Determine the time between frames in ms
		DWORD dwTime = 1000 / 50;
		if (!dwTime) dwTime = 1;

		// Has the frame time changed?
		if (dwTime != dwTimer)
		{
			// Kill any existing timer
			if (hTimer) timeKillEvent(hTimer);

			// Start a new one running
			hTimer = timeSetEvent(dwTimer = dwTime, 0, TimeCallback, 0, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
		}

		// Wait for the timer event
		WaitForSingleObject(hEvent, INFINITE);
		return false;
	}

	// Loop while there's still data to write
	while (nLength_ > 0)
	{
		DWORD dwPlayCursor, dwWriteCursor;

		// Fetch current play and write cursor positions
		if (FAILED(hr = pdsb->GetCurrentPosition(&dwPlayCursor, &dwWriteCursor)))
		{
			break;
		}

		// Determine the available space at the write cursor
		int nSpace = (nSampleBufferSize + dwPlayCursor - dwWriteOffset) % nSampleBufferSize;
		nSpace = std::min(nSpace, nLength_);

		if (nSpace)
		{
			// Lock the available space
			if (FAILED(hr = pdsb->Lock(dwWriteOffset, nSpace, &pvWrite1, &dwLength1, &pvWrite2, &dwLength2, 0)))
				;
			else
			{
				dwLength1 = std::min(dwLength1, static_cast<DWORD>(nLength_));

				// Write the first part (maybe all)
				if (dwLength1)
				{
					memcpy(pvWrite1, pbData_, dwLength1);
					pbData_ += dwLength1;
					nLength_ -= dwLength1;
				}

				dwLength2 = std::min(dwLength2, static_cast<DWORD>(nLength_));

				// Write any second part
				if (dwLength2)
				{
					memcpy(pvWrite2, pbData_, dwLength2);
					pbData_ += dwLength2;
					nLength_ -= dwLength2;
				}

				// Unlock the sound buffer to commit the data
				pdsb->Unlock(pvWrite1, dwLength1, pvWrite2, dwLength2);

				// Work out the offset for the next write in the circular buffer
				dwWriteOffset = (dwWriteOffset + dwLength1 + dwLength2) % nSampleBufferSize;
			}
		}

		// All written?
		if (!nLength_)
			break;

		// Wait for more space
		Sleep(2);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////

static bool InitDirectSound(HWND hWnd)
{
	HRESULT hr;
	bool fRet = false;

	if (!g_hinstDSound)
		g_hinstDSound = LoadLibrary(L"DSOUND.DLL");

	if (g_hinstDSound)
		pfnDirectSoundCreate = reinterpret_cast<PFNDIRECTSOUNDCREATE>(GetProcAddress(g_hinstDSound, "DirectSoundCreate"));

	// Initialise DirectSound
	if (FAILED(hr = pfnDirectSoundCreate(nullptr, &pds, nullptr)))
	{
		pfnDirectSoundCreate = nullptr;
	}

	// We want priority control over the sound format while we're active
	else if (FAILED(hr = pds->SetCooperativeLevel(hWnd, DSSCL_PRIORITY)))
		;
	else
	{
		// Set up the sound format according to the sound options
		WAVEFORMATEX wf = {};
		wf.wFormatTag = WAVE_FORMAT_PCM;
		wf.nSamplesPerSec = 44100;
		wf.wBitsPerSample = 16;
		wf.nChannels = 2;
		wf.nBlockAlign = (wf.wBitsPerSample * wf.nChannels / 8);
		wf.nAvgBytesPerSec = 44100 * wf.nBlockAlign;

		int nSamplesPerFrame = (44100 / 50) + 1;
		nSampleBufferSize = nSamplesPerFrame * wf.nBlockAlign * (1 + 5);

		DSBUFFERDESC dsbd = { sizeof(DSBUFFERDESC) };
		dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLFREQUENCY | DSBCAPS_GETCURRENTPOSITION2;
		dsbd.dwBufferBytes = nSampleBufferSize;
		dsbd.lpwfxFormat = &wf;

		HRESULT hr = pds->CreateSoundBuffer(&dsbd, &pdsb, nullptr);
		if (FAILED(hr))
			;//
		else if (FAILED(hr = pdsb->Play(0, 0, DSBPLAY_LOOPING)))
			;//
		else
			fRet = true;
	}

	return fRet;
}

static void ExitDirectSound()
{
	if (pdsb) pdsb->Release(), pdsb = nullptr;
	if (pds) pds->Release(), pds = nullptr;
}


static void CALLBACK TimeCallback(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR)
{
	// Signal next frame due
	SetEvent(hEvent);
}


void SoundOut(uint8_t val, int cycles)
{
	g_synth.update(cycles, (val & 0x10) ? 0x90 : 0x80);
}

bool Audio_EndFrame(uint64_t audio_frame_cycles)
{
	g_buf.end_frame(static_cast<blip_time_t>(audio_frame_cycles));

	blip_sample_t *ps = reinterpret_cast<blip_sample_t*>(g_pbFrameSample);
	int nSamplesThisFrame = static_cast<int>(g_buf.samples_avail());

	g_buf.read_samples(ps, nSamplesThisFrame, 1);
	for (int i = 0; i < nSamplesThisFrame; ++i, ps += 2)
		ps[1] = ps[0];

	int nSize = nSamplesThisFrame * (16 * 2 / 8);
	Audio_AddData(g_pbFrameSample, nSize);

	return true;
}
