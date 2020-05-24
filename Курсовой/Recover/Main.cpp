#include "recover.h"
#include <io.h>
#include <fcntl.h>
const int NotUsed = system("color f0");

void main()
{
	setlocale(LC_ALL, "Russian");
	int workMode, temp, drive = -1;
	char driveName[23];
	std::vector<DRIVEPACKET*> drivesVector;
	std::vector<deletedItem*> foundFiles;
	int currentDisc = 0;
	int marker = 0;

	while (1)
	{
		rewind(stdin);
		printf("Выберите режим работы:\n1-вывести список доступных дисков\n2-выбрать диск для восстановления\n");
		printf("3-начать восстановление\n0-выход из программы\n");
		printf("Ваш выбор: ");
		while ((!scanf_s("%d", &workMode)) || (workMode < 0 || workMode > 3))
		{
			rewind(stdin);
			printf("Ошибка! Повторите ввод: ");
		}

		switch (workMode)
		{

		case 1:
		{
			currentDisc = 1;
			drivesVector.clear();
			printf("\n\nНайденные диски:\n\n");
			for (int i = 0; i < 256; i++)
			{
				makeDriveName(driveName, i);
				if (scanPartillions(driveName, currentDisc++, drivesVector) == 2)
				{
					printf("Внимание! Диск повреждён!\n");
				}
			}
			currentDisc = 1;
		}
		break;

		case 2:
		{
			printf("Введите номер диска: ");
			while ((!scanf_s("%d", &temp)) || (temp < 0 || temp > 128))
			{
				rewind(stdin);
				printf("Ошибка! Номер может иметь только целочисленное значение от 0 до 128!\n");
				printf("Повторите ввод: ");
			}
			makeDriveName(driveName, temp-1);

			drive = _open(driveName, O_BINARY | O_RDONLY);
			if (drive == -1)
				printf("Ошибка! Диск не найден!\n");
			else
			{
				printf("Выбран диск %d!\n", temp);
				drive = temp-1;
			}

		}
		break;

		case 3:
		{
			foundFiles.clear();
			if (drive == -1 || currentDisc == 0)
			{
				printf("\nДля восстановления сначала необходимо провести сканирование и выбрать диск!\n\n");
				continue;
			}
			if (!scanForFiles(drive, drivesVector[drive], foundFiles))
			{
				printf("Ошибка восстановления!\n");
			}

			saveRecoveredFiles(foundFiles);

		}
		break;

		case 0:
		{
			system("pause");
			return;
		}
		break;

		default:
			continue;
		};

		printf("\n\n");
	}
}