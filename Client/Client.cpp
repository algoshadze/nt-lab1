// Client.cpp : Определяет точку входа для приложения.
//

#include "stdafx.h"
#include "Client.h"

using namespace std;

#define MAX_LOADSTRING 100
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
// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна
int _port = 18080;
IN_ADDR _address = { 127, 0, 0, 1 };
struct addrinfo *_socketAddr = NULL, _hints;
SOCKET _socket = INVALID_SOCKET;
WSADATA _wsaData;


// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    PriceChange(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Settings(HWND, UINT, WPARAM, LPARAM);

vector<ItemData> items;

void InitData() {
	items.clear();
	items.push_back(ItemData(10, 100, L"Минеральная вода"));
	items.push_back(ItemData(3, 50, L"Шоколад \"Аленка\""));
	items.push_back(ItemData(18, 400, L"Свинина"));

}



int InitWinsock() {
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &_wsaData);
	if (iResult == 0) {
		return 1;
	}
	else {
		return 0;
	}
}

BOOL onConnect() {
	int iResult;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	// Resolve the server address and port
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo("localhost", "9432", &hints, &result);
	if (iResult != 0) {
		wstring err(L"getaddrinfo failed with error: ");
		err.append(to_wstring(iResult));
		OutputDebugString(err.c_str());
		WSACleanup();
		return FALSE;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		_socket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (_socket == INVALID_SOCKET) {
			wstring err(L"socket failed with error: ");
			err.append(to_wstring(WSAGetLastError()));
			OutputDebugString(err.c_str());
			continue;
		}

		// Connect to server.
		iResult = connect(_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(_socket);
			_socket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (_socket == INVALID_SOCKET) {
		WSACleanup();
		return FALSE;
	}

	ULONG iMode;
	iResult = ioctlsocket(_socket, FIONBIO, &iMode);

	return TRUE;
}

void onDisconnect() {
	closesocket(_socket);
}

void sendRquest(wstring message) {
	int size = (int)message.size() * sizeof(WCHAR);
	const char* data = (const char*)message.c_str();
	send(_socket, data, size, 0);
}

BOOL receiveResponse(wstring &responceCode, vector<wstring> &responseFields) {

	responseFields.clear();

	wstringstream responseStream;
	char buf[256];
	int iRes;

	do {
		iRes = recv(_socket, buf, 254, 0);
		if (iRes > 0) {
			buf[iRes] = 0;
			buf[iRes+1] = 0;
			responseStream << (WCHAR*)buf;
		}
		else if (iRes == 0) {
			responseFields.push_back(L"Соединение закрыто сервером.");
			return FALSE;
		}
		else {
			responseStream.seekg(0, responseStream.beg);

			if (!getline(responseStream, responceCode, L'|')) {
				responseFields.push_back(L"Ошибка протокола: не удалось считать код ответа");
				return FALSE;
			}

			wstring field;
			while (getline(responseStream, field, L'|')) {
				responseFields.push_back(field);
			}
			return TRUE;
		}
	} while (iRes > 0);
}

BOOL onGetItemsList() {
	wstring msg(L"1|");
	sendRquest(msg);

	wstring responseCode;
	vector<wstring> responseFields;
 	if (!receiveResponse(responseCode, responseFields) || responseCode == L"0") {
		MessageBox(NULL, responseFields[0].c_str(), L"Ошибка получения данных от сервера", MB_ICONERROR | MB_OK);
		return FALSE;
	}

	if (responseCode != L"1") {
		MessageBox(NULL, L"Неверный код ответа - ожидалось значение '1'", L"Ошибка получения данных от сервера", MB_ICONERROR | MB_OK);
		return FALSE;
	}

	if (responseFields.size() % 3 != 0)
	{
		MessageBox(NULL, L"Неверное количество полей в списке товаров. Ожидалось кратное 3.", L"Ошибка операции", MB_ICONERROR | MB_OK);
		return FALSE;
	}

	items.clear();
	for (int i = 0; i < responseFields.size(); i += 3) {
		int code, price;
		wstring name;
		int itemIndex = i / 3;
		try {
			ItemData itemData(stoi(responseFields[i]), stoi(responseFields[i + 2]), responseFields[i + 1].c_str());
			items.push_back(itemData);
		}
		catch (const std::invalid_argument& ia) {
			MessageBox(NULL, L"Неверный формат кода или цены. Ожидалось число.", L"Ошибка операции", MB_ICONERROR | MB_OK);
			return FALSE;
		}
		catch (...) {
			MessageBox(NULL, L"Неизвестная ошибка.", L"Ошибка операции", MB_ICONERROR | MB_OK);
			return FALSE;
		}
	}

	return TRUE;
}

void displayItems(const WCHAR* message) {
	wstringstream itemsTextStream;
	for (int i = 0; i < items.size(); i++) {
		itemsTextStream << items[i].name << L" " << items[i].price << L'\n';
	}

	MessageBox(NULL, itemsTextStream.str().c_str(), message, MB_ICONINFORMATION || MB_OK);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Разместите код здесь.

	// Инициализация глобальных строк
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Выполнить инициализацию приложения:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));

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
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CLIENT);
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
			if (!InitWinsock()) {
				MessageBox(NULL, L"Не удалось инициализировать Winsock2.", L"Ошибка", MB_OK | MB_ICONERROR);
				return FALSE;
			}

			InitData();
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
				case IDM_CONNECT:
				{
					onConnect();
				}
				break;
				case IDM_GET_ITEM_LIST:
				{
					if (onGetItemsList()) {
						displayItems(L"Получен список товаров от сервера.");
					}
				}
				break;
				case IDM_PRICECHANGE:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_PRICE_CHANGE), hWnd, PriceChange);
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
			WSACleanup();
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
// Обработчик сообщений для окна "Изменение цены".
INT_PTR CALLBACK PriceChange(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
		{

			//Получаем дескриптор комбобокса
			HWND hItemsCombo = GetDlgItem(hDlg, IDCB_ITEMS);
			HWND hPriceText = GetDlgItem(hDlg, IDT_PRICE);
			for (int i = 0; i < items.size(); i++) {
				SendMessage(hItemsCombo, CB_ADDSTRING, 0, (LPARAM)(items[i].name));
			}

			//Выделяем первый элемент
			SendMessage(hItemsCombo, CB_SETCURSEL, (WPARAM)0, 0);
			SetDlgItemInt(hDlg, IDT_PRICE, items[0].price, false);
			SetDlgItemInt(hDlg, IDTB_NEW_PRICE, items[0].price, false);

		}
		return (INT_PTR)TRUE;

		case WM_COMMAND:
		{
			//Полчаем дескриптор комбобокса для проверки источника сообщения
			HWND hItemsCombo = GetDlgItem(hDlg, IDCB_ITEMS);
			HWND hPriceText = GetDlgItem(hDlg, IDT_PRICE);
			if ((HWND)lParam == hItemsCombo) //Это сообщение от комбо
			{
				if (HIWORD(wParam) == CBN_SELCHANGE)  //Это сообщение об изменении выбранного товара
				{
					//Берем выбранный индекс
					int itemIndex = (int)SendMessage(hItemsCombo, CB_GETCURSEL,
						(WPARAM)0, (LPARAM)0);
					SetDlgItemInt(hDlg, IDT_PRICE, items[itemIndex].price, false);
					SetDlgItemInt(hDlg, IDTB_NEW_PRICE, items[itemIndex].price, false);
					//Для примера выводим в сообщении
					//wstring msg(L"Выбран элемент с индексом ");
					//msg = msg.append(to_wstring(itemIndex));
					//MessageBox(hDlg, msg.c_str(), L"Информация", MB_OK);
				}
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == IDOK)
			{
				int itemIndex = (int)SendMessage(hItemsCombo, CB_GETCURSEL,
					(WPARAM)0, (LPARAM)0);
				BOOL success;
				int new_price = GetDlgItemInt(hDlg, IDTB_NEW_PRICE, &success, false);
				if (!success) {
					MessageBox(hDlg, L"Введено неправильное значение цены", L"Ошибка", MB_OK);
				}
				else {
					items[itemIndex].price = new_price;
					EndDialog(hDlg, LOWORD(wParam));
				}

				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
		}
		break;
	}
	return (INT_PTR)FALSE;
}
// Обработчик сообщений для окна "Настройки".
INT_PTR CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	WCHAR addressText[16];

	switch (message)
	{
		case WM_INITDIALOG:
			SetDlgItemInt(hDlg, IDTB_PORT, _port, false);
			InetNtopW(AF_INET, &_address, addressText, 16);
			SetDlgItemText(hDlg, IDTB_ADRESS, addressText);
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK)
			{
				GetDlgItemText(hDlg, IDTB_ADRESS, addressText, 16);
				if (InetPtonW(AF_INET, addressText, &_address) != 1) {
					MessageBox(hDlg, L"Введено неверное значение адреса", L"Ошибка", MB_OK);
				}
				else {
					BOOL success;
					int port = GetDlgItemInt(hDlg, IDTB_PORT, &success, false);
					if (!success) {
						MessageBox(hDlg, L"Введено неверное значение порта", L"Ошибка", MB_OK);
					}
					else {
						_port = port;
						EndDialog(hDlg, LOWORD(wParam));
						return (INT_PTR)TRUE;
					}
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
