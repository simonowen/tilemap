// Spindizzy (1520 rooms, 382 populated)
//
// ToDo:
//	- fix remaining fill/transparency issues (mostly crystals)
//	- find a way to restore score panels?

#include "stdafx.h"
#include "Spindizzy.h"

SD_Game::SD_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes)
	: SpectrumGame(pTileView, START_ROOM)
{
	auto &mem = load_snapshot(L"spindizzy.sna");

	// Tweaks
	mem[0xc005] = 0x3a;		// auto-start game
	mem[0xc922] = 0x3e;		// disable pause
	mem[0xb650] = 0xc9;		// no panel surrounds
	mem[0xdd1e] = 0xc9;		// no panel text

	// Graphics tweaks to reduce unwanted transparency mask flood-filling
	mem[0x9540] = 0xc0;
	mem[0x9581] = 0xff;
	mem[0x9720] = 0xff;
	mem[0x9721] = 0xff;
	mem[0x972f] = 0x03;

	if (cheat_pokes)
	{
		mem[0xc05e] = 1;	// infinite time [built-in cheat mode]
		//mem[0xcbca] = 0xc9;	// fall any height
	}

	InstallHooks(mem);
	SetMap(BuildMap(mem));
}

// ToDo: find this in game data?
static const uint8_t spindizzy_map[][5] =
{
	{ 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00001110 },
	{ 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00111100 },
	{ 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b01111000 },
	{ 0b00000000, 0b00011101, 0b01010101, 0b01010101, 0b01111000 },
	{ 0b00000000, 0b00001000, 0b00000000, 0b00000000, 0b01111000 },
	{ 0b00000000, 0b00001000, 0b00000000, 0b00000000, 0b01111000 },
	{ 0b00000000, 0b00001000, 0b00000000, 0b00000000, 0b01111000 },
	{ 0b00000000, 0b00001000, 0b00000000, 0b00000000, 0b00111100 },
	{ 0b00000000, 0b00001000, 0b00000000, 0b00000000, 0b00001110 },
	{ 0b00000000, 0b00001000, 0b00000000, 0b00000000, 0b00000000 },
	{ 0b00000000, 0b00001000, 0b00000000, 0b00000000, 0b00000100 },
	{ 0b00000000, 0b00001000, 0b00000000, 0b00000000, 0b00000100 },
	{ 0b00000000, 0b00011110, 0b00000000, 0b00000000, 0b00000000 },
	{ 0b00000000, 0b00011110, 0b00000001, 0b11000000, 0b00000100 },
	{ 0b00000000, 0b00011110, 0b00000001, 0b11000000, 0b00000100 },
	{ 0b00000000, 0b00011111, 0b10011111, 0b11111111, 0b11000000 },
	{ 0b11111111, 0b10001111, 0b10010111, 0b11100111, 0b11001000 },
	{ 0b11111111, 0b11001111, 0b10011111, 0b10011111, 0b01011100 },
	{ 0b11111111, 0b11001111, 0b10001100, 0b10010100, 0b11111100 },
	{ 0b11111111, 0b11000001, 0b00011111, 0b11110000, 0b00011100 },
	{ 0b00000111, 0b11000001, 0b11111011, 0b11100100, 0b00000000 },
	{ 0b00000001, 0b11000001, 0b10000011, 0b11101110, 0b00000000 },
	{ 0b00000001, 0b11100001, 0b01100011, 0b01100100, 0b00000000 },
	{ 0b00000001, 0b11111111, 0b11110011, 0b11100000, 0b00000000 },
	{ 0b00100001, 0b11111101, 0b11100000, 0b00000000, 0b00000000 },
	{ 0b00111111, 0b11111101, 0b11000000, 0b00000000, 0b00000000 },
	{ 0b00110000, 0b00100001, 0b11000000, 0b00000000, 0b00000000 },
	{ 0b00100111, 0b10100001, 0b01110000, 0b00000000, 0b00000000 },
	{ 0b00110111, 0b11110001, 0b00010000, 0b00000000, 0b00000000 },
	{ 0b00111111, 0b11100001, 0b00111000, 0b00000100, 0b00000000 },
	{ 0b00000011, 0b11000001, 0b00010000, 0b00001100, 0b00000000 },
	{ 0b00000011, 0b10000001, 0b01111000, 0b00011100, 0b00000000 },
	{ 0b00000000, 0b00000001, 0b11111111, 0b11111100, 0b00000000 },
	{ 0b00000000, 0b00000000, 0b00010001, 0b00011100, 0b00000000 },
	{ 0b00000000, 0b00000000, 0b00111001, 0b00001100, 0b00000000 },
	{ 0b00000000, 0b00000000, 0b00010001, 0b00000100, 0b00000000 },
	{ 0b00000000, 0b00000000, 0b00010001, 0b00000000, 0b00000000 },
	{ 0b00000000, 0b00000000, 0b00000001, 0b00000000, 0b00000000 },
};

void SD_Game::InstallHooks(std::vector<uint8_t> &mem)
{
	// Starting game from menu
	Hook(mem, 0xc7c0, 0xcd, [&](Tile &tile)
	{
		// CALL nn
		Z80_PC(tile) += 3;
		Call(tile, tile.DPeek(Z80_PC(tile) - 2));

		if (IsActiveTile(tile))
		{
			BlankScreen(tile);
			CloneToAllTiles(tile);

			auto &tile_start = FindRoomTile(START_ROOM);
			ActivateTile(tile_start);
		}
	});

	// Entering room
	Hook(mem, 0xccb4, 0xed, [&](Tile &tile)
	{
		// LD DE,(nn)
		Z80_PC(tile) += 4;
		auto room_addr = tile.DPeek(Z80_PC(tile) - 2);

		auto new_room = tile.DPeek(room_addr);
		if (IsActiveTile(tile) && new_room != tile.room)
		{
			auto &tile_new = FindRoomTile(new_room);
			auto lock = tile_new.Lock();

			CloneTile(tile, tile_new);
			Z80_DE(tile_new) = tile_new.DPoke(room_addr, tile_new.room);

			ActivateTile(tile_new);
			tile_new.drawing = false;
		}

		// Hide and disable the main character
		if (!IsActiveTile(tile))
		{
			tile.mem[0xca44] = 0xf0;					// position well below the map
			tile.mem[0xca40] = tile.mem[0xca42] = 4;	// position at centre of room
			tile.DPoke(0xca4f, tile.DPoke(0xca52, 0));	// no x or y velocity
			tile.mem[0xca98] = 0;						// no gravity
		}

		Z80_DE(tile) = tile.DPoke(room_addr, tile.room);
		tile.drawing = false;
	});

	// Room drawing complete
	Hook(mem, 0xb969, 0xc9, [&](Tile &tile)
	{
		Ret(tile);

		if (!IsActiveTile(tile))
		{
			auto tile_x = (tile.room >> 8) - MAP_BASE_X;
			auto tile_y = (tile.room & 0xff) - MAP_BASE_Y;
			auto offset = tile_x / 8;
			auto bit = (7 - (tile_x % 8));

			if (!(spindizzy_map[tile_y][offset] & (1 << bit)))
			{
				tile.running = tile.drawing = false;
				return;
			}
		}

		tile.drawing = true;
		tile.mask_dirty = true;
	});

	// Hide bottom-right indicator in inactive rooms (or if blue)
	Hook(mem, 0xb9bb, 0x3c, [&](Tile &tile)
	{
		Z80_PC(tile) += 1;

		if (IsActiveTile(tile) && Z80_A(tile) != 0)
			Z80_A(tile)++;
	});
}

std::vector<MapRoom> SD_Game::BuildMap(const std::vector<uint8_t> &mem)
{
	std::vector<MapRoom> tiles;

	constexpr int MAP_HEIGHT = _countof(spindizzy_map);
	constexpr int MAP_WIDTH = sizeof(spindizzy_map[0]) * 8;

	for (int tile_y = 0; tile_y < MAP_HEIGHT; ++tile_y)
	{
		for (int tile_x = 0; tile_x < MAP_WIDTH; ++tile_x)
		{
			MapRoom tile;
			tile.room = ((tile_x + MAP_BASE_X) << 8) | (tile_y + MAP_BASE_Y);
			tile.x = TileToPixels(tile_x, TILE_WIDTH, -128) - (tile_y * 128);
			tile.y = TileToPixels(tile_y, TILE_HEIGHT, PANEL_HEIGHT - 128) + (tile_x * 64);
			tile.width = TILE_WIDTH;
			tile.height = TILE_HEIGHT;
			tile.offset_x = TILE_OFFSET_X;
			tile.offset_y = TILE_OFFSET_Y;
			tiles.push_back(tile);
		}
	}

	return tiles;
}

std::vector<POINT> SD_Game::FillHints(const Tile &tile)
{
	std::vector<POINT> points;

	// Fill various points around the edge of the screen
	points.push_back({ SPECTRUM_WIDTH_PIXELS * 1/4, 0 });
	points.push_back({ SPECTRUM_WIDTH_PIXELS * 3/4, 0 });
	points.push_back({ SPECTRUM_WIDTH_PIXELS * 1/4, SPECTRUM_HEIGHT_PIXELS - 1 });
	points.push_back({ SPECTRUM_WIDTH_PIXELS * 3/4, SPECTRUM_HEIGHT_PIXELS - 1 });
	points.push_back({ 0, SPECTRUM_HEIGHT_PIXELS / 2 });
	points.push_back({ SPECTRUM_WIDTH_PIXELS - 1, SPECTRUM_HEIGHT_PIXELS / 2 });
	points.push_back({ 0, SPECTRUM_HEIGHT_PIXELS - 1 });
	points.push_back({ SPECTRUM_WIDTH_PIXELS, SPECTRUM_HEIGHT_PIXELS - 1 });

	// Rooms with closed loops
	if (tile.room == 0x4240)		// two rooms right from start
	{
		// Make arch transparent
		points.push_back({ 75, 75 });
	}
	else if (tile.room == 0x323B)	// inside largest pyramid
	{
		// Make back of room transparent
		points.push_back({ SPECTRUM_WIDTH_PIXELS / 2, 40 });
	}

	return points;
}
