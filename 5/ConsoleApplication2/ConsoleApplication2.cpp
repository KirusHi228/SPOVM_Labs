#include <windows.h>
#include <iostream>
using namespace std;

int main() 
{
	HMODULE loadedLibrary;

	void(*concat)(void);

	loadedLibrary = LoadLibraryA("MFCLibrary2.loadedLibrary");
	if (!loadedLibrary)
	{
		cout << "Ошибка загрузки библиотеки! Завершение программы!" << endl;
		system("pause");
		return GetLastError();
	}

	concat = (void(*)(void))GetProcAddress(loadedLibrary, "concat");
	if (!concat) 
	{
		cout << "Ошибка получения адреса функции! Завершение программы!" << endl;
		system("pause");
		return GetLastError();
	}

	concat();

	if (!FreeLibrary(loadedLibrary)) 
	{
		cout << "Ошибка освобождения библиотеки! Завершение программы!" << endl;
		system("pause");
		return GetLastError();
	}

	return 0;
}