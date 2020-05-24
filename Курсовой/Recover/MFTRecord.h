#pragma once
#include <windows.h>
#pragma pack(push, curAlignment)
#pragma pack(1)

struct NTFS_MFT_FILE
{
	char		szSignature[4];
	WORD		wFixupOffset;
	WORD		wFixupSize;	
	LONGLONG	n64LogSeqNumber;
	WORD		wSequence;	
	WORD		wHardLinks;	
	WORD		wAttribOffset;	
	WORD		wFlags;		
	DWORD		dwRecLength;
	DWORD		dwAllLength;
	LONGLONG	n64BaseMftRec;
	WORD		wNextAttrID;
	WORD		wFixupPattern;
	DWORD		dwMFTRecNumber;
};

typedef struct
{
	DWORD	dwType;
	DWORD	dwFullLength;
	BYTE	uchNonResFlag;
	BYTE	uchNameLength;
	WORD	wNameOffset;
	WORD	wFlags;
	WORD	wID;

	union ATTR
	{
		struct RESIDENT
		{
			DWORD	dwLength;
			WORD	wAttrOffset;
			BYTE	uchIndexedTag;
			BYTE	uchPadding;
		} Resident;

		struct NONRESIDENT
		{
			LONGLONG	n64StartVCN;
			LONGLONG	n64EndVCN;
			WORD		wDatarunOffset;
			WORD		wCompressionSize;
			BYTE		uchPadding[4];
			LONGLONG	n64AllocSize;
			LONGLONG	n64RealSize;
			LONGLONG	n64StreamSize;
		}NonResident;
	}Attr;
} NTFS_ATTRIBUTE;


typedef struct
{
	LONGLONG	n64Create;
	LONGLONG	n64Modify;
	LONGLONG	n64Modfil;
	LONGLONG	n64Access;
	DWORD		dwFATAttributes;
	DWORD		dwReserved1;

} ATTR_STANDARD;

typedef struct
{
	LONGLONG	dwMftParentDir;
	LONGLONG	n64Create;
	LONGLONG	n64Modify;
	LONGLONG	n64Modfil;
	LONGLONG	n64Access;
	LONGLONG	n64Allocated;
	LONGLONG	n64RealSize;
	DWORD		dwFlags;
	DWORD		dwEAsReparsTag;
	BYTE		chFileNameLength;
	BYTE		chFileNameType;
	WORD		wFilename[512];

}ATTR_FILENAME;


class MFTRecord
{
protected:
	HANDLE	m_hDrive;

	BYTE *m_pMFTRecord;
	DWORD m_dwMaxMFTRecSize;
	DWORD m_dwCurPos;
	DWORD m_dwBytesPerCluster;
	LONGLONG m_n64StartPos;

	int readRaw(LONGLONG n64LCN, BYTE *chData, DWORD &dwLen);
	int extractData(NTFS_ATTRIBUTE ntfsAttr, BYTE *&puchData, DWORD &dwDataLen);

public:
	MFTRecord();
	virtual ~MFTRecord();

	ATTR_STANDARD m_attrStandard;
	ATTR_FILENAME m_attrFilename;

	BYTE *m_puchFileData; 
	DWORD m_dwFileDataSz; 

	bool m_bInUse;
	int setRecordInfo(LONGLONG n64StartPos, DWORD dwRecSize, DWORD dwBytesPerCluster);
	void setDriveHandle(HANDLE hDrive);
	int extractFile(BYTE *puchMFTBuffer, DWORD dwLen, bool bExcludeData = false);
};
