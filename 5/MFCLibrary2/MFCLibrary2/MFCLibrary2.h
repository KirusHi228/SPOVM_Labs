// MFCLibrary2.h : main header file for the MFCLibrary2 DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CMFCLibrary2App
// See MFCLibrary2.cpp for the implementation of this class
//

class CMFCLibrary2App : public CWinApp
{
public:
	CMFCLibrary2App();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
