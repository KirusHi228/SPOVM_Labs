#include "NTFSDrive.h"
#include "MFTRecord.h"

NTFSDrive::NTFSDrive()
{
	m_bInitialized = false;

	m_hDrive = 0;
	m_dwBytesPerCluster = 0;
	m_dwBytesPerSector = 0;

	m_puchMFTRecord = 0;
	m_dwMFTRecordSz = 0;

	m_puchMFT = 0;
	m_dwMFTLen = 0;

	m_dwStartSector = 0;
}

NTFSDrive::~NTFSDrive()
{
	if (m_puchMFT)
	{
		delete m_puchMFT;
	}
	m_puchMFT = 0;
	m_dwMFTLen = 0;
}

void NTFSDrive::setDriveHandle(HANDLE hDrive)
{
	m_hDrive = hDrive;
	m_bInitialized = false;
}

void NTFSDrive::SetStartSector(DWORD dwStartSector, DWORD dwBytesPerSector)
{
	m_dwStartSector = dwStartSector;
	m_dwBytesPerSector = dwBytesPerSector;
}

int NTFSDrive::Initialize()
{
	NTFS_PART_BOOT_SEC ntfsBS;
	DWORD dwBytes;
	LARGE_INTEGER n84StartPos;

	n84StartPos.QuadPart = (LONGLONG)m_dwBytesPerSector*m_dwStartSector;

	DWORD dwCur = SetFilePointer(m_hDrive, n84StartPos.LowPart, &n84StartPos.HighPart, FILE_BEGIN);

	int retCode = ReadFile(m_hDrive, &ntfsBS, sizeof(NTFS_PART_BOOT_SEC), &dwBytes, NULL);
	
	if (!retCode)
	{
		return 1;
	}

	if (memcmp(ntfsBS.chOemID, "NTFS", 4)) 
	{
		return 2;
	}

	m_dwBytesPerCluster = ntfsBS.bpb.uchSecPerClust * ntfsBS.bpb.wBytesPerSec;

	if (m_puchMFTRecord)
	{
		delete m_puchMFTRecord;
	}

	m_dwMFTRecordSz = 0x01 << ((-1)*((char)ntfsBS.bpb.nClustPerMFTRecord));
	m_puchMFTRecord = new BYTE[m_dwMFTRecordSz];

	m_bInitialized = true;

	retCode = LoadMFT(ntfsBS.bpb.n64MFTLogicalClustNum);
	if (retCode)
	{
		m_bInitialized = false;
		return retCode;
	}
	return 0;
}

int NTFSDrive::LoadMFT(LONGLONG nStartCluster)
{
	DWORD dwBytes;
	int retCode;
	LARGE_INTEGER n64Pos;

	if (!m_bInitialized)
	{
		return ERROR_INVALID_ACCESS;
	}

	MFTRecord cMFTRec;

	wchar_t uszMFTName[10];
	mbstowcs(uszMFTName, "$MFT", 10);

	n64Pos.QuadPart = (LONGLONG)m_dwBytesPerSector*m_dwStartSector;
	n64Pos.QuadPart += (LONGLONG)nStartCluster*m_dwBytesPerCluster;

	retCode = SetFilePointer(m_hDrive, n64Pos.LowPart, &n64Pos.HighPart, FILE_BEGIN);
	if (retCode == 0xFFFFFFFF)
	{
		return GetLastError();
	}

	retCode = ReadFile(m_hDrive, m_puchMFTRecord, m_dwMFTRecordSz, &dwBytes, NULL);
	if (!retCode)
	{
		return GetLastError();
	}

	cMFTRec.setDriveHandle(m_hDrive);
	cMFTRec.setRecordInfo((LONGLONG)m_dwStartSector*m_dwBytesPerSector, m_dwMFTRecordSz, m_dwBytesPerCluster);
	retCode = cMFTRec.extractFile(m_puchMFTRecord, dwBytes);
	if (retCode)
	{
		return retCode;
	}

	if (memcmp(cMFTRec.m_attrFilename.wFilename, uszMFTName, 8))
	{
		return ERROR_BAD_DEVICE; 
	}

	if (m_puchMFT)
	{
		delete m_puchMFT;
	}

	m_puchMFT = 0;
	m_dwMFTLen = 0;

	m_puchMFT = new BYTE[cMFTRec.m_dwFileDataSz];
	m_dwMFTLen = cMFTRec.m_dwFileDataSz;

	memcpy(m_puchMFT, cMFTRec.m_puchFileData, m_dwMFTLen);

	return 0;
}

int NTFSDrive::Read_File(DWORD nFileSeq, BYTE *&puchFileData, DWORD &dwFileDataLen)
{
	int retCode;
	MFTRecord cFile;

	if (!m_bInitialized)
	{
		return ERROR_INVALID_ACCESS;
	}

	memcpy(m_puchMFTRecord, &m_puchMFT[nFileSeq*m_dwMFTRecordSz], m_dwMFTRecordSz);

	cFile.setDriveHandle(m_hDrive);
	cFile.setRecordInfo((LONGLONG)m_dwStartSector*m_dwBytesPerSector, m_dwMFTRecordSz, m_dwBytesPerCluster);
	retCode = cFile.extractFile(m_puchMFTRecord, m_dwMFTRecordSz);

	if (retCode)
		return 1;

	puchFileData = new BYTE[cFile.m_dwFileDataSz];
	dwFileDataLen = cFile.m_dwFileDataSz;

	memcpy(puchFileData, cFile.m_puchFileData, dwFileDataLen);

	return 0;
}


int NTFSDrive::GetFileDetail(DWORD nFileSeq, ST_FILEINFO &stFileInfo)
{
	int retCode = 0;

	if (!m_bInitialized)
	{
		return ERROR_INVALID_ACCESS;
	}

	if ((nFileSeq*m_dwMFTRecordSz + m_dwMFTRecordSz) >= m_dwMFTLen)
	{
		return ERROR_NO_MORE_FILES;
	}

	MFTRecord cFile;

	memcpy(m_puchMFTRecord, &m_puchMFT[nFileSeq*m_dwMFTRecordSz], m_dwMFTRecordSz);

	cFile.setDriveHandle(m_hDrive);
	cFile.setRecordInfo((LONGLONG)m_dwStartSector*m_dwBytesPerSector, m_dwMFTRecordSz, m_dwBytesPerCluster);
	retCode = cFile.extractFile(m_puchMFTRecord, m_dwMFTRecordSz, true);

	if (retCode)
	{
		return retCode;
	}

	memset(&stFileInfo, 0, sizeof(ST_FILEINFO));
	wcstombs(stFileInfo.szFilename, (const wchar_t*)cFile.m_attrFilename.wFilename, _MAX_PATH);

	stFileInfo.dwAttributes = cFile.m_attrFilename.dwFlags;

	stFileInfo.n64Create = cFile.m_attrStandard.n64Create;
	stFileInfo.n64Modify = cFile.m_attrStandard.n64Modify;
	stFileInfo.n64Access = cFile.m_attrStandard.n64Access;
	stFileInfo.n64Modfil = cFile.m_attrStandard.n64Modfil;

	stFileInfo.n64Size = cFile.m_attrFilename.n64Allocated;
	stFileInfo.n64Size /= m_dwBytesPerCluster;
	stFileInfo.n64Size = (!stFileInfo.n64Size) ? 1 : stFileInfo.n64Size;

	stFileInfo.bDeleted = !cFile.m_bInUse;

	return retCode;
}

