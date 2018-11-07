// Starquake (512 screens)
//
// ToDo:
//	- seamless mode for lasers

#include "stdafx.h"
#include "Starquake.h"

SQ_Game::SQ_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes)
	: SpectrumGame(pTileView, START_ROOM)
{
	auto &mem = load_snapshot(L"starquake.sna");

	// Tweaks
	mem[0x6024] = 0x3e;		// auto-start game
	mem[0xda33] = 0x3e;		// no menu tune
	mem[0xc561] = 0x18;		// disable pause

	// Seed random number generator (FRAMES sysvar)
	mem[0x5c78] = random_uint32() & 0xff;
	mem[0x5c79] = random_uint32() & 0xff;

	if (cheat_pokes)
	{
		mem[0xc462] = 0x00;		// infinite lives
		mem[0xd41f] = 0xc9;		// infinite energy, platforms, laser power
		mem[0xce8e] = 0x18;		// open doors without key
		mem[0xd735] = 0x00;		// pass security door and Cheops
	}

	InstallHooks(mem);
	SetMap(BuildMap(mem));
	m_pTileView->SetPanels({ { 8, 0, TILE_WIDTH - 16, PANEL_HEIGHT, PanelAlignment::Top } });
}

void SQ_Game::InstallHooks(std::vector<uint8_t> &mem)
{
	// Start of game
	Hook(mem, 0x672d, 0xcd /*CALL nn*/, [&](Tile &tile)
	{
		if (IsActiveTile(tile))
		{
			CloneToAllTiles(tile);

			auto &tile_down = FindRoomTile(DOWN_ONE_ROOM);
			m_pTileView->CentreViewOnTile(tile_down);

			auto &tile_start = FindRoomTile(START_ROOM);
			ActivateTile(tile_start);
		}
	});

	// Room change CLS
	Hook(mem, 0xa45f, 0xcd /*CALL nn*/, [&](Tile &tile)
	{
		auto room_addr = 0xd2c8;
		auto new_room = tile.DPeek(room_addr);

		if (IsActiveTile(tile) && new_room != tile.room)
		{
			auto &tile_new = FindRoomTile(new_room);
			auto lock = tile_new.Lock();

			CloneTile(tile, tile_new);
			tile_new.DPoke(room_addr, new_room);

			ActivateTile(tile_new, new_room == CORE_ROOM);
			tile_new.drawing = false;
		}

		tile.drawing = false;
		tile.DPoke(room_addr, tile.room);
	});

	// After room drawn, turn drawing back on
	Hook(mem, 0xa475, 0xcd /*CALL nn*/, [&](Tile &tile)
	{
		tile.drawing = true;
	});

	// Call-if-active helper
	auto call_if_active = [&](Tile &tile)
	{
		if (!IsActiveTile(tile) && tile.room != CORE_ROOM)
			Ret(tile);
	};

	// Skip sprite drawing and attr painting in inactive rooms
	Hook(mem, 0xd9cb, 0xcd /*CALL nn*/, call_if_active);
	Hook(mem, 0xd9ce, 0xcd /*CALL nn*/, call_if_active);

	// Stop BLOB falling in inactive rooms
	Hook(mem, 0xc747, 0x3a /*LD A,(nn)*/, [&](Tile &tile)
	{
		if (!IsActiveTile(tile))
			Z80_PC(tile)++;	// skip SUB (HL)
	});

	// Immunity from zappers in inactive rooms
	Hook(mem, 0xc35e, 0x21 /*LD HL,nn*/, [&](Tile &tile)
	{
		if (!IsActiveTile(tile))
			Ret(tile);
	});

	// Immunity from spikes in inactive rooms
	Hook(mem, 0xcbdc, 0x7a /*LD A,D*/, [&](Tile &tile)
	{
		if (!IsActiveTile(tile))
			Z80_A(tile) = 0;
	});

	// Sprites bounce indefinitely in inactive core
	Hook(mem, 0xa774, 0x21 /*LD HL,nn*/, [&](Tile &tile)
	{
		if (!IsActiveTile(tile))
			Z80_PC(tile) = 0xa759;
	});
}

std::vector<MapRoom> SQ_Game::BuildMap(const std::vector<uint8_t> &mem)
{
	return BuildRegularMap(MAP_WIDTH, MAP_HEIGHT, TILE_WIDTH, TILE_HEIGHT,
		0, 0, TILE_OFFSET_X, TILE_OFFSET_Y);
}
