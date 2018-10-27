#include "stdafx.h"
#include "SpectrumGame.h"
#include "Audio.h"
#include "Utils.h"

extern bool active;
extern HWND g_hwnd;
extern std::array<uint8_t, 8> matrix;


void SpectrumGame::CloneTile(const Tile &from_tile, Tile &to_tile)
{
	if (to_tile.room != from_tile.room)
	{
		std::copy(from_tile.mem.begin(), from_tile.mem.end(), to_tile.mem.begin());
		Z80_STATE(to_tile) = Z80_STATE(from_tile);
	}
}

void SpectrumGame::SetMap(const std::vector<MapRoom> &map)
{
	m_tiles = std::vector<Tile>(map.size());

	for (int i = 0; i < static_cast<int>(map.size()); ++i)
	{
		auto &tile = m_tiles[i];
		static_cast<MapRoom&>(tile) = map[i];

		tile.pGame = this;
		tile.screen_index = i;

		z80_power(&tile.z80, 1);
		z80_reset(&tile.z80);
		tile.z80.state = snapshot_cpu_state;
		tile.z80.callback_context = &tile;
		tile.mem = snapshot_mem;

		tile.z80.read = [](void *context, zuint16 address) -> zuint8 {
			auto &td = *reinterpret_cast<Tile*>(context);
			return td.mem[address];
		};
		tile.z80.write = [](void *context, zuint16 address, zuint8 value) {
			auto &td = *reinterpret_cast<Tile*>(context);
			if (address >= 0x4000)
				td.mem[address] = value;
		};
		tile.z80.in = [](void *context, zuint16 address) -> zuint8 {
			auto &td = *reinterpret_cast<Tile*>(context);
			if (!td.pGame->IsActiveTile(td))
				return 0xff;

			uint8_t value = 0xff;

			for (int i = 0; i < 8; ++i)
			{
				if (!(address & (1 << (8 + i))))
					value &= matrix[i];
			}

			return value;
		};
		tile.z80.out = [](void *context, zuint16 address, zuint8 value) {
			auto &td = *reinterpret_cast<Tile*>(context);
			if (td.pGame->IsActiveTile(td))
			{
				if (active && (address & 0x00ff) == 0x00fe)
					SoundOut(value, td.frame_cycles + td.z80.cycles);
			}
		};
		tile.z80.int_data = [](void *context) -> zuint32 {
			return 0xffff;
		};
		tile.z80.hook = [](void *context, zuint16 address) {
			auto &td = *reinterpret_cast<Tile*>(context);
			td.pGame->OnHook(address, td);
		};
	}

	m_pTileView->SetMap(map);
}

void SpectrumGame::RunFrame()
{
	int running = 0;
	int boosted = 0;
	int suspended = 0;

	uint64_t audio_frame_cycles = SPECTRUM_CYCLES_PER_FRAME;
	for (auto &tile : m_tiles)
	{
		// Skip running inactive tiles that aren't visible
		if (!m_pTileView->IsTileVisible(tile) && !IsActiveTile(tile))
			continue;

		auto future = m_thread_pool.enqueue([&]() {
			auto lock = tile.Lock();
			if (tile.running)
			{
				running++;

				// Run for up to 50 frames with the display disabled
				for (int i = 0; i < 50; ++i)
				{
					tile.frame_cycles = 0;	// TODO: test if still needed
					tile.frame_cycles += z80_run(&tile.z80, SPECTRUM_CYCLES_BEFORE_INT);
					z80_int(&tile.z80, ON);
					tile.frame_cycles += z80_run(&tile.z80, SPECTRUM_CYCLES_PER_INT);
					z80_int(&tile.z80, OFF);
					tile.frame_cycles -= SPECTRUM_CYCLES_PER_FRAME;

					if (tile.drawing)
						break;

					boosted++;
				}

				// Enable drawing
				tile.drawing = true;

				if (IsActiveTile(tile))
					audio_frame_cycles = SPECTRUM_CYCLES_PER_FRAME + tile.frame_cycles;
			}
			else
			{
				suspended++;
			}
		});
		m_thread_futures.push_back(std::move(future));
	}

	for (auto &future : m_thread_futures)
	{
		future.get();
	}
	m_thread_futures.clear();

	static int frames = 0;
	++frames;

	static time_t last_time;
	time_t now = time(nullptr);
	if (now != last_time)
	{
		wchar_t sz[64];
		wsprintf(sz, L"TileMap: %d fps, %d running, %d suspended, %d boosted", frames, running, suspended, boosted);
		SetWindowText(g_hwnd, sz);
		last_time = now;
		frames = 0;
	}
}

std::vector<uint8_t> &SpectrumGame::load_snapshot(
	_In_ const std::wstring &filename)
{
	snapshot_mem = FileContents(L"48.rom");
	snapshot_mem.resize(0x10000);
	
	auto file = FileContents(filename);

	auto cpu = &snapshot_cpu_state;
	Z_Z80_STATE_SP(cpu) = file[23] + (file[24] << 8);
	Z_Z80_STATE_AF(cpu) = file[21] + (file[22] << 8);
	Z_Z80_STATE_BC(cpu) = file[13] + (file[14] << 8);
	Z_Z80_STATE_DE(cpu) = file[11] + (file[12] << 8);
	Z_Z80_STATE_HL(cpu) = file[9] + (file[10] << 8);
	Z_Z80_STATE_IX(cpu) = file[17] + (file[18] << 8);
	Z_Z80_STATE_IY(cpu) = file[15] + (file[16] << 8);
	Z_Z80_STATE_AF_(cpu) = file[7] + (file[8] << 8);
	Z_Z80_STATE_BC_(cpu) = file[5] + (file[6] << 8);
	Z_Z80_STATE_DE_(cpu) = file[3] + (file[4] << 8);
	Z_Z80_STATE_HL_(cpu) = file[1] + (file[2] << 8);
	Z_Z80_STATE_I(cpu) = file[0];
	Z_Z80_STATE_R(cpu) = file[20];
	Z_Z80_STATE_IRQ(cpu) = 0;
	Z_Z80_STATE_NMI(cpu) = 0;
	Z_Z80_STATE_HALT(cpu) = 0;
	Z_Z80_STATE_IFF1(cpu) = (file[19] & 4) ? 1 : 0;
	Z_Z80_STATE_IFF2(cpu) = Z_Z80_STATE_IFF1(cpu);
	Z_Z80_STATE_EI(cpu) = Z_Z80_STATE_IFF2(cpu);
	Z_Z80_STATE_IM(cpu) = (file[25] & 3);

	auto &mem = snapshot_mem;
	std::copy(file.begin() + 27, file.end(), mem.begin() + 0x4000);

	Z_Z80_STATE_PC(cpu) = mem[Z_Z80_STATE_SP(cpu)] + (mem[Z_Z80_STATE_SP(cpu) + 1] << 8);
	Z_Z80_STATE_SP(cpu) = Z_Z80_STATE_SP(cpu) + 2;

	return mem;
}

void SpectrumGame::BlankScreen(Tile &tile)
{
	std::memset(tile.mem.data() + 0x4000, 0, SPECTRUM_SCREEN_BYTES);
}


inline bool GetPixel(int x, int y, uint8_t *pDisplay, uint8_t *pMask)
{
	auto attr_offset = 0x1800 +
		((y & 0xf8) << 2) +
		((x & 0xf8) >> 3);

	auto ink = pDisplay[attr_offset] & 7;
	auto paper = (pDisplay[attr_offset] >> 3) & 7;
	if (ink != 0 && paper != 0)
		return true;

	auto data_offset =
		((y & 0xc0) << 5) +
		((y & 0x38) << 2) +
		((y & 0x07) << 8) +
		((x & 0xf8) >> 3);
	auto data_mask = 1 << (7 - (x & 0x07));
	if ((pDisplay[data_offset] & data_mask) ? ink : paper)
		return true;

	if (pMask[data_offset] & data_mask)
		return true;

	return false;
}

inline void SetPixel(int x, int y, uint8_t *pMask)
{
	auto data_offset =
		((y & 0xc0) << 5) +
		((y & 0x38) << 2) +
		((y & 0x07) << 8) +
		((x & 0xf8) >> 3);
	auto data_mask = 1 << (7 - (x & 0x07));
	pMask[data_offset] |= data_mask;
}

void SpectrumGame::CreateTileMask(Tile &tile, uint8_t *pDisplay, uint8_t *pMask)
{
	std::stack<POINT> to_fill;
	for (auto &pt : FillHints(tile))
		to_fill.push(pt);

	memset(pMask, 0x00, 0x1800);

	while (!to_fill.empty())
	{
		auto top = to_fill.top();
		to_fill.pop();
		if (top.x < 0 || top.x >= SPECTRUM_WIDTH_PIXELS ||
			top.y < 0 || top.y >= SPECTRUM_HEIGHT_PIXELS)
		{
			continue;
		}

		if (GetPixel(top.x, top.y, pDisplay, pMask))
			continue;

		SetPixel(top.x, top.y, pMask);
		to_fill.push({ top.x, top.y + 1 });
		to_fill.push({ top.x, top.y - 1 });
		to_fill.push({ top.x + 1, top.y });
		to_fill.push({ top.x - 1, top.y });
	}
}

std::vector<POINT> SpectrumGame::FillHints(const Tile &tile)
{
	return {
		{0, 0},
		{ SPECTRUM_WIDTH_PIXELS - 1, 0 },
		{ SPECTRUM_WIDTH_PIXELS - 1, SPECTRUM_HEIGHT_PIXELS - 1 },
		{ 0, SPECTRUM_HEIGHT_PIXELS - 1 }
	};
}
