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
		printf("�������� ����� ������:\n1-������� ������ ��������� ������\n2-������� ���� ��� ��������������\n");
		printf("3-������ ��������������\n0-����� �� ���������\n");
		printf("��� �����: ");
		while ((!scanf_s("%d", &workMode)) || (workMode < 0 || workMode > 3))
		{
			rewind(stdin);
			printf("������! ��������� ����: ");
		}

		switch (workMode)
		{

		case 1:
		{
			currentDisc = 1;
			drivesVector.clear();
			printf("\n\n��������� �����:\n\n");
			for (int i = 0; i < 256; i++)
			{
				makeDriveName(driveName, i);
				if (scanPartillions(driveName, currentDisc++, drivesVector) == 2)
				{
					printf("��������! ���� ��������!\n");
				}
			}
			currentDisc = 1;
		}
		break;

		case 2:
		{
			printf("������� ����� �����: ");
			while ((!scanf_s("%d", &temp)) || (temp < 0 || temp > 128))
			{
				rewind(stdin);
				printf("������! ����� ����� ����� ������ ������������� �������� �� 0 �� 128!\n");
				printf("��������� ����: ");
			}
			makeDriveName(driveName, temp-1);

			drive = _open(driveName, O_BINARY | O_RDONLY);
			if (drive == -1)
				printf("������! ���� �� ������!\n");
			else
			{
				printf("������ ���� %d!\n", temp);
				drive = temp-1;
			}

		}
		break;

		case 3:
		{
			foundFiles.clear();
			if (drive == -1 || currentDisc == 0)
			{
				printf("\n��� �������������� ������� ���������� �������� ������������ � ������� ����!\n\n");
				continue;
			}
			if (!scanForFiles(drive, drivesVector[drive], foundFiles))
			{
				printf("������ ��������������!\n");
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