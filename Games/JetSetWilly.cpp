// Jet Set Willy (60 rooms)
//
// ToDo:
//	- support custom maps in mods

#include "stdafx.h"
#include "JetSetWilly.h"

JSW_Game::JSW_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes)
	: SpectrumGame(pTileView, 0)
{
	auto &mem = load_snapshot(L"jsw.sna");
	m_current_room = mem[START_ROOM_ADDR];

	// Tweaks
	mem[0x96D9] = 0;		// press enter to start game
	mem[0x8ac2] = 0x06;		// disable inactivity timer
	mem[0x8acc] = 0x18;		// disable pause

	if (cheat_pokes)
	{
		mem[0x8c3b] = 0x00;		// infinite lives
		mem[0x8a50] = 0x18;		// infinite time
	}

	InstallHooks(mem);
	SetMap(BuildMap(mem));
	m_pTileView->SetPanels({ { 0, TILE_HEIGHT + 16, TILE_WIDTH, PANEL_HEIGHT - 24, PanelAlignment::Bottom } });
}

void JSW_Game::InstallHooks(std::vector<uint8_t> &mem)
{
	// Start of game
	Hook(mem, 0x8905, 0xed, [&](Tile &tile)
	{
		// LDIR (ignored)
		Z80_PC(tile) += 2;

		if (IsActiveTile(tile))
		{
			BlankScreen(tile);
			CloneToAllTiles(tile);

			auto &tile_start = FindRoomTile(tile.mem[START_ROOM_ADDR]);
			ActivateTile(tile_start);
		}
	});

	// Room change start
	Hook(mem, 0x8912, 0x3a, [&](Tile &tile)
	{
		// LD A,(nn)
		Z80_PC(tile) += 3;
		auto room_addr = tile.DPeek(Z80_PC(tile) - 2);

		auto new_room = tile.mem[room_addr];
		if (IsActiveTile(tile) && new_room != tile.room)
		{
			auto &tile_new = FindRoomTile(new_room);
			auto lock = tile_new.Lock();

			if (m_seamless)
			{
				// Preserve the current state of the new room so it can be restored.
				m_entity_data.clear();
				m_entity_data.insert(m_entity_data.begin(), tile_new.mem.begin() + 0x8100, tile_new.mem.begin() + 0x8140);
			}

			CloneTile(tile, tile_new);
			Z80_A(tile_new) = tile_new.mem[room_addr] = new_room;

			ActivateTile(tile_new);
			tile_new.drawing = false;

			// Skip re-initialisation in old room, to preserve its state
			Z80_PC(tile) = 0x89ad;
		}

		Z80_A(tile) = tile.mem[room_addr] = tile.room;
		tile.drawing = false;
	});

	// Room init (after entity initialisation)
	Hook(mem, 0x8988, 0xc3, [&](Tile &tile)
	{
		// JP nn
		Z80_PC(tile) = tile.DPeek(Z80_PC(tile) + 1);

		if (IsActiveTile(tile) && !m_entity_data.empty())
		{
			std::copy(m_entity_data.begin(), m_entity_data.end(), tile.mem.begin() + 0x8100);
			m_entity_data.clear();

			// Position Willy closer to the screen edge.
			if ((tile.mem[0x85d3] & 0x1f) == 0x1e)
				tile.mem[0x85d2] = 3;
			else if ((tile.mem[0x85d3] & 0x1f) == 0x00)
				tile.mem[0x85d2] = 0;
		}
	});

	// Room drawing complete
	Hook(mem, 0x8a3a, 0xcd, [&](Tile &tile)
	{
		// CALL nn
		Z80_PC(tile) += 3;
		Call(tile, tile.DPeek(Z80_PC(tile) - 2));

		tile.drawing = true;
	});

	// Draw main sprite (including collision detection)
	Hook(mem, 0x95c8, 0x2a, [&](Tile &tile)
	{
		if (IsActiveTile(tile))
		{
			Z80_HL(tile) = tile.mem[0x85d3] | (tile.mem[0x85d4] << 8);
			Z80_PC(tile) += 3;
		}
		else
		{
			Z80_PC(tile) = tile.mem[Z80_SP(tile)] + (tile.mem[Z80_SP(tile) + 1] << 8);
			Z80_SP(tile) = Z80_SP(tile) + 2;
		}
	});

	// Move Willy (falling)
	Hook(mem, 0x8dd3, 0x3a, [&](Tile &tile)
	{
		if (IsActiveTile(tile))
		{
			Z80_A(tile) = tile.mem[0x85d6];
			Z80_PC(tile) += 3;
		}
		else
		{
			Z80_PC(tile) = tile.mem[Z80_SP(tile)] + (tile.mem[Z80_SP(tile) + 1] << 8);
			Z80_SP(tile) = Z80_SP(tile) + 2;
		}
	});
}

static const std::vector<SparseMapRoom> jsw_map =
{
	{ 50, 9,0 },	// Watch Tower

	{ 48, 6,1 },	// Nomen Luni
	{ 18, 7,1 },	// On the Roof
	{ 17, 8,1 },	// Up on the Battlements
	{ 16, 9,1 },	// We must perform a Quirkafleeg
	{ 15, 10,1 },	// I'm sure I've seen this before..
	{ 14, 11,1 },	// Rescue Esmerelda
	{ 44, 12,1 },	// On top of the house

	{ 43, 5,2 },	// Conservatory Roof
	{ 42, 6,2 },	// Under the Roof
	{ 41, 7,2 },	// The Attic
	{ 40, 8,2 },	// Dr Jones will never believe this
	{ 39, 9,2 },	// Emergency Generator
	{ 38, 10,2 },	// Priests' Hole

	{ 57, 3,3 },	// Above the West Bedroom
	{ 56, 4,3 },	// West Wing Roof
	{ 37, 5,3 },	// Orangery
	{ 36, 6,3 },	// A bit of tree
	{ 35, 7,3 },	// Master Bedroom
	{ 34, 8,3 },	// Top Landing
	{ 33, 9,3 },	// The Bathroom
	{ 32, 10,3 },	// Halfway up the East Wall

	{ 55, 3,4 },	// West Bedroom
	{ 54, 4,4 },	// West  Wing
	{ 31, 5,4 },	// Swimming Pool
	{ 30, 6,4 },	// The Banyan Tree
	{ 29, 7,4 },	// The Nightmare Room
	{ 28, 8,4 },	// First Landing
	{ 27, 9,4 },	// The Chapel
	{ 26, 10,4 },	// East Wall Base
					// ...
	{ 13, 13,4 },	// Out on a limb
	{ 12, 14,4 },	// Tree Top

	{ 53, 3,5 },	// Back Door
	{ 52, 4,5 },	// Back Stairway
	{ 25, 5,5 },	// Cold Store
	{ 24, 6,5 },	// West of Kitchen
	{ 23, 7,5 },	// The Kitchen
	{ 22, 8,5 },	// To the Kitchens    Main Stairway
	{ 21, 9,5 },	// Ballroom West
	{ 20, 10,5 },	// Ballroom East
	{ 11, 11,5 },	// The Hall
	{ 10, 12,5 },	// The Front Door
	{ 9,  13,5 },	// On a Branch Over the Drive
	{ 8,  14,5 },	// Inside the MegaTrunk
	{ 7,  15,5 },	// Cuckoo's Nest

	{ 60, 0,6 },	// The Bow
	{ 59, 1,6 },	// The Yacht
	{ 58, 2,6 },	// The Beach
	{ 51, 3,6 },	// Tool  Shed
	{ 49, 4,6 },	// The Wine Cellar
					// ...
	{ 19, 11,6 },	// The Forgotten Abbey
	{ 5,  12,6 },	// The Security Guard
	{ 4,  13,6 },	// The Drive
	{ 3,  14,6 },	// At the Foot of the MegaTree
	{ 2,  15,6 },	// Under the MegaTree
	{ 1,  16,6 },	// The Bridge
	{ 0,  17,6 },	// The Off Licence

	{ 6,  12,7 },	// Entrance to Hades
	{ 45, 13,7 },	// Under the Drive
	{ 46, 14,7 },	// Tree Root
};

static std::vector<std::wstring> room_names =
{
	L"The_Off_Licence",
	L"The_Bridge",
	L"Under_the_MegaTree",
	L"At_the_Foot_of_the_MegaTree",
	L"The_Drive",
	L"The_Security_Guard",
	L"Entrance_to_Hades",
	L"Cuckoos_Nest",
	L"Inside_the_MegaTrunk",
	L"On_a_Branch_Over_the_Drive",
	L"The_Front_Door",
	L"The_Hall",
	L"Tree_Top",
	L"Out_on_a_limb",
	L"Rescue_Esmerelda",
	L"Im_sure_Ive_seen_this_before",
	L"We_must_perform_a_Quirkafleeg",
	L"Up_on_the_Battlements",
	L"On_the_Roof",
	L"The_Forgotten_Abbey",
	L"Ballroom_East",
	L"Ballroom_West",
	L"To_the_Kitchens____Main_Stairway",
	L"The_Kitchen",
	L"West_of_Kitchen",
	L"Cold_Store",
	L"East_Wall_Base",
	L"The_Chapel",
	L"First_Landing",
	L"The_Nightmare_Room",
	L"The_Banyan_Tree",
	L"Swimming_Pool",
	L"Halfway_up_the_East_Wall",
	L"The_Bathroom",
	L"Top_Landing",
	L"Master_Bedroom",
	L"A_bit_of_tree",
	L"Orangery",
	L"Priests_Hole",
	L"Emergency_Generator",
	L"Dr_Jones_will_never_believe_this",
	L"The_Attic",
	L"Under_the_Roof",
	L"Conservatory_Roof",
	L"On_top_of_the_house",
	L"Under_the_Drive",
	L"Tree_Root",
	L"BRACKET",
	L"Nomen_Luni",
	L"The_Wine_Cellar",
	L"Watch_Tower",
	L"Tool__Shed",
	L"Back_Stairway",
	L"Back_Door",
	L"West__Wing",
	L"West_Bedroom",
	L"West_Wing_Roof",
	L"Above_the_West_Bedroom",
	L"The_Beach",
	L"The_Yacht",
	L"The_Bow",
};

enum RoomName
{
	The_Off_Licence,
	The_Bridge,
	Under_the_MegaTree,
	At_the_Foot_of_the_MegaTree,
	The_Drive,
	The_Security_Guard,
	Entrance_to_Hades,
	Cuckoos_Nest,
	Inside_the_MegaTrunk,
	On_a_Branch_Over_the_Drive,
	The_Front_Door,
	The_Hall,
	Tree_Top,
	Out_on_a_limb,
	Rescue_Esmerelda,
	Im_sure_Ive_seen_this_before,
	We_must_perform_a_Quirkafleeg,
	Up_on_the_Battlements,
	On_the_Roof,
	The_Forgotten_Abbey,
	Ballroom_East,
	Ballroom_West,
	To_the_Kitchens____Main_Stairway,
	The_Kitchen,
	West_of_Kitchen,
	Cold_Store,
	East_Wall_Base,
	The_Chapel,
	First_Landing,
	The_Nightmare_Room,
	The_Banyan_Tree,
	Swimming_Pool,
	Halfway_up_the_East_Wall,
	The_Bathroom,
	Top_Landing,
	Master_Bedroom,
	A_bit_of_tree,
	Orangery,
	Priests_Hole,
	Emergency_Generator,
	Dr_Jones_will_never_believe_this,
	The_Attic,
	Under_the_Roof,
	Conservatory_Roof,
	On_top_of_the_house,
	Under_the_Drive,
	Tree_Root,
	BRACKET,
	Nomen_Luni,
	The_Wine_Cellar,
	Watch_Tower,
	Tool__Shed,
	Back_Stairway,
	Back_Door,
	West__Wing,
	West_Bedroom,
	West_Wing_Roof,
	Above_the_West_Bedroom,
	The_Beach,
	The_Yacht,
	The_Bow
};

std::vector<MapRoom> JSW_Game::BuildMap(const std::vector<uint8_t> &mem)
{
	return BuildSparseMap(jsw_map, TILE_WIDTH, TILE_HEIGHT);
}
