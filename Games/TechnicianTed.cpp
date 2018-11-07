// Technician Ted (48 rooms)
//
// ToDo:
//	- redraw rooms immediately on task completion
//	- lift acceleration?

#include "stdafx.h"
#include "TechnicianTed.h"

TT_Game::TT_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes)
	: SpectrumGame(pTileView, START_ROOM)
{
	auto &mem = load_snapshot(L"techted.sna");

	// Tweaks
	mem[0xc089] = 0x3e;		// auto-start game
	mem[0xaff1] = 0x18;		// disable pause
	//mem[0xad3d] = 0x3e;		// disable ending cheat protection

	if (cheat_pokes)
	{
		mem[0xacb2] = 0x32; mem[0xace2] = 0x00;	// infinite lives
		mem[0xab72] = 0x00; mem[0xb0ae] = 0x21;	// infinite time

		mem[0xacf4] = 0x18;	// no fast death loop, to allow break with infinite lives
	}

	InstallHooks(mem);
	SetMap(BuildMap(mem));
	m_pTileView->SetPanels({ { 0, TILE_HEIGHT + 8, TILE_WIDTH, PANEL_HEIGHT - 8, PanelAlignment::Bottom } });
}


void TT_Game::InstallHooks(std::vector<uint8_t> &mem)
{
	// Start of game
	Hook(mem, 0xab96, 0xaf, [&](Tile &tile)
	{
		Z80_PC(tile)++;
		Z80_A(tile) = 0;

		if (tile.room == START_ROOM)
		{
			CloneToAllTiles(tile);
			ActivateTile(tile);
		}
	});

	// Room number update on room change
	Hook(mem, 0xac73, 0x32 /*LD (nn),A*/, [&](Tile &tile)
	{
		auto room_addr = tile.DPeek(Z80_PC(tile) - 2);
		auto new_room = Z80_A(tile);

		// Entering the final room has side-effects that break the penultimate room,
		// so clone at this point instead of when the room is due to be drawn.
		if (new_room == END_ROOM)
		{
			auto &tile_new = FindRoomTile(new_room);
			auto lock = tile_new.Lock();

			CloneTile(tile, tile_new);
			Z80_A(tile_new) = tile_new.mem[room_addr] = new_room;

			ActivateTile(tile_new);
			tile_new.drawing = false;

			Z80_A(tile) = tile.room;
		}
	});

	// Room draw start
	Hook(mem, 0xae77, 0x2a /*LD HL,(nn)*/, [&](Tile &tile)
	{
		auto room_addr = tile.DPeek(Z80_PC(tile) - 2);

		if (m_task_completed)
		{
			CloneToAllTiles(tile, [&](Tile &other_tile)
			{
				Z80_L(other_tile) = other_tile.mem[room_addr] = other_tile.room;
				other_tile.drawing = false;
			});

			m_task_completed = false;
		}

		auto new_room = Z80_L(tile);
		if (IsActiveTile(tile) && new_room != tile.room)
		{
			auto &tile_new = FindRoomTile(new_room);
			auto lock = tile_new.Lock();

			CloneTile(tile, tile_new);
			Z80_L(tile_new) = tile_new.mem[room_addr], new_room;

			ActivateTile(tile_new);
			tile_new.drawing = false;

			// Vertically align the lift with the current room.
			auto &tile_lift = FindRoomTile(LIFT_ROOM);
			tile_lift.y = tile_new.y;
			m_pTileView->UpdateTiles();

			// Position closer to left/right screen edge
			auto xpos = tile_new.mem[0xa406] & 0x1f;
			if (xpos == 0x00)
			{
				tile_new.mem[0xa40a] = 0x00;
				tile_new.mem[0xa40f] = 0xff;
			}
			else if (xpos == 0x1e)
			{
				tile_new.mem[0xa40a] = 0xe0;
				tile_new.mem[0xa40f] = 0x01;
			}
		}

		Z80_L(tile) = tile.mem[room_addr] = tile.room;
		tile.drawing = false;
	});

	// After room drawn, turn drawing back on
	Hook(mem, 0xac05, 0x3e /*LD A,n*/, [&](Tile &tile)
	{
		tile.drawing = true;
	});

	// Lift floor selection
	Hook(mem, 0xbace, 0x7e /*LD A,(HL)*/, [&](Tile &tile)
	{
		auto &tile_lift = FindRoomTile(LIFT_ROOM);
		tile_lift.y = (7 - (Z80_B(tile) & 7)) * tile.height;
		m_pTileView->UpdateTiles();
		m_pTileView->EnsureTileVisible(tile);
	});

	// Task completed
	Hook(mem, 0xb373, 0x7e /*LD A,(HL)*/, [&](Tile &tile)
	{
		m_task_completed = true;
	});

	// Drawing Ted
	Hook(mem, 0xbd89, 0xcd /*CALL nn*/, [&](Tile &tile)
	{
		if (!IsActiveTile(tile))
			Ret(tile);
	});

	// Stop Ted falling in inactive rooms
	Hook(mem, 0xbb87, 0x21 /*LD HL,nn*/, [&](Tile &tile)
	{
		if (!IsActiveTile(tile))
			Z80_PC(tile) = 0xbba0;
	});

	// Stop Ted reacting to travellators in inactive rooms
	Hook(mem, 0xbcd6, 0x3a /*LD A,(nn)*/, [&](Tile &tile)
	{
		if (!IsActiveTile(tile))
			Z80_A(tile) = 0;
	});

	// Make Ted invulnerable in inactive rooms
	Hook(mem, 0xba84, 0x37 /*SCF*/, [&](Tile &tile)
	{
		if (!IsActiveTile(tile))
			Z80_F(tile) &= ~1;	// clear carry
	});
}

static const std::vector<SparseMapRoom> tt_map =
{
	{ 20, 3,0 },	// Matthew's Lair
	{ 24, 4,0 },	// Electrical Conduit Tubing
	{ 23, 5,0 },	// On Top Of The Factory
	{ 22, 6,0 },	// The Wage Department
	{ 19, 7,0 },	// Abrasion Dust Extraction
	{ 11, 8,0 },	// Slice Surface Abrasion

	{ 25, 3,1 },	// Trademan's Entrance
	{ 26, 4,1 },	// The Fire Escape
	{ 18, 5,1 },	// Quality Assurance Department
	{ 12, 6,1 },	// Laser Slice Separation Plant
	{  5, 7,1 },	// The Fuming Cupboard
	{  6, 8,1 },	// The Clean Room

	{ 27, 2,2 },	// The Official Union Flag
	{ 28, 3,2 },	// A Bit Of Fresh Air
	{ 21, 4,2 },	// We Call Him Sir!
	{  2, 5,2 },	// Ted's Desk
	{  8, 6,2 },	// The Canteen
	{  7, 7,2 },	// The Silicon Etching Tank
	{ 29, 8,2 },	// The Acid Bottle Store

	{ 48, 0,3 },	// [Game Completion Sequence]
	{ 47, 1,3 },	// OK ! Where Do I Get My Reward
	{ 30, 2,3 },	// The Picket Line
	{  0, 3,3 },	// The Factory Gates
	{  1, 4,3 },	// Reception
	{ 31, 5,3 },	// The Cloakroom
	{  3, 6,3 },	// Silicon Slice Store
	{ 17, 7,3 },	// The ElectroPlating Bath
	{ 14, 8,3 },	// Micro-Chip Mounting Furnace
	{ 42, 9,3 },	// The Lift

	{ 46, 1,4 },	// Surbiton Colliery
	{ 32, 2,4 },	// The Shop Steward
	{  4, 3,4 },	// Slice Diffusion Furnaces
	{ 10, 4,4 },	// The Boardroom
	{  9, 5,4 },	// The Photocopier
	{ 33, 6,4 },	// Main Corridor
	{ 13, 7,4 },	// Bay 7
	{ 34, 8,4 },	// Danger= High Voltage Testing

	{ 35, 3,5 },	// Under A Diffusion Furnace
	{ 16, 4,5 },	// Power Generator
	{ 36, 5,5 },	// Power Cables
	{ 37, 6,5 },	// Beaker Store
	{ 38, 7,5 },	// <- Beaker Store  Tea Machine ->
	{ 15, 8,5 },	// The Tea-Machine

	{ 44, 4,6 },	// Prehistoric Caves
	{ 39, 5,6 },	// Down In The Sewerage
					// ...
	{ 40, 7,6 },	// The Cellar
	{ 41, 8,6 },	// Under MUDL's Tea Machine
	{ 43, 9,6 },	// The Forgotten Room
};

std::vector<MapRoom> TT_Game::BuildMap(const std::vector<uint8_t> &mem)
{
	return BuildSparseMap(tt_map, TILE_WIDTH, TILE_HEIGHT);
}
