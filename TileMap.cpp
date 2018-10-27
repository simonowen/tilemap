#include "stdafx.h"
#include "TileView.h"
#include "SpectrumGame.h"	// ToDo: should just be Game.h
#include "Audio.h"
#include "Input.h"

constexpr float SCALE_INCREMENT_MOUSE = 1.25f;
constexpr float SCALE_INCREMENT_KEYBOARD = 1.5f;
constexpr int HORIZ_PAN_STEP_KEYBOARD = 256;		// ToDo: ask game?
constexpr int VERT_PAN_STEP_KEYBOARD = 192;

HWND g_hwnd;

std::shared_ptr<TileView> g_pTileView;
std::shared_ptr<Game> g_pGame;

std::shared_ptr<Game> SelectGame(HINSTANCE hinst, std::shared_ptr<TileView> pTileView);

std::array<uint8_t, 8> matrix;

bool active = true;

////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WindowProc(
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam)
{
	static int mouse_x, mouse_y;
	static bool lbutton_down = false;
	static bool rbutton_down = false;
	static Tile *pTile = nullptr;

	switch (msg)
	{
	case WM_ACTIVATE:
		active = LOWORD(wParam) != WA_INACTIVE;
		break;

	case WM_ACTIVATEAPP:
		Input_ReleaseKeys(matrix);
		break;

	case WM_SIZE:
		g_pTileView->OnResize();
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_LBUTTONDOWN:
		mouse_x = GET_X_LPARAM(lParam);
		mouse_y = GET_Y_LPARAM(lParam);
		lbutton_down = true;
		SetCapture(hwnd);
		SetCursor(LoadCursor(NULL, IDC_SIZEALL));
		break;

	case WM_LBUTTONUP:
		ReleaseCapture();
		lbutton_down = false;
		break;

	case WM_RBUTTONDOWN:
		mouse_x = GET_X_LPARAM(lParam);
		mouse_y = GET_Y_LPARAM(lParam);

		if (GetKeyState(VK_RCONTROL) < 0)
		{
			pTile = g_pTileView->TileFromWindowPoint(mouse_x, mouse_y);
			if (pTile)
			{
				rbutton_down = true;
				SetCapture(hwnd);
				SetCursor(LoadCursor(NULL, IDC_SIZEALL));
			}
		}
		break;

	case WM_RBUTTONUP:
		ReleaseCapture();
		rbutton_down = false;
		break;

	case WM_CANCELMODE:
		lbutton_down = rbutton_down = false;
		break;

	case WM_MOUSEMOVE:
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		if (lbutton_down)
			g_pTileView->SetViewRelativeOffset(x - mouse_x, y - mouse_y, false);
		else if (rbutton_down)
			g_pTileView->MoveTileRelativeOffset(*pTile, x - mouse_x, y - mouse_y);

		mouse_x = x;
		mouse_y = y;
		return 0;
	}

	case WM_MOUSEWHEEL:
	{
		POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(hwnd, &pt);

		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / 120;
		auto scale = (zDelta < 0) ?
			(1.0f / SCALE_INCREMENT_MOUSE) : (1.0f * SCALE_INCREMENT_MOUSE);
		g_pTileView->SetRelativeViewScale(scale, pt.x, pt.y, true);
		return 0;
	}

	case WM_KEYDOWN:
	{
		bool control = GetKeyState(VK_CONTROL) < 0;
		bool shift = GetKeyState(VK_SHIFT) < 0;
		int step_x = shift ? 1 : HORIZ_PAN_STEP_KEYBOARD;
		int step_y = shift ? 1 : VERT_PAN_STEP_KEYBOARD;

		//if (lParam & 0x4000'0000)
		//	break;	// ignore key repeats

		switch (wParam)
		{
		case VK_F1:
		case VK_F2:
		case VK_F3:
		case VK_F4:
			g_pTileView->SetViewScale(static_cast<float>(wParam - VK_F1 + 1));
			break;

		case VK_NEXT:
		case VK_OEM_MINUS:
			g_pTileView->SetRelativeViewScale(1.0f / SCALE_INCREMENT_KEYBOARD);
			break;
		case VK_PRIOR:
		case VK_OEM_PLUS:
			g_pTileView->SetRelativeViewScale(SCALE_INCREMENT_KEYBOARD);
			break;

		case VK_HOME:
		{
			auto current_room = g_pGame->GetCurrentRoom();
			auto &td_current = g_pGame->FindRoomTile(current_room);
			g_pTileView->CentreViewOnTile(td_current);

			if (control || g_pTileView->GetScale() < 1.0f || g_pTileView->GetScale() > 4.0f)
				g_pTileView->SetViewScale(DEFAULT_VIEW_MAGNIFY);
			break;
		}

		case VK_END:
			g_pTileView->ScaleToFit(control);
			break;

		case VK_INSERT:
		case VK_DELETE:
			g_pTileView->SetSingleTile(wParam == VK_DELETE);
			break;

		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
		{
			int dx = 0, dy = 0;
			if (wParam == VK_LEFT) dx = -step_x;
			if (wParam == VK_RIGHT) dx = step_x;
			if (wParam == VK_UP) dy = -step_y;
			if (wParam == VK_DOWN) dy = step_y;

			if (control)
			{
				auto current_room = g_pGame->GetCurrentRoom();
				auto &td_current = g_pGame->FindRoomTile(current_room);
				td_current.x += dx;
				td_current.y += dy;
				g_pTileView->UpdateTiles();

				wchar_t sz[256];
				wsprintf(sz, L"x=%d, y=%d", td_current.x, td_current.y);
				SetWindowText(hwnd, sz);
			}
			else
				g_pTileView->SetViewRelativeOffset(-dx, -dy);
			break;
		}
		break;
		}

		break;
	}

	case WM_ENTERMENULOOP:
	case WM_NCLBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
		SilenceAudio();
		return DefWindowProc(hwnd, msg, wParam, lParam);

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

HWND InitWindow(
	_In_ HINSTANCE hInstance)
{
	WNDCLASSEX wc{ sizeof(wc) };
	wc.lpszClassName = L"tilemap_window_class";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	if (!RegisterClassEx(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
		return NULL;

	return CreateWindowEx(
		NULL,
		wc.lpszClassName,
		L"TileMap",
		WS_OVERLAPPEDWINDOW,
		0, 0,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);
}

int WINAPI
WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE /*hPrevInstance*/,
	_In_ LPSTR /*lpCmdLine*/,
	_In_ int nCmdShow)
{
	SetUnhandledExceptionFilter(CrashDumpUnhandledExceptionFilter);

#ifdef _DEBUG
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(123);
#endif

	for (;;)
	{
	try
	{
		g_hwnd = InitWindow(hInstance);
		g_pTileView = std::make_shared<TileView>(g_hwnd);

		g_pGame = SelectGame(hInstance, g_pTileView);
		if (!g_pGame)
			break;

		HRESULT hr = g_pTileView->InitD3D();
		if (FAILED(hr))
			break;

		auto TotalSize = g_pTileView->TotalSize();
		SIZE window_size{ static_cast<LONG>(TotalSize.cx * DEFAULT_VIEW_MAGNIFY), static_cast<LONG>(TotalSize.cy * DEFAULT_VIEW_MAGNIFY) };
#if 0
		// HD size for video recording
		window_size.cx = 1920;
		window_size.cy = 1080;
#endif
		RECT rect = { 0, 0, window_size.cx, window_size.cy };
		AdjustWindowRectEx(&rect, GetWindowLong(g_hwnd, GWL_STYLE), FALSE, GetWindowLong(g_hwnd, GWL_EXSTYLE));
		SetWindowPos(g_hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
		ShowWindow(g_hwnd, SW_SHOWMAXIMIZED);

		auto current_room = g_pGame->GetCurrentRoom();
		auto &tile_current = g_pGame->FindRoomTile(current_room);
		g_pTileView->CentreViewOnTile(tile_current, true);

		Input_Init(g_hwnd, hInstance, matrix);
		Audio_Init(g_hwnd);
		SilenceAudio();

		MSG msg{};
		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message != WM_SYSKEYDOWN || msg.wParam != VK_RETURN)
					TranslateMessage(&msg);
				DispatchMessage(&msg);
				continue;
			}

			Input_ReadKeyboard(matrix);
			g_pGame->RunFrame();

			g_pTileView->UpdateScreens();
			g_pTileView->UpdateMasks();
			g_pTileView->UpdateView();

			hr = g_pTileView->Render();
			if (FAILED(hr))
				break;

			bool turbo = GetKeyState(VK_APPS) < 0;
			if (!active || turbo)
				SilenceAudio();
			if (!active || !turbo)
				Audio_EndFrame(SPECTRUM_CYCLES_PER_FRAME);
		}

		g_pTileView->ExitD3D();
		g_pTileView = nullptr;
		Audio_Exit();
	}
	catch (std::exception &e)
	{
		if (g_hwnd)
			ShowWindow(g_hwnd, SW_HIDE);

		MessageBoxA(NULL, e.what(), "TileMap Error", MB_ICONSTOP);
	}
	}

	if (g_pTileView)
		g_pTileView->ExitD3D();

	g_pGame = nullptr;
	g_pTileView = nullptr;

	Input_Exit();
	Audio_Exit();

	return 0;
}
