#pragma once
#pragma pack(push, curAlignment)
#pragma pack(1)
#include <windows.h>

struct NTFS_PART_BOOT_SEC
{
	char		chJumpInstruction[3];
	char		chOemID[4];
	char		chDummy[4];

	struct NTFS_BPB
	{
		WORD		wBytesPerSec;
		BYTE		uchSecPerClust;
		WORD		wReservedSec;
		BYTE		uchReserved[3];
		WORD		wUnused1;
		BYTE		uchMediaDescriptor;
		WORD		wUnused2;
		WORD		wSecPerTrack;
		WORD		wNumberOfHeads;
		DWORD		dwHiddenSec;
		DWORD		dwUnused3;
		DWORD		dwUnused4;
		LONGLONG	n64TotalSec;
		LONGLONG	n64MFTLogicalClustNum;
		LONGLONG	n64MFTMirrLogicalClustNum;
		int			nClustPerMFTRecord;
		int			nClustPerIndexRecord;
		LONGLONG	n64VolumeSerialNum;
		DWORD		dwChecksum;
	} bpb;

	char		chBootstrapCode[426];
	WORD		wSecMark;
};


class NTFSDrive
{
protected:
	HANDLE	m_hDrive;
	DWORD m_dwStartSector;
	bool m_bInitialized;
	DWORD m_dwBytesPerCluster;
	DWORD m_dwBytesPerSector;

	int LoadMFT(LONGLONG nStartCluster);

	BYTE *m_puchMFT;
	DWORD m_dwMFTLen;

	BYTE *m_puchMFTRecord;
	DWORD m_dwMFTRecordSz;

public:
	struct ST_FILEINFO
	{
		char szFilename[_MAX_PATH];
		LONGLONG	n64Create;
		LONGLONG	n64Modify;
		LONGLONG	n64Modfil;
		LONGLONG	n64Access;
		DWORD		dwAttributes;
		LONGLONG	n64Size;
		bool 		bDeleted;
	};

	int GetFileDetail(DWORD nFileSeq, ST_FILEINFO &stFileInfo);
	int Read_File(DWORD nFileSeq, BYTE *&puchFileData, DWORD &dwFileDataLen);
	void setDriveHandle(HANDLE hDrive);
	void  SetStartSector(DWORD dwStartSector, DWORD dwBytesPerSector);
	int Initialize();
	NTFSDrive();
	virtual ~NTFSDrive();

};
