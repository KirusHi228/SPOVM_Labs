#include "Recover.h"
#include <ShlObj.h>
#include <iostream>
#include <direct.h>
const char driveNameTemplate[] = "\\\\.\\PhysicalDrive%d";

void makeDriveName(char* driveName, int i)
{
	sprintf(driveName, driveNameTemplate, i);
}

void addDrive(std::vector<DRIVEPACKET*> &drivesVector, DRIVEPACKET *pstDrive)
{
	DRIVEPACKET *pDrive = new DRIVEPACKET;
	memcpy(pDrive, pstDrive, sizeof(DRIVEPACKET));
	drivesVector.push_back(pDrive);
}

LPTSTR setDirectory(LPTSTR &Default)
{
	char* szPath = new char[MAX_PATH];
	while (1)
	{
		printf("�������� ����� ��� ��������������� ������. ��� ������ �������� ����� ������� 0\n");
		rewind(stdin);
		std::cin >> szPath;
		if (strcmp((char*)szPath, "0") == 0)
		{
			printf("������ �������� ����: %s\n\n", Default);
			return Default;
		}
		else
		{
			if (_chdir(szPath) == -1)
				printf("������ �������� � ����������! ��������� ����!\n");
			else
				return (LPTSTR)szPath;
		}
	}
}

int scanPartillions(char* drive, int currentDisc, std::vector<DRIVEPACKET*> &drivesVector)
{
	int i, retCode;
	DWORD dwBytes;

	PARTITION *PartitionTbl;
	DRIVEPACKET stDrive;
	char szSector[512];
	WORD wDrive = 0;
	char szTmpStr[64];
	DWORD dwMainPrevRelSector = 0;
	DWORD dwPrevRelSector = 0;

	HANDLE hDrive = CreateFileA(drive, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hDrive == INVALID_HANDLE_VALUE)
		return 0;

	retCode = ReadFile(hDrive, szSector, 512, &dwBytes, 0);
	if (!retCode)
		return 2;

	dwPrevRelSector = 0;
	dwMainPrevRelSector = 0;

	PartitionTbl = (PARTITION*)(szSector + 0x1BE);

	for (i = 0; i < 4; i++)
	{
		stDrive.wCylinder = PartitionTbl->chCylinder;
		stDrive.wHead = PartitionTbl->chHead;
		stDrive.wSector = PartitionTbl->chSector;
		stDrive.dwNumSectors = PartitionTbl->dwNumberSectors;
		stDrive.wType = ((PartitionTbl->chType == PART_EXTENDED) || (PartitionTbl->chType == PART_DOSX13X)) ? EXTENDED_PART : BOOT_RECORD;

		if ((PartitionTbl->chType == PART_EXTENDED) || (PartitionTbl->chType == PART_DOSX13X))
		{
			dwMainPrevRelSector = PartitionTbl->dwRelativeSector;
			stDrive.dwNTRelativeSector = dwMainPrevRelSector;
		}
		else
		{
			stDrive.dwNTRelativeSector = dwMainPrevRelSector + PartitionTbl->dwRelativeSector;
		}

		if (stDrive.wType == EXTENDED_PART)
			break;

		if (PartitionTbl->chType == 0)
			break;

		switch (PartitionTbl->chType)
		{
		case PART_DOS2_FAT:
			strcpy(szTmpStr, "FAT12");
			break;
		case PART_DOSX13:
		case PART_DOS4_FAT:
		case PART_DOS3_FAT:
			strcpy(szTmpStr, "FAT16");
			break;
		case PART_DOS32X:
		case PART_DOS32:
			strcpy(szTmpStr, "FAT32");
			break;
		case 7:
			strcpy(szTmpStr, "NTFS");
			break;
		default:
			strcpy(szTmpStr, "Unknown");
			break;
		}

		printf("%d) %s Drive %d\n", currentDisc, szTmpStr, wDrive);

		addDrive(drivesVector, &stDrive);
		PartitionTbl++;
		wDrive++;
	}
	CloseHandle(hDrive);
	return 1;
}

int scanForFiles(int drive, DRIVEPACKET* currentDisc, std::vector<deletedItem*> &foundFiles)
{
	NTFSDrive::ST_FILEINFO stFInfo;
	DWORD dwDeleted = 0, dwBytes = 0, i = 0;
	int retCode;

	if (!currentDisc)
		return 0;

	char name[20];
	makeDriveName(name, drive);
	m_hDrive = CreateFile(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, NULL);
	if (m_hDrive == INVALID_HANDLE_VALUE)
	{
		printf("������ �������� �����!");
		CloseHandle(m_hDrive);
		return 0;
	}

	m_cNTFS.setDriveHandle(m_hDrive); 
	m_cNTFS.SetStartSector(currentDisc->dwNTRelativeSector, 512);

	retCode = m_cNTFS.Initialize(); 
	if (retCode)
	{
		printf("������ ������ �������� �������!");
		CloseHandle(m_hDrive);
		m_hDrive = INVALID_HANDLE_VALUE;
		return 0;
	}
	printf("��������� �����: \n");

	for (i = 0; (i<0xFFFFFFFF); i++) 
	{
		retCode = m_cNTFS.GetFileDetail(i + 30, stFInfo);
		if ((retCode == ERROR_NO_MORE_FILES) || (retCode == ERROR_INVALID_PARAMETER))
			if (dwDeleted == 0)
			{
				printf("�������� ������ �� �������! \n");
				return 2;
			}
			else
				return 1;

		if (retCode)
		{
			printf("������ ������ �����!");
			CloseHandle(m_hDrive);
			m_hDrive = INVALID_HANDLE_VALUE;
			return 0;
		}

		if (!stFInfo.bDeleted)
		{
			continue;
		}
		dwDeleted++;

		deletedItem *element = new deletedItem;
		element->isRecovered = 0;
		element->realNumber = i;
		foundFiles.push_back(element);

		printf("%d) %s\n", dwDeleted, stFInfo.szFilename);
	}
	return 1;
}

int setWorkParameters(int &firstElement, int &lastElement, int maxSize)
{
	int workMode;
	printf("�������� ����� ������:\n1-������� ���� ����\n");
	printf("2-������� ��������� ������\n0-����� � ������� ����\n");
	printf("��� �����: ");
	while ((!scanf_s("%d", &workMode)) || (workMode < 0 || workMode > 2))
	{
		rewind(stdin);
		printf("������! ��������� ����: ");
	}
	switch (workMode)
	{
	case 1:
	{
		printf("������� ����� ��������: ");
		while ((!scanf_s("%d", &firstElement)) || (firstElement < 1) || (firstElement > maxSize))
		{
			rewind(stdin);
			printf("������! ��������� ����: ");
		}
		lastElement = firstElement;
		return 1;
	}
	case 2:
	{
		printf("������� ������ ������� � ���������� ���������: ");
		while ((!scanf_s("%d %d", &firstElement, &lastElement)) || !(firstElement > 0 && lastElement > 0) || (firstElement > lastElement) || !(firstElement <= maxSize && lastElement <= maxSize))
		{
			rewind(stdin);
			printf("������! ��������� ����: ");
		}
		return 1;
	}
	case 0:
		return 0;
	}
}

void saveRecoveredFiles(std::vector<deletedItem*> &foundFiles)
{
	if (foundFiles.size() == 0)
		return;
	char* szPath = new char[_MAX_PATH];
	char szTmpPath[_MAX_PATH];
	int retCode;
	BYTE *pData;
	DWORD dwBytes = 0;
	DWORD dwLen;
	NTFSDrive::ST_FILEINFO stFInfo;
	HANDLE hNewFile;
	int firstElement, lastElement;

	GetCurrentDirectory(_MAX_PATH, (LPSTR)szPath);
	memcpy(szPath, setDirectory((LPTSTR)szPath), _MAX_PATH);

	while (1)
	{
		if (!setWorkParameters(firstElement, lastElement, foundFiles.size()))
			return;
		printf("�������������� ������!\n\n");
		for (int i = firstElement; i <= lastElement; i++)
		{
			if ((foundFiles[i - 1])->isRecovered == 1)
			{
				printf("���� %d ��� ������������!\n", i);
				continue;
			}

			strcpy(szTmpPath, szPath);
			retCode = m_cNTFS.Read_File((foundFiles[i - 1])->realNumber + 30, pData, dwLen);
			if (retCode)
			{
				printf("������ ������ ��������� �����!\n");
				continue;
			}
			retCode = m_cNTFS.GetFileDetail((foundFiles[i - 1])->realNumber + 30, stFInfo);
			if (retCode)
			{
				printf("������ ��������� ����� ��������� �����!\n");
				continue;
			}
			strcat(szTmpPath, "\\");
			strcat(szTmpPath, stFInfo.szFilename);
			hNewFile = CreateFile(szTmpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, stFInfo.dwAttributes, 0);
			if (hNewFile == INVALID_HANDLE_VALUE)
			{
				printf("������ �������� ��������� �����!\n");
				continue;
			}
			retCode = WriteFile(hNewFile, pData, dwLen, &dwBytes, NULL);
			if (!retCode)
			{
				printf("������ ������ ��������� �����!\n");
				continue;
			}
			CloseHandle(hNewFile);
			(foundFiles[i - 1])->isRecovered = 1;
		}
		printf("�������������� ��������!\n\n");

	}
}
