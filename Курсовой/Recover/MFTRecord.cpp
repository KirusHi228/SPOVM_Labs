#include "MFTRecord.h"

MFTRecord::MFTRecord()
{
	m_hDrive = 0;

	m_dwMaxMFTRecSize = 1023; 
	m_pMFTRecord = 0;
	m_dwCurPos = 0;

	m_puchFileData = 0;
	m_dwFileDataSz = 0;

	memset(&m_attrStandard, 0, sizeof(ATTR_STANDARD));
	memset(&m_attrFilename, 0, sizeof(ATTR_FILENAME));

	m_bInUse = false;;
}

MFTRecord::~MFTRecord()
{
	if (m_puchFileData)
		delete m_puchFileData;
	m_puchFileData = 0;
	m_dwFileDataSz = 0;
}

void MFTRecord::setDriveHandle(HANDLE hDrive)
{
	m_hDrive = hDrive;
}

int MFTRecord::setRecordInfo(LONGLONG  n64StartPos, DWORD dwRecSize, DWORD dwBytesPerCluster)
{
	if (!dwRecSize)
		return ERROR_INVALID_PARAMETER;

	if (dwRecSize % 2)
		return ERROR_INVALID_PARAMETER;

	if (!dwBytesPerCluster)
		return ERROR_INVALID_PARAMETER;

	if (dwBytesPerCluster % 2)
		return ERROR_INVALID_PARAMETER;

	m_dwMaxMFTRecSize = dwRecSize;
	m_dwBytesPerCluster = dwBytesPerCluster;
	m_n64StartPos = n64StartPos;
	return 0;
}


int MFTRecord::extractFile(BYTE *puchMFTBuffer, DWORD dwLen, bool bExcludeData)
{
	if (m_dwMaxMFTRecSize > dwLen)
		return ERROR_INVALID_PARAMETER;
	if (!puchMFTBuffer)
		return ERROR_INVALID_PARAMETER;

	NTFS_MFT_FILE ntfsMFT;
	NTFS_ATTRIBUTE ntfsAttr;

	BYTE *puchTmp = 0;
	BYTE *uchTmpData = 0;
	DWORD dwTmpDataLen;
	int retCode;

	m_pMFTRecord = puchMFTBuffer;
	m_dwCurPos = 0;

	if (m_puchFileData)
		delete m_puchFileData;
	m_puchFileData = 0;
	m_dwFileDataSz = 0;

	memcpy(&ntfsMFT, &m_pMFTRecord[m_dwCurPos], sizeof(NTFS_MFT_FILE));

	if (memcmp(ntfsMFT.szSignature, "FILE", 4))
		return ERROR_INVALID_PARAMETER; 

	m_bInUse = (ntfsMFT.wFlags & 0x01);
	m_dwCurPos = ntfsMFT.wAttribOffset;

	do
	{
		memcpy(&ntfsAttr, &m_pMFTRecord[m_dwCurPos], sizeof(NTFS_ATTRIBUTE));

		switch (ntfsAttr.dwType) 
		{
		case 0:
			break;

		case 0x10:
			retCode = extractData(ntfsAttr, uchTmpData, dwTmpDataLen);
			if (retCode)
				return retCode;
			memcpy(&m_attrStandard, uchTmpData, sizeof(ATTR_STANDARD));

			delete uchTmpData;
			uchTmpData = 0;
			dwTmpDataLen = 0;
			break;

		case 0x30: 
			retCode = extractData(ntfsAttr, uchTmpData, dwTmpDataLen);
			if (retCode)
				return retCode;
			memcpy(&m_attrFilename, uchTmpData, dwTmpDataLen);

			delete uchTmpData;
			uchTmpData = 0;
			dwTmpDataLen = 0;

			break;

		case 0x40:
			break;
		case 0x50: 
			break;
		case 0x60:
			break;
		case 0x70: 
			break;
		case 0x80:
			if (!bExcludeData)
			{
				retCode = extractData(ntfsAttr, uchTmpData, dwTmpDataLen);
				if (retCode)
					return retCode;

				if (!m_puchFileData)
				{
					m_dwFileDataSz = dwTmpDataLen;
					m_puchFileData = new BYTE[dwTmpDataLen];

					memcpy(m_puchFileData, uchTmpData, dwTmpDataLen);
				}
				else
				{
					puchTmp = new BYTE[m_dwFileDataSz + dwTmpDataLen];
					memcpy(puchTmp, m_puchFileData, m_dwFileDataSz);
					memcpy(puchTmp + m_dwFileDataSz, uchTmpData, dwTmpDataLen);

					m_dwFileDataSz += dwTmpDataLen;
					delete m_puchFileData;
					m_puchFileData = puchTmp;
				}

				delete uchTmpData;
				uchTmpData = 0;
				dwTmpDataLen = 0;
			}
			break;

		case 0x90:
		case 0xa0: 
			return 0;
			continue;
			break;
		case 0xb0:
		case 0xc0:
		case 0xd0:
		case 0xe0:
		case 0xf0:
		case 0x100:
		case 0x1000:
			break;

		case 0xFFFFFFFF: 
			if (uchTmpData)
				delete uchTmpData;
			uchTmpData = 0;
			dwTmpDataLen = 0;
			return 0;

		default:
			break;
		};

		m_dwCurPos += ntfsAttr.dwFullLength;
	} while (ntfsAttr.dwFullLength);

	if (uchTmpData)
		delete uchTmpData;
	uchTmpData = 0;
	dwTmpDataLen = 0;
	return 0;
}

int MFTRecord::extractData(NTFS_ATTRIBUTE ntfsAttr, BYTE *&puchData, DWORD &dwDataLen)
{
	DWORD dwCurPos = m_dwCurPos;

	if (!ntfsAttr.uchNonResFlag)
	{
		puchData = new BYTE[ntfsAttr.Attr.Resident.dwLength];
		dwDataLen = ntfsAttr.Attr.Resident.dwLength;

		memcpy(puchData, &m_pMFTRecord[dwCurPos + ntfsAttr.Attr.Resident.wAttrOffset], dwDataLen);
	}
	else
	{

		if (!ntfsAttr.Attr.NonResident.n64AllocSize) 
			ntfsAttr.Attr.NonResident.n64AllocSize = (ntfsAttr.Attr.NonResident.n64EndVCN - ntfsAttr.Attr.NonResident.n64StartVCN) + 1;

		dwDataLen = ntfsAttr.Attr.NonResident.n64RealSize;

		puchData = new BYTE[ntfsAttr.Attr.NonResident.n64AllocSize];

		BYTE chLenOffSz; 
		BYTE chLenSz; 
		BYTE chOffsetSz;
		LONGLONG n64Len, n64Offset;
		LONGLONG n64LCN = 0; 
		BYTE *pTmpBuff = puchData;
		int retCode;

		dwCurPos += ntfsAttr.Attr.NonResident.wDatarunOffset;;

		while (1)
		{
			chLenOffSz = 0;

			memcpy(&chLenOffSz, &m_pMFTRecord[dwCurPos], sizeof(BYTE));

			dwCurPos += sizeof(BYTE);

			if (!chLenOffSz)
				break;

			chLenSz = chLenOffSz & 0x0F;
			chOffsetSz = (chLenOffSz & 0xF0) >> 4;

			n64Len = 0;

			memcpy(&n64Len, &m_pMFTRecord[dwCurPos], chLenSz);

			dwCurPos += chLenSz;

			n64Offset = 0;

			memcpy(&n64Offset, &m_pMFTRecord[dwCurPos], chOffsetSz);

			dwCurPos += chOffsetSz;

			if ((((char*)&n64Offset)[chOffsetSz - 1]) & 0x80)
				for (int i = sizeof(LONGLONG) - 1; i>(chOffsetSz - 1); i--)
					((char*)&n64Offset)[i] = 0xff;

			n64LCN += n64Offset;

			n64Len *= m_dwBytesPerCluster;
			retCode = readRaw(n64LCN, pTmpBuff, (DWORD&)n64Len);
			if (retCode)
				return retCode;

			pTmpBuff += n64Len;
		}
	}
	return 0;
}

int MFTRecord::readRaw(LONGLONG n64LCN, BYTE *chData, DWORD &dwLen)
{
	int retCode;

	LARGE_INTEGER n64Pos;

	n64Pos.QuadPart = (n64LCN)*m_dwBytesPerCluster;
	n64Pos.QuadPart += m_n64StartPos;

	retCode = SetFilePointer(m_hDrive, n64Pos.LowPart, &n64Pos.HighPart, FILE_BEGIN);
	if (retCode == 0xFFFFFFFF)
		return GetLastError();

	BYTE *pTmp = chData;
	DWORD dwBytesRead = 0;
	DWORD dwBytes = 0;
	DWORD dwTotRead = 0;

	while (dwTotRead <dwLen)
	{
		dwBytesRead = m_dwBytesPerCluster;
		retCode = ReadFile(m_hDrive, pTmp, dwBytesRead, &dwBytes, NULL);
		if (!retCode)
			return GetLastError();

		dwTotRead += dwBytes;
		pTmp += dwBytes;
	}

	dwLen = dwTotRead;

	return 0;
}