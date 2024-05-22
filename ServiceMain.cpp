#define _CRT_SECURE_NO_WARNINGS
#include "ServiceMain.h"

#define FILE_DIRECTORY "C:\\"
#define LOG_FILE_PATH "C:\\logfile.log"

#define PATH_SIZE 64
#define BUF_SIZE 256

HANDLE hFileMapping;
LPVOID lpFileMap;
const char* fileShareName = "Global\\fileMap";

HANDLE hEventSend;
HANDLE hEventRecv;
HANDLE hEventTermination;
HANDLE hEvents[2];
CHAR lpEventSendName[] = "Global\\lpEventSendName";
CHAR lpEventRecvName[] = "Global\\lpEventRecvName";
int digitsCount = 0;

int AddLogMessage(const char* message, int errorCode)
{
	HANDLE hFile;
	DWORD bytesWritten;
	static bool isFirstLog = true;
	char buffer[BUF_SIZE] = { 0 };

	DWORD dwCreationDistribution;

	if (isFirstLog)
	{
		dwCreationDistribution = CREATE_ALWAYS;
		isFirstLog = false;
	}
	else
		dwCreationDistribution = OPEN_ALWAYS;

	hFile = CreateFile(LOG_FILE_PATH, GENERIC_WRITE, 0, NULL, dwCreationDistribution, FILE_FLAG_WRITE_THROUGH, NULL);

	if (!hFile)
		return -1;

	SetFilePointer(hFile, 0, NULL, FILE_END);

	if (errorCode == 0)
		sprintf(buffer, "%s\n", message);
	else
		sprintf(buffer, "%s, code:%d\n", message, errorCode);

	WriteFile(hFile, buffer, strlen(buffer), &bytesWritten, NULL);
	CloseHandle(hFile);
	return 0;
}

int CountSymbolInString(CHAR* str, char symbol)
{
	int n = 0;
	for (int i = 0; i < strlen(str); i++) {
		if (str[i] == symbol)
			n++;
	}
	return n;
}

void AddFileNameToPath(char* path, LPTSTR fileName)
{
	for (int i = 0; i < PATH_SIZE; i++)
		path[i] = '\0';

	if (CountSymbolInString(fileName, '\\') > 0)
	{
		strcat(path, fileName);
	}
	else
	{
		strcat(path, FILE_DIRECTORY);
		strcat(path, fileName);
	}
}

char* CreateNewStringFromBuffer(CHAR* buffer, int replacesMaxCount)
{
	char* newStr = (char*)calloc(strlen(buffer) + 1, sizeof(CHAR));

	int length = strlen(buffer);
	for (int i = 0; i < length - 1; i++)
	{
		newStr[i] = buffer[i];

		if (buffer[i] >= 48 && buffer[i]<=57 && digitsCount < replacesMaxCount)
		{
			newStr[i] = ' ';
			digitsCount++;
		}

	}

	for (int i = 0; i < BUF_SIZE; i++)
		buffer[i] = 0;

	return newStr;
}

int WorkWithFiles(char* path, LPTSTR fileName, int replacesMaxCount)
{
	if (strlen(fileName) == 0 || replacesMaxCount == -1)
		return -1;

	HANDLE hIn, hOut;
	DWORD readBytesCount, writeBytesCount;
	CHAR buffer[BUF_SIZE] = { 0 };

	hIn = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hIn == INVALID_HANDLE_VALUE)
	{
		AddLogMessage("The input file can't be opened", GetLastError());
		return -2;
	}

	strtok(fileName, ".");
	strcat(fileName, ".out");
	AddFileNameToPath(path, fileName);

	hOut = CreateFile(path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hOut == INVALID_HANDLE_VALUE)
	{
		AddLogMessage("The output file can't be opened", GetLastError());
		return -3;
	}

	while (ReadFile(hIn, buffer, BUF_SIZE - 1, &readBytesCount, NULL) && readBytesCount > 0)
	{
		char* newStr = CreateNewStringFromBuffer(buffer, replacesMaxCount);
		readBytesCount = strlen(newStr) * sizeof(CHAR);

		WriteFile(hOut, newStr, readBytesCount, &writeBytesCount, NULL);

		if (readBytesCount != writeBytesCount)
		{
			AddLogMessage("Fatal recording error", GetLastError());
			return -1;
		}
	}
	CloseHandle(hIn);
	CloseHandle(hOut);
	return 0;
}

bool IsDigit(char symbol)
{
	return 48 <= symbol && symbol <= 57;
}

bool IsDigit(char* str)
{
	for (int i = 0; i < strlen(str); i++)
	{
		if (!IsDigit(str[i]))
			return false;
	}
	return true;
}

int ServiceStart()
{
	char message[80] = { 0 };
	DWORD res;

	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;
	AddLogMessage("Creating security attributes ALL ACCESS for EVERYONE!!!\n");
	// Создаем дескриптор безопасности
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	// DACL не установлен (FALSE) - объект незащищен
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	// Настраиваем атрибуты безопасности, передавая туда указатель на дескриптор безопасности sd и создаем объект-событие
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor= &sd;
	sa.bInheritHandle = false;
	// проверяем структуру дескриптора безопасности
	if (!IsValidSecurityDescriptor(&sd))
		{
			res = GetLastError();
			AddLogMessage("Security descriptor is invalid.\n");
			sprintf(message, "The last error code: %u\n", res);
			return -(int)res;
		}
	// устанавливаем новый дескриптор безопасности
	hEventSend = CreateEvent( &sa, FALSE, FALSE, lpEventSendName);
	hEventRecv = CreateEvent(&sa, FALSE, FALSE, lpEventRecvName);
	
	// Создаем объект-отображение, файл не создаем
	hFileMapping = CreateFileMapping((HANDLE)-1,
		&sa, PAGE_READWRITE, 0, BUF_SIZE, fileShareName);

	if (hFileMapping == NULL)
	{
		AddLogMessage("Error CreateFileMapping", GetLastError());
		return -1;
	}

	SetSecurityInfo(hFileMapping, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL);

	lpFileMap = MapViewOfFile(hFileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, BUF_SIZE);

	if (lpFileMap == 0)
	{
		AddLogMessage("Error MapViewOfFile", GetLastError());
		return -1;
	}
	return 0;
}



char* SumStrings(const char* str1, const char* str2)
{
	int length = strlen(str1) + strlen(str2);
	char* newStr = (char*)calloc(length, sizeof(CHAR));
	strcat(newStr, str1);
	strcat(newStr, str2);
	return newStr;
}

void SetError(char* errorBuf, const char* error)
{
	if (strlen(errorBuf) != 0)
		return;

	strcpy(errorBuf, error);
	AddLogMessage(error);
}

int WorkWithClient()
{
	DWORD dwRetCode;
	char buffer[BUF_SIZE] = { 0 };
	char errorBuffer[BUF_SIZE] = { 0 };
	digitsCount = 0;

	const char* messagesToClient[2] = { "Send file name", "Send replaces max count" };

	char fileName[BUF_SIZE] = { 0 };
	int replacesMaxCount = -1;
	int iteraction = 0;

	int timeout = INFINITE;


	WaitForSingleObject(hEventSend, timeout);

	while (iteraction < 2)
	{
		strcpy((char*)lpFileMap, messagesToClient[iteraction]);
		AddLogMessage(SumStrings("Message sent to client: ", messagesToClient[iteraction]));
		// Устанавливаем объект-событие в отмеченное состояние
		SetEvent(hEventRecv);
		// ждем ответа
		dwRetCode = WaitForSingleObject(hEventSend, INFINITE);
		// если ответ получен - выводим, если ошибка - выходим
		if (dwRetCode == WAIT_OBJECT_0) puts(((LPSTR)lpFileMap));
		if (dwRetCode == WAIT_ABANDONED_0 || dwRetCode == WAIT_FAILED)
		{
			printf("\nError waiting responce!\n)");
			//break;
		}
		strcpy(buffer, (char*)lpFileMap);
		AddLogMessage(SumStrings("Message read from client: ", buffer));

		if (strcmp(buffer, "stop service") == 0)
			return -2;

		if (strlen(buffer) == 0)
		{
			SetError(errorBuffer, "Missed argument");
			SetEvent(hEventRecv);
		}

		if (iteraction == 0)
			strcpy(fileName, buffer);

		if (iteraction == 1)
		{
			if (!IsDigit(buffer))
				SetError(errorBuffer, "Invalid argument");
			else
				replacesMaxCount = atoi(buffer);
		}
		iteraction++;
	}

	char path[PATH_SIZE] = { 0 };
	AddFileNameToPath(path, fileName);

	if (WorkWithFiles(path, fileName, replacesMaxCount) != 0)
		SetError(errorBuffer, "Error work with file, see logfile");

	if (strlen(errorBuffer) > 0)
		strcpy(buffer, errorBuffer);
	else
		_itoa_s(digitsCount, buffer, 10);

	strcpy((char*)lpFileMap, buffer);
	AddLogMessage("Message with result sent to client: %d", digitsCount);
	SetEvent(hEventRecv);

	return (strlen(errorBuffer) > 0 ? -1 : 0);
}





void ServiceStop()
{
	CloseHandle(hEventSend);
	CloseHandle(hEventRecv);
	// Отменяем отображение файла
	UnmapViewOfFile(lpFileMap);
	// Освобождаем идентификатор созданного объекта-отображения
	CloseHandle(hFileMapping);
	AddLogMessage("All Kernel objects closed!");
	AddLogMessage("Service stopped!");
}
