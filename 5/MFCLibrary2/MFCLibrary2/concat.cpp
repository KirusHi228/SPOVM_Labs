#include "stdafx.h"
#include "concat.h"
#include "locale.h"
using namespace std;

DWORD WINAPI reader(LPVOID parameters) 
{
	setlocale(LC_ALL, "Russian");
	HANDLE writePipe = *(HANDLE*)parameters;

	HANDLE foundFile;
	HANDLE createdFile;
	WIN32_FIND_DATAA fileParameters;

	int positionPointer;
	const int bufSize = 100;
	char buffer[bufSize];
	char stopSymbol = '\n';

	HANDLE final;
	final = CreateEventA(NULL, TRUE, FALSE, "final");
	if (!final) 
	{
		cout << "Ошибка создания события! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	HANDLE readingAllowed;
	readingAllowed = CreateEventA(NULL, TRUE, FALSE, "readingAllowed");
	if (!readingAllowed)
	{
		cout << "Ошибка создания события разрешения чтения! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	HANDLE fileRead;
	fileRead = CreateEventA(NULL, TRUE, FALSE, "fileRead");
	if (!fileRead) 
	{
		cout << "Ошибка создания события чтения файла! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	OVERLAPPED filePtr = { 0 };
	filePtr.hEvent = fileRead;

	HANDLE writePipe;
	writePipe = CreateEventA(NULL, TRUE, FALSE, "writePipe");
	if (!writePipe)
	{
		cout << "Ошибка создания события записи! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	OVERLAPPED pipe = { 0 };
	pipe.hEvent = writePipe;

	char folderPath[MAX_PATH];												
	GetModuleFileNameA(NULL, folderPath, MAX_PATH);

	int i = MAX_PATH - 1;
	int j = 0;
  
	while (i) 
	{
		if (folderPath[i - 1] == '\\')
		{
			if (j) 
			{
				char identificator[] = "*.txt";
				for (j = 0; j < 5; j++)
					folderPath[i + j] = identificator[j];

				folderPath[i + 5] = 0;
				break;
			}
			else j++;
		}
		i--;
	}

	Sleep(100);

	HANDLE readyToWrite = OpenEventA(EVENT_ALL_ACCESS, FALSE, "readyToWrite");
	if (!readyToWrite) 
	{
		cout << "Ошибка создания события разрешения записи! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	if ((foundFile = FindFirstFileA(folderPath, &fileParameters)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!strcmp(fileParameters.cFileName, "output.txt")) continue; 
					
			createdFile = CreateFileA(fileParameters.cFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
			if (!createdFile) 
			{
				cout << "Ошибка создания файла! Завершение программы!" << endl;
				system("pause");
				return 0;
			}

			filePtr.Offset = 0;	

			do 
			{
				ReadFile(createdFile, &buffer, bufSize, NULL, &filePtr);   
				WaitForSingleObject(fileRead, INFINITE);	

				positionPointer = filePtr.InternalHigh;	
				filePtr.Offset += positionPointer;	

				WaitForSingleObject(readyToWrite, INFINITE);  
				ResetEvent(readyToWrite);

				if (!positionPointer) 
				{

					WriteFile(writePipe, &stopSymbol, 1, NULL, &pipe);	
					WaitForSingleObject(writePipe, INFINITE);	  

					SetEvent(readingAllowed);
					break;
				}

				WriteFile(writePipe, buffer, positionPointer, NULL, &pipe);	
				WaitForSingleObject(writePipe, INFINITE);	 

				SetEvent(readingAllowed);

			} while (1);

			CloseHandle(createdFile);

		} while (FindNextFileA(foundFile, &fileParameters));	

		FindClose(foundFile);
	}
	else 
	{
		cout << "Ошибка записи! В исходном каталоге отсутствуют текстовые файлы!" << endl;
	}

	WaitForSingleObject(readyToWrite, INFINITE); 

	SetEvent(final);
	SetEvent(readingAllowed);

	CloseHandle(writePipe);
	CloseHandle(final);
	CloseHandle(readingAllowed);
	CloseHandle(fileRead);
	CloseHandle(writePipe);
	CloseHandle(readyToWrite);

	return 0;
}

DWORD WINAPI writer(LPVOID parameters) 
{
	HANDLE readPipe = *(HANDLE*)parameters;	

	int positionPointer;	
	const int bufSize = 100;	
	char buffer[bufSize];																		

	HANDLE readyToWrite;
	readyToWrite = CreateEventA(NULL, TRUE, FALSE, "readyToWrite");
	if (!readyToWrite)
	{
		cout << "Ошибка создания события разрешения записи! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	HANDLE writeFile;
	writeFile = CreateEventA(NULL, TRUE, TRUE, "writeFile");
	if (!writeFile) 
	{
		cout << "Ошибка создания события записи в файл! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	OVERLAPPED filePtr = { 0 };
	filePtr.hEvent = writeFile;

  
	HANDLE pipeReader;
	pipeReader = CreateEventA(NULL, TRUE, TRUE, "pipeReader");
	if (!pipeReader) 
	{
		cout << "Ошибка создания события чтения канала! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	OVERLAPPED pipe = { 0 };
	pipe.hEvent = pipeReader;
 
	HANDLE createdFile;
	createdFile = CreateFileA("output.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	if (!createdFile) 
	{
		cout << "Ошибка создания файла записи! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	Sleep(100);

 
	HANDLE readingAllowed = OpenEventA(EVENT_ALL_ACCESS, FALSE, "readingAllowed");
	if (!readingAllowed)
	{
		cout << "Ошибка создания события разрешения записи! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	    
	HANDLE final = OpenEventA(SYNCHRONIZE, FALSE, "final");
	if (!final) 
	{
		cout << "Ошибка создания события! Завершение программы!" << endl;
		system("pause");
		return 0;
	}

	do
	{
		SetEvent(readyToWrite);															   

		WaitForSingleObject(readingAllowed, INFINITE);										    

		if (WaitForSingleObject(final, 0) == WAIT_OBJECT_0) break;				    

		ResetEvent(readingAllowed);

		ReadFile(readPipe, &buffer, bufSize, NULL, &pipe);									     

		WaitForSingleObject(pipeReader, INFINITE);										    

		positionPointer = pipe.InternalHigh;	

		WriteFile(createdFile, buffer, positionPointer, NULL, &filePtr);										  

		WaitForSingleObject(writeFile, INFINITE);										    

		filePtr.Offset += positionPointer;

	} while (1);

	CloseHandle(createdFile);
	CloseHandle(readPipe);
	CloseHandle(readyToWrite);
	CloseHandle(writeFile);
	CloseHandle(pipeReader);
	CloseHandle(readingAllowed);
	CloseHandle(final);

	return 0;
}

 
extern "C" __declspec(dllexport) void concat(void) 
{	  
	HANDLE writePipe, readPipe;
	if (!CreatePipe(&readPipe, &writePipe, NULL, 0))
	{
		cout << "Ошибка создания канала! Завершение программы!" << endl;
		system("pause");
		return;
	}

	HANDLE pipeWrite;
	pipeWrite = CreateThread(NULL, 0, &writer, &readPipe, 0, NULL);

	if (!pipeWrite) 
	{
		cout << "Ошибка создания потока записи! Завершение программы!" << endl;
		system("pause");
		return;
	}

	HANDLE hReader;
	hReader = CreateThread(NULL, 0, &reader, &writePipe, 0, NULL);

	if (!hReader)
	{
		cout << "Ошибка создания потока чтения! Завершение программы!" << endl;
		system("pause");
		TerminateThread(pipeWrite, 0);
		return;
	}

	WaitForSingleObject(pipeWrite, INFINITE);	

	CloseHandle(writePipe);
	CloseHandle(readPipe);
	CloseHandle(hReader);
	CloseHandle(pipeWrite);

	return;
}