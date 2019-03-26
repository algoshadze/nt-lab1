// Server.cpp : Определяет точку входа для приложения.
//

#include "stdafx.h"
#include "Server.h"

#define MAX_LOADSTRING 100
#define SETTINGS_FILE_NAME "settings.txt"
using namespace std;

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

int _port = 18080;


// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Settings(HWND, UINT, WPARAM, LPARAM);


BOOL writeSettings();

BOOL readSettings() {
	//Открываем файл настроек для чтения
	ifstream settingsFile("settings.txt");
	if (!settingsFile.is_open()) { //Если не существует
		//записываем настройки по умолчанию 
		writeSettings();
	}
	else {
		//Переменные для строки, ключа и значения
		string line, key, value;
		while (getline(settingsFile, line)) { //Пока еще есть несчитанные строки в файле настроек- считываем
			//Создаем потоковую переменную для считывания из строки
			istringstream ls(line);
			if (getline(ls, key, '=')) { //если удалось считать до "=" (включительно)
				if (key == "port") { // если ключ = port
					ls >> _port; //считываем значение порта
				}
			}
		}
		settingsFile.close();
	}

	return TRUE;
}
BOOL writeSettings() {
	//Открываем файл настроек для записи, обнуляя содержимое файла
	ofstream settingsFile(SETTINGS_FILE_NAME, fstream::out | fstream::trunc);
	//Записываем ключ= и значения
	settingsFile << "port=" << _port << endl;
	//Закрываем файл
	settingsFile.close();
	return TRUE;
}


struct addrinfo *_socketAddr = NULL, _hints;
SOCKET _socket = INVALID_SOCKET;
WSADATA _wsaData;

BOOL initWinsock() {
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &_wsaData);
	if (iResult != 0) {
		return FALSE;
	}
	else {
		ZeroMemory(&_hints, sizeof(_hints));
		_hints.ai_family = AF_INET;
		_hints.ai_socktype = SOCK_STREAM;
		_hints.ai_protocol = IPPROTO_TCP;
		_hints.ai_flags = AI_PASSIVE;

		// Resolve the local address and port to be used by the server
		char portStr[16];
		_itoa_s(_port, portStr, 16, 10);
		int iResult = getaddrinfo(NULL, portStr, &_hints, &_socketAddr);
		if (iResult != 0) {
			cleanupWinsock();
			return FALSE;
		}

		_socket = socket(_socketAddr->ai_family, _socketAddr->ai_socktype, _socketAddr->ai_protocol);
		if (_socket == INVALID_SOCKET) {
			cleanupWinsock();
			return FALSE;
		}

		iResult = bind(_socket, _socketAddr->ai_addr, _socketAddr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			cleanupWinsock();
			return FALSE;
		}

		if (listen(_socket, SOMAXCONN) == SOCKET_ERROR) {
			cleanupWinsock();
			return FALSE;
		}

		return TRUE;
	}
}

void cleanupWinsock() {
	if (_socket != INVALID_SOCKET) {
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}

	if (_socketAddr != NULL) {
		freeaddrinfo(_socketAddr);
		_socketAddr = NULL;
	}

	WSACleanup();
}

BOOL startServer(HWND hWnd) {
	if (!initWinsock()) {
		MessageBox(hWnd, L"Не удалось запустить сервер", L"Ошибка", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	EnableMenuItem(GetMenu(hWnd), IDM_SETTINGS, MF_DISABLED);
	EnableMenuItem(GetMenu(hWnd), IDM_START, MF_DISABLED);
	EnableMenuItem(GetMenu(hWnd), IDM_STOP, MF_ENABLED);
	return TRUE;
}

BOOL stopServer(HWND hWnd) {
	WSACleanup();
	EnableMenuItem(GetMenu(hWnd), IDM_START, MF_ENABLED);
	EnableMenuItem(GetMenu(hWnd), IDM_STOP, MF_DISABLED);
	return TRUE;
}
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Разместите код здесь.

	readSettings();


	// Инициализация глобальных строк
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_SERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Выполнить инициализацию приложения:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SERVER));

	MSG msg;

	// Цикл основного сообщения:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERVER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SERVER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			// Разобрать выбор в меню:
			switch (wmId)
			{
				case IDM_ABOUT:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					break;
				case IDM_SETTINGS:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, Settings);
					break;
				case IDM_START:
					startServer(hWnd);
					break;
				case IDM_STOP:
					stopServer(hWnd);
					break;
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			// TODO: Добавьте сюда любой код прорисовки, использующий HDC...
			EndPaint(hWnd, &ps);
		}
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}

// Обработчик сообщений для окна "Настройки".
INT_PTR CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
		case WM_INITDIALOG:
			SetDlgItemInt(hDlg, IDTB_PORT, _port, false);
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK)
			{
				BOOL success;
				int port = GetDlgItemInt(hDlg, IDTB_PORT, &success, false);

				if (!success) {
					MessageBox(hDlg, L"Введено неверное значение", L"Ошибка", MB_OK);
				}
				else {
					_port = port;
					writeSettings();
					EndDialog(hDlg, LOWORD(wParam));
					return (INT_PTR)TRUE;
				}
			}
			else if (LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}
