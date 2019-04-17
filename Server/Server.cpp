// Server.cpp : Определяет точку входа для приложения.
//

#include "stdafx.h"
#include "Server.h"

#define MAX_LOADSTRING 100
#define SETTINGS_FILE_NAME "settings.txt"
#define WSA_ACCEPT (WM_USER + 1)
#define WSA_EVENT (WM_USER + 2)



using namespace std;

struct ItemData
{
	int code;
	int price;
	WCHAR name[100];
	ItemData(int code, int price, const WCHAR* name)
	{
		this->code = code;
		this->price = price;
		lstrcpynW(this->name, name, 100);
	}
};

vector<ItemData> items;

void readData() {
	items.clear();
	items.push_back(ItemData(10, 100, L"Минеральная вода"));
	items.push_back(ItemData(3, 50, L"Шоколад \"Аленка\""));
	items.push_back(ItemData(18, 400, L"Свинина"));

}

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна
HWND _hAppWnd;

int _port = 18080;


// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Settings(HWND, UINT, WPARAM, LPARAM);
void sendResponse(SOCKET socket, wstring message);


static wchar_t* charToWChar(const char* text)
{
	const size_t size = strlen(text) + 1;
	wchar_t* wText = new wchar_t[size];
	size_t converted;
	mbstowcs_s(&converted, wText, size, text, size);
	return wText;
}

BOOL writeSettings();

BOOL readSettings() {
	//Открываем файл настроек для чтения
	ifstream settingsFile(SETTINGS_FILE_NAME);
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

		iResult = bind(_socket, _socketAddr->ai_addr, (int)_socketAddr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			cleanupWinsock();
			return FALSE;
		}

		if (listen(_socket, SOMAXCONN) == SOCKET_ERROR) {
			cleanupWinsock();
			return FALSE;
		}
	}
	return TRUE;
}



BOOL startServer() {
	if (!initWinsock()) {
		MessageBox(_hAppWnd, L"Не удалось запустить сервер", L"Ошибка", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	int iResult = WSAAsyncSelect(_socket, _hAppWnd, WSA_ACCEPT, FD_ACCEPT);
	if (iResult > 0) {
		MessageBox(_hAppWnd, L"Ошибка WSASyncSelect", NULL, MB_OK | MB_ICONERROR);
		return FALSE;
	}
	HMENU hAppMenu = GetMenu(_hAppWnd);
	EnableMenuItem(hAppMenu, IDM_SETTINGS, MF_DISABLED);
	EnableMenuItem(hAppMenu, IDM_START, MF_DISABLED);
	EnableMenuItem(hAppMenu, IDM_STOP, MF_ENABLED);
	return TRUE;
}

BOOL stopServer() {
	WSACleanup();
	HMENU hAppMenu = GetMenu(_hAppWnd);
	EnableMenuItem(hAppMenu, IDM_SETTINGS, MF_ENABLED);
	EnableMenuItem(hAppMenu, IDM_START, MF_ENABLED);
	EnableMenuItem(hAppMenu, IDM_STOP, MF_DISABLED);
	return TRUE;
}

void onWsaAccept(WORD selectError) {
	if (selectError != NULL) {
		return;
	}
	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);
	SOCKET clientSocket = accept(_socket, (LPSOCKADDR)&addr, &addrlen);
	if (clientSocket == INVALID_SOCKET) {
		return;
	}

	int iResult = WSAAsyncSelect(clientSocket, _hAppWnd, WSA_EVENT, FD_READ | FD_CLOSE);
	if (iResult > 0) {

	}

	OutputDebugString(L"Клиент подключен\r\n");
}

void onWsaEvent(SOCKET socket, WORD event, WORD selectError) {
	if (selectError != 0) {
		return;
	}

	switch (event) {
		case FD_READ:
		{
			wstring message;
			char buf[256];
			int iRes;
			do {
				iRes = recv(socket, buf, 254, 0);
				if (iRes > 0) {
					buf[iRes] = 0;
					buf[iRes+1] = 0;
					//WCHAR *text = charToWChar(buf);
					WCHAR *text = (WCHAR*)buf;
					message.append(text);
					//delete[] text;
				}
				else if (iRes == 0) {
					OutputDebugString(L"Соединение закрыто\r\n");
				}
				else {
					OutputDebugString(L"Получен запрос\r\n");
					OutputDebugString(message.c_str());
					OutputDebugString(L"\r\n");
					wstringstream ms(message);
					wstring command;
					wstringstream responseStream;

					if (message.size() > 1 && getline(ms, command, L'|')) {
						if (command == L"1") {
							responseStream << L"1|";
							for (int i = 0; i < items.size(); i++) {
								responseStream << items[i].code << L"|" << items[i].name << L"|" << items[i].price << L"|";
							}
							sendResponse(socket, responseStream.str());
							return;
						}
						else if (command == L"2") {
							wstring codeStr, priceStr;
							int code, price;

							if (!getline(ms, codeStr, L'|') || !getline(ms, priceStr, L'|')) {
								responseStream << L"0|Ошибка в формате команды изменения цены: неверное количество аргументов.";
								sendResponse(socket, responseStream.str());
								return;
							}
							try {
								code = stoi(codeStr);
								price = stoi(priceStr);
							}
							catch (const std::invalid_argument& ia) {
								responseStream << L"0|Ошибка в формате команды изменения цены: неверный тип аргументов. " << ia.what();
								sendResponse(socket, responseStream.str());
								return;
							}
							for (int i = 0; i < items.size(); i++) {
								if (items[i].code == code) {
									items[i].price = price;
									sendResponse(socket, L"2|");
									return;
								}
							}
							sendResponse(socket, L"0|Ошибка изменения цены: не найден код товара.");
						}
						else {
							sendResponse(socket, L"Ошибка: 0|Неизвестный код команды");
						}
					}
					else {
						sendResponse(socket, L"0|Ошибка протокола - неверный формат команды");
					}

				}
			} while (iRes > 0);
		}
		break;
		case FD_CLOSE:
		{
			OutputDebugString(L"Соединение закрыто\r\n");
			closesocket(socket);
		}
		break;
	}

}


void sendResponseUTF8(SOCKET socket, wstring message) {
	message.append(L"\r\n");
	wstring_convert<codecvt_utf8<wchar_t>> conv;
	string u8str = conv.to_bytes(message);
	//int size = (int)message.size()*sizeof(WCHAR);
	int size = (int)u8str.size();
	//const char* data = (const char*)message.c_str();
	const char* data = u8str.c_str();
	send(socket, data, size, 0);
}

void sendResponseUTF16(SOCKET socket, wstring message) {
	message.append(L"\n");
	int size = (int)message.size()*sizeof(WCHAR);
	const char* data = (const char*)message.c_str();
	send(socket, data, size, 0);
}



void sendResponse(SOCKET socket, wstring message) {
	OutputDebugString(L"Отправляем ответ\r\n");
	OutputDebugString(message.c_str());
	OutputDebugString(L"\r\n");
	sendResponseUTF16(socket, message);
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
		case WM_CREATE:
			_hAppWnd = hWnd;
			readData();
			break;
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
					startServer();
					break;
				case IDM_STOP:
					stopServer();
					break;
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
		case WSA_ACCEPT:
		{
			onWsaAccept(WSAGETSELECTERROR(lParam));
		}
		break;
		case WSA_EVENT:
		{
			onWsaEvent((SOCKET)wParam, WSAGETSELECTEVENT(lParam), WSAGETSELECTERROR(lParam));
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
