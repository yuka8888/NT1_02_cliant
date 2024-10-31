#include <windows.h>
#include <process.h>
#include <mmsystem.h>

#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "winmm.lib")


HWND hwMain;

// 送受信する座標データ
struct POS
{
	int x;
	int y;
};
POS pos1P, pos2P, old_pos1P;
RECT rect;

// プロトタイプ宣言
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI Threadfunc(void*);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_ LPSTR szCmdLine, _In_ int iCmdShow) {

	MSG  msg;
	WNDCLASS wndclass;
	WSADATA wdData;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = "CWindow";

	RegisterClass(&wndclass);

	// winsock初期化
	WSAStartup(MAKEWORD(2, 0), &wdData);

	hwMain = CreateWindow("CWindow", "Server",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600, NULL, NULL, hInstance, NULL);

	// ウインドウ表示
	ShowWindow(hwMain, iCmdShow);

	// ウィンドウ領域更新(WM_PAINTメッセージを発行)
	UpdateWindow(hwMain);

	// メッセージループ
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// winsock終了
	WSACleanup();

	return 0;
}

// ウインドウプロシージャ
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {

	static HDC hdc, mdc, mdc2P;
	static PAINTSTRUCT ps;
	static HBITMAP hBitmap;
	static HBITMAP hBitmap2P;
	static char str[256];

	static HANDLE hThread;
	static DWORD dwID;

	// WINDOWSから飛んで来るメッセージに対応する処理の記述
	switch (iMsg) {
		case WM_CREATE:
			// リソースからビットマップを読み込む（1P）
			hBitmap = LoadBitmap(((LPCREATESTRUCT)lParam)->hInstance, "MAJO");

			// ディスプレイと互換性のある論理デバイス（デバイスコンテキスト）を取得（1P）
			mdc = CreateCompatibleDC(NULL);

			// 論理デバイスに読み込んだビットマップを展開（1P）
			SelectObject(mdc, hBitmap);

			// リソースからビットマップを読み込む（2P）
			hBitmap2P = LoadBitmap(((LPCREATESTRUCT)lParam)->hInstance, "MAJO2P");

			// （2P）デバイスコンテキストを作成
			mdc2P = CreateCompatibleDC(NULL);

			// （2P）ビットマップを選択
			SelectObject(mdc2P, hBitmap2P);

			// 位置情報を初期化
			pos1P.x = pos1P.y = 0;
			pos2P.x = pos2P.y = 100;

			// データを送受信処理をスレッドとして生成
			hThread = (HANDLE)CreateThread(NULL, 0, Threadfunc, (LPVOID)&pos1P, 0, &dwID);
			break;

		case WM_KEYDOWN:
			switch (wParam) {
				case VK_ESCAPE:
					SendMessage(hwnd, WM_CLOSE, NULL, NULL);
					break;
				case VK_RIGHT:
					pos2P.x += 20;  // →キーでサーバ側キャラのX座標を更新
					break;
				case VK_LEFT:
					pos2P.x -= 20;  // ←キーでサーバ側キャラのX座標を更新
					break;
				case VK_DOWN:
					pos2P.y += 20;  // ↓キーでサーバ側キャラのY座標を更新
					break;
				case VK_UP:
					pos2P.y -= 20;  // ↑キーでサーバ側キャラのY座標を更新
					break;
			}

			// 指定ウィンドウの指定矩形領域を更新領域に追加
			InvalidateRect(hwnd, NULL, TRUE);
			UpdateWindow(hwnd);
			break;

		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);

			// サーバ側キャラ描画
			BitBlt(hdc, pos1P.x, pos1P.y, 32, 32, mdc, 0, 0, SRCCOPY);
			// クライアント側キャラ描画
			BitBlt(hdc, pos2P.x, pos2P.y, 32, 32, mdc2P, 0, 0, SRCCOPY);

			wsprintf(str, "サーバ側：X:%d Y:%d　　クライアント側：X:%d Y:%d", pos1P.x, pos1P.y, pos2P.x, pos2P.y);
			SetWindowText(hwMain, str);

			EndPaint(hwnd, &ps);
			return 0;

		case WM_DESTROY:
			DeleteObject(hBitmap);
			DeleteDC(mdc);
			DeleteObject(hBitmap2P);
			DeleteDC(mdc2P);

			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
/* 通信スレッド関数 */
DWORD WINAPI Threadfunc(void* px) {

	SOCKET sConnect;
	WORD wPort = 8000;
	SOCKADDR_IN saServer;
	int iRecv;

	// ソケットを作成
	sConnect = socket(AF_INET, SOCK_STREAM, 0);

	if (sConnect == INVALID_SOCKET) {
		SetWindowText(hwMain, "ソケット作成失敗");
		return 1;
	}

	// サーバのアドレス情報を設定
	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = inet_addr("10.97.181.1"); // サーバのIPアドレスを指定
	saServer.sin_port = htons(wPort);

	// サーバへ接続
	if (connect(sConnect, (SOCKADDR*)&saServer, sizeof(saServer)) == SOCKET_ERROR) {
		closesocket(sConnect);
		SetWindowText(hwMain, "サーバ接続失敗");
		return 1;
	}

	SetWindowText(hwMain, "サーバ接続成功");

	while (1)
	{
		old_pos1P = pos1P;

		// クライアント側キャラの位置情報をサーバへ送信
		send(sConnect, (const char*)&pos2P, sizeof(POS), 0);

		// サーバからキャラの位置情報を受け取る
		int nRcv = recv(sConnect, (char*)&pos1P, sizeof(POS), 0);

		if (nRcv == SOCKET_ERROR) break;


		// サーバから受信した座標が更新されていたら再描画
		if (old_pos1P.x != pos1P.x || old_pos1P.y != pos1P.y)
		{
			rect.left = old_pos1P.x - 10;
			rect.top = old_pos1P.y - 10;
			rect.right = old_pos1P.x + 42;
			rect.bottom = old_pos1P.y + 42;
			InvalidateRect(hwMain, &rect, TRUE);
		}
	}

	// ソケットを閉じる
	shutdown(sConnect, 2);
	closesocket(sConnect);

	return 0;
}