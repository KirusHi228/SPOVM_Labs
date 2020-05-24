#pragma once
#include "NTFSDrive.h"
#include <iostream>
#include <vector>
#include <stdio.h>

typedef struct
{
	WORD	wCylinder;
	WORD	wHead;
	WORD	wSector;
	DWORD	dwNumSectors;
	WORD	wType;
	DWORD	dwRelativeSector;
	DWORD	dwNTRelativeSector;
	DWORD	dwBytesPerSector;

}DRIVEPACKET;

typedef struct
{
	char	chBootInd;
	char	chHead;
	char	chSector;
	char	chCylinder;
	char	chType;
	char	chLastHead;
	char	chLastSector;
	char	chLastCylinder;
	DWORD	dwRelativeSector;
	DWORD	dwNumberSectors;

}PARTITION;

struct deletedItem
{
	bool isRecovered;
	int realNumber;
};

#define PART_TABLE 0
#define BOOT_RECORD 1
#define EXTENDED_PART 2
#define PART_UNKNOWN 0x00
#define PART_DOS2_FAT 0x01 
#define PART_DOS3_FAT 0x04
#define PART_EXTENDED 0x05
#define PART_DOS4_FAT 0x06
#define PART_DOS32 0x0B
#define PART_DOS32X 0x0C
#define PART_DOSX13 0x0E
#define PART_DOSX13X 0x0F 

static HANDLE m_hDrive;
static NTFSDrive m_cNTFS;

void makeDriveName(char* driveName, int i);
void addDrive(std::vector<DRIVEPACKET*> &q, DRIVEPACKET *pstDrive);
LPTSTR setDirectory(LPTSTR &Default);
int scanPartillions(char* drive, int currentDisc, std::vector<DRIVEPACKET*> &drivesVector);
int scanForFiles(int drive, DRIVEPACKET* currentDisc, std::vector<deletedItem*> &foundFiles);
int setWorkParameters(int &firstElement, int &lastElement, int maxSize);
void saveRecoveredFiles(std::vector<deletedItem*> &foundFiles);