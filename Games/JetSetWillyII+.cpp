// Jet Set Willy II+ (147 rooms)
//
// ToDo:
//	- seamless mode, except for lifts

#include "stdafx.h"
#include "JetSetWillyII+.h"

JSW2_Game::JSW2_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes)
	: SpectrumGame(pTileView)
{
	auto &mem = load_snapshot(L"jsw2+.sna");
	m_current_room = m_start_room = mem[START_ROOM_ADDR];

	// Tweaks
	mem[0x9754] = 0xc9;		// skip menu tune
	mem[0x7957] = 0x3e;		// auto-start game
	mem[0x7a6d] = 0xc3;		// disable pause
	mem[0x7e72] = 0x01;		// enter rooms closer to left edge
	mem[0x7e7c] = 0x7a;		// enter rooms closer to right edge

	if (cheat_pokes)
		mem[0x7f16] = 0xb6;		// infinite lives

	InstallHooks(mem);
	SetMap(BuildMap(mem));
	m_pTileView->SetPanels({ { 0, TILE_HEIGHT + 16, TILE_WIDTH, PANEL_HEIGHT - 16, PanelAlignment::Bottom } });
}

void JSW2_Game::InstallHooks(std::vector<uint8_t> &mem)
{
	// Start of game from menu
	Hook(mem, 0x7959, 0xcd, [&](Tile &tile)
	{
		// CALL nn
		Z80_PC(tile) += 3;
		Call(tile, tile.DPeek(Z80_PC(tile) - 2));

		if (IsActiveTile(tile))
		{
			BlankScreen(tile);
			CloneToAllTiles(tile);

			auto &tile_start = FindRoomTile(m_start_room);
			ActivateTile(tile_start);
		}
	});

	// Room change
	Hook(mem, 0x7991, 0x32, [&](Tile &tile)
	{
		// LD (nn),A
		Z80_PC(tile) += 3;

		auto new_room = Z80_A(tile);
		if (IsActiveTile(tile) && new_room != tile.room)
		{
			auto &tile_new = FindRoomTile(new_room);
			auto lock = tile_new.Lock();

			CloneTile(tile, tile_new);
			Z80_A(tile_new) = tile_new.mem[0x5082] = new_room;

			// Don't get stuck in Experiment X wall due to closer room entry!
			if (tile_new.room == 139 && tile_new.mem[0x5080] == 1)
				tile_new.mem[0x5080] = 3;

			ActivateTile(tile_new);
			tile_new.drawing = false;
		}

		Z80_A(tile) = tile.mem[0x5082] = tile.room;
		tile.drawing = false;
	});

	// Room drawing complete
	Hook(mem, 0x799d, 0xcd, [&](Tile &tile)
	{
		// CALL nn
		Z80_PC(tile) += 3;

		// Update the Cartography Room map if it's not the current room.
		if (IsActiveTile(tile) && tile.room != CARTOGRAPHY_ROOM)
		{
			auto &td_cart = FindRoomTile(CARTOGRAPHY_ROOM);
			auto lock = td_cart.Lock();
			CloneTile(tile, td_cart);

			Z80_A(td_cart) = td_cart.mem[0x5082] = td_cart.room;
			Z80_PC(td_cart) = 0x7994;
			td_cart.drawing = false;
		}

		Call(tile, tile.DPeek(Z80_PC(tile) - 2));
		tile.drawing = true;
	});

	// Call to core game logic
	Hook(mem, 0x7a07, 0xcd, [&](Tile &tile)
	{
		// CALL nn
		Z80_PC(tile) += 3;

		if (IsActiveTile(tile))
			Call(tile, tile.DPeek(Z80_PC(tile) - 2));
		else
		{
			tile.mem[0x5080] = 0x08;

			// Demo loop, with calls in reverse order due to stacking!
			Call(tile, 0x7aa4);				// copy back buffer to screen
			Call(tile, 0x96cc);				// animate travelators
			Call(tile, 0x80c7);				// move enemies
			Call(tile, 0x88c6);				// draw flashing items
			if (tile.mem[0x519e] & 0x80)	// room has a rope?
				Call(tile, 0x927a);			// animate rope
			Call(tile, 0x89f4);				// animate lifts
		}
	});
}

static const std::vector<SparseMapRoom> jsw2_plus_map =
{
	{ 115, 17,0 },		// Maria in Space
	{ 116, 18,0 },		// Banned
	{ 117, 19,0 },		// (Flower) Power Source
	{ 118, 20,0 },		// Star Drive (Early Brick Version)

	{  96,  7,1 },		// Beam me Down Spotty
	{  97,  8,1 },		// Captain Slog
	{  98,  9,1 },		// Alienate?
	{  99, 10,1 },		// Ship's Computer
	{ 100, 11,1 },		// MAIN LIFT 1
	{ 101, 12,1 },		// Phaser Power
	{ 102, 13,1 },		// Sickbay
	{ 103, 14,1 },		// Foot Room
						// ...
	{ 114, 17,1 },		// Someone Else

	{  94,  4,2 },		// Mega Hill
						// ...
	{ 104, 10,2 },		// Defence System
	{ 105, 11,2 },		// MAIN LIFT 2
						// ...
	{ 113, 17,2 },		// The TROUBLE with TRIBBLES is...

	{  89,  0,3 },		// The Outlet
	{  90,  1,3 },		// In The Drains
	{  91,  2,3 },		// Nasties
	{  92,  3,3 },		// Main Entrance (The Sewer)
	{  93,  4,3 },		// Holt Road
	{ 139,  5,3 },		// Experiment X
	{ 140,  6,3 },		// The secret lab
	{ 141,  7,3 },		// The Kimberlite Pipe
						// ...
	{ 106, 10,3 },		// Photon Tube
	{ 107, 11,3 },		// MAIN LIFT 3
	{ 108, 12,3 },		// Cartography Room (start room)
	{ 138, 13,3 },		// Unrestrained
	{ 109, 14,3 },		// Docking Bay
	{ 110, 15,3 },		// NCC 1501
	{ 111, 16,3 },		// Aye 'Appen
	{ 112, 17,3 },		// Shuttle Bay
	{ 134, 18,3 },		// Glitch In Holodeck XX
	{ 135, 19,3 },		// Glitch In Holodeck XX
	{ 136, 20,3 },		// Glitch In Holodeck XX
	{ 137, 21,3 },		// Glitch In Holodeck XX

	{  95,  4,4 },		// Downstairs
						// ...
	{ 142,  7,4 },		// Turbo Lift Down
						// ..
	{ 119, 14,4 },		// Rocket Room

	{ 145,  7,5 },		// Free Fall
						// ...
	{  48, 14,5 },		// The Watch Tower
						// ...
	{  68, 16,5 },		// Belfry
						// ...
	{ 120, 19,5 },		// Teleport
	{ 121, 20,5 },		// Galactic Invasion
	{ 122, 21,5 },		// INCREDIBLE -
	{ 123, 22,5 },		// - BIG HOLE -
	{ 124, 23,5 },		// - IN THE GROUND
	{ 125, 24,5 },		// Loony Jet Set
	{ 126, 25,5 },		// Eggoids
	{ 127, 26,5 },		// Beam Me Up Spotty

	{ 143,  7,6 },		// Abandoned Workings
	{ 144,  8,6 },		// Unused Cutting
	{ 146,  9,6 },		// Eureka
						// ...
	{  45, 11,6 },		// Nomen Luni
	{  18, 12,6 },		// On The Roof
	{  17, 13,6 },		// Up On The Battlements
	{  16, 14,6 },		// We must perform a Quirkafleeg
	{  15, 15,6 },		// I'm sure I've seen this before
	{  14, 16,6 },		// Rescue Esmerelda
	{  42, 17,6 },		// On Top Of The House
						// ...
	{ 128, 22,6 },		// The Hole with No Name
	{ 129, 23,6 },		// Secret passage

	{  41, 10,7 },		// Conservatory Roof
	{  40, 11,7 },		// Under The Roof
	{  39, 12,7 },		// The Attic
	{  60, 13,7 },		// Hero Worship
	{  38, 14,7 },		// I mean,even I don't believe this
	{  61, 15,7 },		// Present wrapping room
	{  37, 16,7 },		// Emergency Power Generator
	{  36, 17,7 },		// Priest's Hole

	{  54,  8,8 },		// Above The West Bedroom
	{  53,  9,8 },		// West Wing Roof
	{  35, 10,8 },		// The Orangery
	{  34, 11,8 },		// A bit of Tree
	{  33, 12,8 },		// Master Bedroom
	{  32, 13,8 },		// Top Landing
	{  62, 14,8 },		// Macaroni Ted
	{  63, 15,8 },		// Dumb Waiter
	{  31, 16,8 },		// The Bathroom
	{  30, 17,8 },		// Half Way Up The East Wall
	{ 133, 18,8 },		// Oh $#!+!The Central Cavern

	{  52,  8,9 },		// West Bedroom
	{  51,  9,9 },		// West Wing
	{  70, 10,9 },		// Swimming Pool
	{  29, 11,9 },		// Banyan Tree
	{  28, 12,9 },		// Nightmare Room
	{  26, 13,9 },		// First Landing
	{  64, 14,9 },		// Study
	{  65, 15,9 },		// Library
	{  25, 16,9 },		// The Chapel
	{  24, 17,9 },		// East Wall Base
						// ...
	{ 130, 19,9 },		// Without A Limb
	{  13, 20,9 },		// Out On A Limb
	{  12, 21,9 },		// Tree Top

	{  50,  8,10 },		// Back Door
	{  49,  9,10 },		// Back Stairway
	{  23, 10,10 },		// Cold Store
	{  22, 11,10 },		// West of Kitchen
	{  21, 12,10 },		// The Kitchen
	{  20, 13,10 },		// To The Kitchen / Main Stairway
	{  66, 14,10 },		// Megaron
	{  67, 15,10 },		// Butler's Pantry
	{  19, 16,10 },		// Ballroom West
	{  58, 17,10 },		// Ballroom East
	{  11, 18,10 },		// The Hall
	{  10, 19,10 },		// The Front Door
	{   9, 20,10 },		// On A Branch Over The Drive
	{   8, 21,10 },		// Inside The Megatree
	{   7, 22,10 },		// Cuckoo's Nest

	{  80,  3,11 },		// Deserted Isle
	{  79,  4,11 },		// cheat
	{  55,  5,11 },		// The Bow
	{  56,  6,11 },		// The Yacht
	{  27,  7,11 },		// The Beach
	{  47,  8,11 },		// Tool Shed
	{  46,  9,11 },		// The Wine Cellar
	{  57, 10,11 },		// Forgotten Abbey
	{  71, 11,11 },		// Trip Switch
	{  72, 12,11 },		// Willy's Lookout
	{  73, 13,11 },		// Sky Blue Pink
	{  74, 14,11 },		// Potty Pot Plant
	{  75, 15,11 },		// Rigor Mortis
	{  76, 16,11 },		// Crypt
	{  77, 17,11 },		// Decapitaire
	{  78, 18,11 },		// Money Bags
	{   5, 19,11 },		// The Security Guard
	{   4, 20,11 },		// The Drive
	{   3, 21,11 },		// At The Foot Of The Megatree
	{   2, 22,11 },		// Under The Megatree
	{   1, 23,11 },		// The Bridge
	{  69, 24,11 },		// Garden
	{   0, 25,11 },		// The Off Licence

	{  81, 12,12 },		// Wonga's Spillage Tray
	{  82, 13,12 },		// Willy's Bird Bath
	{  83, 14,12 },		// Seedy Hole
	{  84, 15,12 },		// The Zoo
	{  85, 16,12 },		// Pit Gear On
						// ...
	{  59, 19,12 },		// Highway to Hell
	{  43, 20,12 },		// Under The Drive
	{  44, 21,12 },		// Tree Root

	{  86, 16,13 },		// In T' Rat Hole
	{  87, 17,13 },		// Down T' Pit
						// ...
	{   6, 19,13 },		// Entrance To Hades

	{  88, 17,14 },		// Water Supply

	{ 131, 17,15 },		// Well

	{ 132, 17,16 },		// Dinking Vater?
};

std::vector<MapRoom> JSW2_Game::BuildMap(const std::vector<uint8_t> &mem)
{
	return BuildSparseMap(jsw2_plus_map, TILE_WIDTH, TILE_HEIGHT);
}
