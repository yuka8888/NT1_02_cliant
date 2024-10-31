#include <windows.h>
#include <process.h>
#include <mmsystem.h>

#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "winmm.lib")


HWND hwMain;

// ����M������W�f�[�^
struct POS
{
	int x;
	int y;
};
POS pos1P, pos2P, old_pos1P;
RECT rect;

// �v���g�^�C�v�錾
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

	// winsock������
	WSAStartup(MAKEWORD(2, 0), &wdData);

	hwMain = CreateWindow("CWindow", "Server",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600, NULL, NULL, hInstance, NULL);

	// �E�C���h�E�\��
	ShowWindow(hwMain, iCmdShow);

	// �E�B���h�E�̈�X�V(WM_PAINT���b�Z�[�W�𔭍s)
	UpdateWindow(hwMain);

	// ���b�Z�[�W���[�v
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// winsock�I��
	WSACleanup();

	return 0;
}

// �E�C���h�E�v���V�[�W��
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {

	static HDC hdc, mdc, mdc2P;
	static PAINTSTRUCT ps;
	static HBITMAP hBitmap;
	static HBITMAP hBitmap2P;
	static char str[256];

	static HANDLE hThread;
	static DWORD dwID;

	// WINDOWS������ŗ��郁�b�Z�[�W�ɑΉ����鏈���̋L�q
	switch (iMsg) {
		case WM_CREATE:
			// ���\�[�X����r�b�g�}�b�v��ǂݍ��ށi1P�j
			hBitmap = LoadBitmap(((LPCREATESTRUCT)lParam)->hInstance, "MAJO");

			// �f�B�X�v���C�ƌ݊����̂���_���f�o�C�X�i�f�o�C�X�R���e�L�X�g�j���擾�i1P�j
			mdc = CreateCompatibleDC(NULL);

			// �_���f�o�C�X�ɓǂݍ��񂾃r�b�g�}�b�v��W�J�i1P�j
			SelectObject(mdc, hBitmap);

			// ���\�[�X����r�b�g�}�b�v��ǂݍ��ށi2P�j
			hBitmap2P = LoadBitmap(((LPCREATESTRUCT)lParam)->hInstance, "MAJO2P");

			// �i2P�j�f�o�C�X�R���e�L�X�g���쐬
			mdc2P = CreateCompatibleDC(NULL);

			// �i2P�j�r�b�g�}�b�v��I��
			SelectObject(mdc2P, hBitmap2P);

			// �ʒu����������
			pos1P.x = pos1P.y = 0;
			pos2P.x = pos2P.y = 100;

			// �f�[�^�𑗎�M�������X���b�h�Ƃ��Đ���
			hThread = (HANDLE)CreateThread(NULL, 0, Threadfunc, (LPVOID)&pos1P, 0, &dwID);
			break;

		case WM_KEYDOWN:
			switch (wParam) {
				case VK_ESCAPE:
					SendMessage(hwnd, WM_CLOSE, NULL, NULL);
					break;
				case VK_RIGHT:
					pos2P.x += 20;  // ���L�[�ŃT�[�o���L������X���W���X�V
					break;
				case VK_LEFT:
					pos2P.x -= 20;  // ���L�[�ŃT�[�o���L������X���W���X�V
					break;
				case VK_DOWN:
					pos2P.y += 20;  // ���L�[�ŃT�[�o���L������Y���W���X�V
					break;
				case VK_UP:
					pos2P.y -= 20;  // ���L�[�ŃT�[�o���L������Y���W���X�V
					break;
			}

			// �w��E�B���h�E�̎w���`�̈���X�V�̈�ɒǉ�
			InvalidateRect(hwnd, NULL, TRUE);
			UpdateWindow(hwnd);
			break;

		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);

			// �T�[�o���L�����`��
			BitBlt(hdc, pos1P.x, pos1P.y, 32, 32, mdc, 0, 0, SRCCOPY);
			// �N���C�A���g���L�����`��
			BitBlt(hdc, pos2P.x, pos2P.y, 32, 32, mdc2P, 0, 0, SRCCOPY);

			wsprintf(str, "�T�[�o���FX:%d Y:%d�@�@�N���C�A���g���FX:%d Y:%d", pos1P.x, pos1P.y, pos2P.x, pos2P.y);
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
/* �ʐM�X���b�h�֐� */
DWORD WINAPI Threadfunc(void* px) {

	SOCKET sConnect;
	WORD wPort = 8000;
	SOCKADDR_IN saServer;
	int iRecv;

	// �\�P�b�g���쐬
	sConnect = socket(AF_INET, SOCK_STREAM, 0);

	if (sConnect == INVALID_SOCKET) {
		SetWindowText(hwMain, "�\�P�b�g�쐬���s");
		return 1;
	}

	// �T�[�o�̃A�h���X����ݒ�
	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = inet_addr("10.97.181.1"); // �T�[�o��IP�A�h���X���w��
	saServer.sin_port = htons(wPort);

	// �T�[�o�֐ڑ�
	if (connect(sConnect, (SOCKADDR*)&saServer, sizeof(saServer)) == SOCKET_ERROR) {
		closesocket(sConnect);
		SetWindowText(hwMain, "�T�[�o�ڑ����s");
		return 1;
	}

	SetWindowText(hwMain, "�T�[�o�ڑ�����");

	while (1)
	{
		old_pos1P = pos1P;

		// �N���C�A���g���L�����̈ʒu�����T�[�o�֑��M
		send(sConnect, (const char*)&pos2P, sizeof(POS), 0);

		// �T�[�o����L�����̈ʒu�����󂯎��
		int nRcv = recv(sConnect, (char*)&pos1P, sizeof(POS), 0);

		if (nRcv == SOCKET_ERROR) break;


		// �T�[�o�����M�������W���X�V����Ă�����ĕ`��
		if (old_pos1P.x != pos1P.x || old_pos1P.y != pos1P.y)
		{
			rect.left = old_pos1P.x - 10;
			rect.top = old_pos1P.y - 10;
			rect.right = old_pos1P.x + 42;
			rect.bottom = old_pos1P.y + 42;
			InvalidateRect(hwMain, &rect, TRUE);
		}
	}

	// �\�P�b�g�����
	shutdown(sConnect, 2);
	closesocket(sConnect);

	return 0;
}