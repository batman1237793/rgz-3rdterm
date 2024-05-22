#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#define BUF_SIZE 256
// Идентификаторы объектов-событий, которые используются
// для синхронизации, область видимости - сеанс клиента
HANDLE hEventSend;
HANDLE hEventRecv;
// Имя объектов-событий для синхронизации записи и чтения из отображаемого файла
CHAR lpEventSendName[] = "Global\\lpEventSendName";
CHAR lpEventRecvName[] = "Global\\lpEventRecvName";
// Имя отображния файла на память
const char* fileShareName = "Global\\fileMap";
// Идентификатор отображения файла на память
HANDLE hFileMapping;
// Указатель на отображенную область памяти
LPVOID lpFileMap;



void OutputError(const char* error)
{
	int x = GetLastError();
	if (x == 0)
		x = -1;

	printf("%s, code:%d\n", error, x);

	char a;
	scanf("%c", &a);

	exit(-1);
}


int UseService()
{
	DWORD bytesRead, bytesWritten;
	char buffer[BUF_SIZE] = { 0 };

	printf("Print \"stop service\" to stop service and exit\n\n");
	SetEvent(hEventSend);
	int iteraction = 0;
	int timeout = INFINITE;
	while (iteraction < 2)
	{
		
		WaitForSingleObject(hEventRecv, timeout);
		strcpy(buffer, (char*)lpFileMap);
		printf("Service message:%s\n", buffer);

		printf("Client answer:");
		fgets(buffer, BUF_SIZE - 1, stdin);
		buffer[strlen(buffer) - 1] = 0; // to remove \n

		strcpy((char*)lpFileMap, buffer);
		SetEvent(hEventSend);


		if (strcmp(buffer, "stop service") == 0)
			return -1;

		for (int i = 0; i < BUF_SIZE; i++)
			buffer[i] = 0;

		iteraction++;

	}

	WaitForSingleObject(hEventRecv, INFINITE);

	strcpy(buffer, (char*)lpFileMap);
	if (strcmp(buffer, "Error work with file, see logfile")==0)
		printf("Service work result: %s \n\n", buffer);
	else
		printf("Service work result: %s replaces complete\n\n", buffer);

	return 0;
}

int main()
{
	// Открываем объекты-события для синхронизации  чтения и записи
	hEventSend = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpEventSendName);
	hEventRecv = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpEventRecvName);
	if (hEventSend == NULL || hEventRecv == NULL)
	{
		fprintf(stdout, "OpenEvent: Error %ld\n",
			GetLastError());
		_getch();
		return-1;
	}
	// Открываем объект-отображение
	hFileMapping = OpenFileMapping(
		FILE_MAP_READ | FILE_MAP_WRITE, FALSE, fileShareName);
	// Если открыть не удалось, выводим код ошибки
	if (hFileMapping == NULL)
	{
		fprintf(stdout, "OpenFileMapping: Error %ld\n",
			GetLastError());
		_getch();
		return -2;
	}
	// Выполняем отображение файла на память.
	// В переменную lpFileMap будет записан указатель на отображаемую область памяти
	lpFileMap = MapViewOfFile(hFileMapping,	FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	// Если выполнить отображение не удалось, выводим код ошибки
	if (lpFileMap == 0)
	{
		fprintf(stdout, "MapViewOfFile: Error %ld\n",
			GetLastError());
		_getch();
		return -3;
	}

	UseService();

	CloseHandle(hEventSend);
	CloseHandle(hEventRecv);
	// Отменяем отображение файла
	UnmapViewOfFile(lpFileMap);
	// Освобождаем идентификатор созданного объекта-отображения
	CloseHandle(hFileMapping);

	char a;
	scanf("%c", &a);

	return 0;
}
