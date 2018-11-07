// Dynamite Dan II (193 rooms)

#include "stdafx.h"
#include "DynamiteDanII.h"

DD2_Game::DD2_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes)
	: SpectrumGame(pTileView, FIRST_ROOM)
{
	auto &mem = load_snapshot(L"dd2.sna");

	// Tweaks
	mem[0x71cd] = 0x3e;		// auto-start game

	// Seed random number generator
	mem[0x7313] = random_uint32() & 0xff;
	mem[0x7314] = random_uint32() & 0xff;

	if (cheat_pokes)
	{
		mem[0x70ce] = 0x00;		// infinite energy
		mem[0x79f0] = 0x3e;		// blow doors without bombs
		mem[0xf800] = 0x00;		// leave island without fuel
#ifdef _DEBUG
		mem[0xf805] = 0x00;		// leave without playing record
#endif
	}

	InstallHooks(mem);
	SetMap(BuildMap(mem));
	m_pTileView->SetPanels({ { 0, TILE_HEIGHT + 8, TILE_WIDTH, PANEL_HEIGHT - 8, PanelAlignment::Bottom } });
}

void DD2_Game::InstallHooks(std::vector<uint8_t> &mem)
{
	// Start of game
	Hook(mem, 0x7253, 0x21 /*LD HL,nn*/, [&](Tile &tile)
	{
		std::unique_lock<std::mutex> lock(Tile::shared_lock, std::defer_lock);
		if (lock.try_lock())
		{
			BlankScreen(tile);
			CloneToAllTiles(tile);

			auto &tile_arrival = FindRoomTile(FIRST_ROOM);
			m_pTileView->CentreViewOnTile(tile_arrival);
		}
	});

	// Airship moving
	Hook(mem, 0xf8e6, 0x1c /*INC E*/, [&](Tile &tile)
	{
		auto airship_x = 78 - ((((int)(char)Z80_D(tile)) * 8) + ((Z80_E(tile) - 1) * 2));
		auto airship_island = NUM_ISLANDS - tile.mem[0x5bda];
		auto arrival_room = mem[0x818a + (NUM_ISLANDS - airship_island)];
		auto room_x = ((NUM_ISLANDS - airship_island) * NUM_ISLANDS) + (arrival_room & 0x7);
		auto pixel_x = TileToPixels(room_x, TILE_WIDTH, TILE_GAP_X);

		tile.x = pixel_x + airship_x;
		tile.height = TILE_HEIGHT + (airship_x ? TILE_GAP_Y : 0);
		m_pTileView->UpdateTiles();

		auto &arrival_tile = FindRoomTile(arrival_room);
		m_pTileView->CentreOnWorldPointX(arrival_tile.x + arrival_tile.width / 2);
	});

	// Entering room
	Hook(mem, 0x6d00, 0x21 /*LD HL,nn*/, [&](Tile &tile)
	{
		auto new_room = Z80_A(tile);
		if (IsActiveTile(tile) && new_room != tile.room)
		{
			auto &tile_new = FindRoomTile(new_room);
			auto lock = tile_new.Lock();

			CloneTile(tile, tile_new);
			Z80_A(tile_new) = new_room;

			ActivateTile(tile_new);
			tile_new.drawing = false;
		}

		Z80_A(tile) = tile.room;
		tile.drawing = false;
	});

	// Suppress Dan redraw after room is redrawn
	Hook(mem, 0x7c2e, 0xe5 /*PUSH HL*/, [&](Tile &tile)
	{
		if (!IsActiveTile(tile))
		{
			Pop(tile);
			Ret(tile);
		}

		tile.drawing = true;
	});

	// Return-if-inactive helper.
	auto ret_if_inactive = [&](Tile &tile)
	{
		if (!IsActiveTile(tile))
			Ret(tile);
	};

	// Suppress Dan actions (drawing, falling, ...) in inactive rooms
	Hook(mem, 0x78ed, 0xdd /*LD IX,nn*/, ret_if_inactive);

	// Hide enemies in inactive rooms
	Hook(mem, 0x7368, 0x3a /*LD A,(nn)*/, ret_if_inactive);

	// Hide Blitzen in inactive rooms
	Hook(mem, 0x8192, 0xfd /*LD IY,nn*/, ret_if_inactive);
}

std::vector<MapRoom> DD2_Game::BuildMap(const std::vector<uint8_t> &mem)
{
	auto tiles = BuildRegularMap(MAP_WIDTH, MAP_HEIGHT,
		TILE_WIDTH, TILE_HEIGHT, TILE_GAP_X, TILE_GAP_Y, 0, 0,
		[](int x, int y, int pitch) { return ((MAP_HEIGHT - y - 1) * MAP_WIDTH) + x; });

	// Add airship above map
	MapRoom tile;
	tile.room = AIRSHIP_ROOM;
	tile.x = TileToPixels(62, TILE_WIDTH, TILE_GAP_X) + 328;
	tile.y = TileToPixels(-1, TILE_HEIGHT, TILE_GAP_Y);
	tile.width = TILE_WIDTH;
	tile.height = TILE_HEIGHT;
	tiles.push_back(tile);

	return tiles;
}
