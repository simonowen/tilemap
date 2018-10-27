// Dynamite Dan
//
// ToDo:
//	- seamless mode for creatures

#include "stdafx.h"
#include "DynamiteDan.h"

DD_Game::DD_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes)
	: SpectrumGame(pTileView, START_ROOM)
{
	auto &mem = load_snapshot(L"dd.sna");

	// Tweaks
	mem[0xcb61] = 0x21;		// auto-start game
	mem[0xcb69] = 0xc9;		// skip start sequence

	// Seed random number generator (refresh register)
	snapshot_cpu_state.r = random_uint32() & 0xff;

	if (cheat_pokes)
	{
		mem[0xcdc6] = mem[0xdecb] = 0x00;	// infinite lives
		mem[0xc93f] = 0xb6;					// infinite energy
		mem[0xdc27] = mem[0xdc2b] = 0x00;	// no drowning
		//mem[0xcfa4] = 0x00;				// open safe without all 8 dynamite sticks
		//mem[0xe970] = 0x06;				// no enemies
	}

	InstallHooks(mem);
	SetMap(BuildMap(mem));
	m_pTileView->SetPanels({ { 0, TILE_HEIGHT + 4, TILE_WIDTH, PANEL_HEIGHT - 5, PanelAlignment::Bottom } });
}

void DD_Game::InstallHooks(std::vector<uint8_t> &mem)
{
	// Game start from menu
	Hook(mem, 0xc8cc, 0x3e, [&](Tile &tile)
	{
		// LD A,n
		Z80_PC(tile) += 2;
		Z80_A(tile) = tile.mem[Z80_PC(tile) - 1];

		if (IsActiveTile(tile))
		{
			CloneToAllTiles(tile);

			auto &tile_down = FindRoomTile(DOWN_ONE_ROOM);
			m_pTileView->CentreViewOnTile(tile_down);

			auto &tile_start = FindRoomTile(Z80_A(tile));
			ActivateTile(tile_start);
		}
	});

	// After enemy reset
	Hook(mem, 0xef47, 0xc9, [&](Tile &tile)
	{
		// RET
		Ret(tile);

		// Duplicate enemies in the overlap area between rooms
		std::vector<uint16_t> dup_enemies = {
			0xa4c3, 0xa509,		// 2nd room down in lift [red], and room left [yellow]
			0xa629, 0xa631,		// below room right of start [white + magenta]
			0xa427, 0xa42f,		// above safe [yellow + white]
			0xa417, 0xa41f,		// below safe [magenta + yellow]
			0x9f71, 0x9f79,		// room with jukebox [meganta + yellow]
			0xa897,				// start room [white]
			0xa04b,				// 4th room down in lift [red]
		};

		// Remove the duplicate enemies that can't be touched
		for (auto enemy : dup_enemies)
			tile.mem[enemy + 4] |= 0x40;
	});

	// Entering room
	Hook(mem, 0xcbbd, 0x32, [&](Tile &tile)
	{
		// LD (nn),A
		Z80_PC(tile) += 3;
		auto room_addr = tile.DPeek(Z80_PC(tile) - 2);

		auto new_room = Z80_A(tile);
		if (IsActiveTile(tile) && new_room != tile.room)
		{
			auto &tile_new = FindRoomTile(new_room);
			auto lock = tile_new.Lock();

			CloneTile(tile, tile_new);
			Z80_A(tile_new) = tile_new.mem[room_addr] = new_room;

			ActivateTile(tile_new);
			tile_new.drawing = false;
		}

		Z80_A(tile) = tile.mem[room_addr] = tile.room;
		tile.mem[0xc880] |= 1;
		tile.drawing = false;
	});

	// Dan handler (drawing, falling, ...)
	Hook(mem, 0xcfd9, 0x3a, [&](Tile &tile)
	{
		// LD A,(nn)
		Z80_PC(tile) += 3;
		Z80_A(tile) = tile.mem[tile.DPeek(Z80_PC(tile) - 2)];

		if (!IsActiveTile(tile))
			Ret(tile);

		tile.drawing = true;
	});

	// Hide Dan on lift in inactive rooms
	Hook(mem, 0xd184, 0xcd, [&](Tile &tile)
	{
		// CALL nn
		Z80_PC(tile) += 3;

		if (IsActiveTile(tile))
			Call(tile, tile.DPeek(Z80_PC(tile) - 2));
	});

	// Hide lift in inactive room (due to position assist)
	Hook(mem, 0xcd3c, 0x12, [&](Tile &tile)
	{
		// LD (DE),A
		Z80_PC(tile)++;

		if (IsActiveTile(tile))
			tile.mem[Z80_DE(tile)] = Z80_A(tile);
	});

	// Hide raft in inactive rooms (due to position assist)
	Hook(mem, 0xda3b, 0x79, [&](Tile &tile)
	{
		// LD A,C
		Z80_PC(tile)++;
		Z80_A(tile) = Z80_C(tile);

		if (!IsActiveTile(tile))
			Ret(tile);
	});
}

std::vector<MapRoom> DD_Game::BuildMap(const std::vector<uint8_t> &mem)
{
	return BuildRegularMap(MAP_WIDTH, MAP_HEIGHT, TILE_WIDTH, TILE_HEIGHT,
		TILE_GAP_X, TILE_GAP_Y, 0, 0,
		[](int x, int y, int) { return (MAP_HEIGHT - y - 1) * MAP_WIDTH + (MAP_WIDTH - x - 1); });
}
