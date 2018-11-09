#include "stdafx.h"
#include "resource.h"

#include "Games/Test.h"
#include "Games/DynamiteDan.h"
#include "Games/DynamiteDanII.h"
#include "Games/JetSetWilly.h"
#include "Games/JetSetWillyII+.h"
#include "Games/SabreWulf.h"
#include "Games/Spindizzy.h"
#include "Games/Starquake.h"
#include "Games/TechnicianTed.h"

constexpr int CHEATS_ENABLED_BIT = 0x10000;

enum class GameType {
	Test, DynamiteDan, DynamiteDanII, JetSetWilly, JetSetWillyIIPlus, SabreWulf, Spindizzy,
	Starquake, TechnicianTed
};

struct GameEntry
{
	std::wstring name;
	GameType type;
};

static std::vector<GameEntry> Games
{
#ifdef _DEBUG
	{ L"Test", GameType::Test },
#endif
	{ L"Dynamite Dan", GameType::DynamiteDan },
	{ L"Dynamite Dan II", GameType::DynamiteDanII },
	{ L"Jet Set Willy", GameType::JetSetWilly },
	{ L"Jet Set Willy II+", GameType::JetSetWillyIIPlus },
	{ L"Sabre Wulf", GameType::SabreWulf },
	{ L"Spindizzy", GameType::Spindizzy },
	{ L"Starquake", GameType::Starquake },
	{ L"Technician Ted", GameType::TechnicianTed },
};

INT_PTR CALLBACK DialogProc(
	_In_ HWND hdlg,
	_In_ UINT uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam)
{
	// Settings persisted within session only
	static bool cheats = true;
	static int index = 0;

	auto hwndLB = GetDlgItem(hdlg, IDC_GAMES);
	auto hwndCheats = GetDlgItem(hdlg, IDC_ENABLE_CHEATS);

	switch (uMsg)
	{
	case WM_INITDIALOG:
		Button_SetCheck(hwndCheats, cheats ? BST_CHECKED : BST_UNCHECKED);

		for (auto &game : Games)
			ListBox_AddString(hwndLB, (L" " + game.name).c_str());

		ListBox_SetCurSel(hwndLB, index);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCLOSE:
		case IDCANCEL:
			EndDialog(hdlg, -1);
			return TRUE;

		case IDC_GAMES:
			if (HIWORD(wParam) != LBN_DBLCLK)
				break;
			//[[fallthrough]]
		case IDB_PLAY:
			index = ListBox_GetCurSel(hwndLB);
			cheats = (Button_GetCheck(hwndCheats) == BST_CHECKED);
			EndDialog(hdlg, index | (cheats ? CHEATS_ENABLED_BIT : 0));
			return TRUE;
		}
		break;
	}

	return FALSE;
}

static std::shared_ptr<Game> CreateGame(std::shared_ptr<TileView> view, GameType type, bool cheats)
{
	switch (type)
	{
	case GameType::DynamiteDan:
		return Game::Create<DD_Game>(view, cheats);
	case GameType::DynamiteDanII:
		return Game::Create<DD2_Game>(view, cheats);
	case GameType::JetSetWilly:
		return Game::Create<JSW_Game>(view, cheats);
	case GameType::JetSetWillyIIPlus:
		return Game::Create<JSW2_Game>(view, cheats);
	case GameType::SabreWulf:
		return Game::Create<SW_Game>(view, cheats);
	case GameType::Spindizzy:
		return Game::Create<SD_Game>(view, cheats);
	case GameType::Starquake:
		return Game::Create<SQ_Game>(view, cheats);
	case GameType::TechnicianTed:
		return Game::Create<TT_Game>(view, cheats);
	case GameType::Test:
		return Game::Create<Test_Game>(view, cheats);
	default:
		throw std::logic_error("unknown game in CreateGame");
	}
}

std::shared_ptr<Game> SelectGame(HINSTANCE hinst, std::shared_ptr<TileView> pTileView)
{
	std::shared_ptr<Game> pGame;

	auto index = DialogBox(hinst, MAKEINTRESOURCE(IDD_TILEMAP), NULL, DialogProc);
	if (index >= 0)
	{
		bool cheats = (index & CHEATS_ENABLED_BIT) != 0;
		pGame = CreateGame(pTileView, Games[index & ~CHEATS_ENABLED_BIT].type, cheats);
	}

	return pGame;
}
