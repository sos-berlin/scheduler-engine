/*--------------------------------------------------------------------------
  Drvsetup.h -- Resource IDs used by ODBC Setup

  (c) Microsoft Corp., 1990-1994
--------------------------------------------------------------------------*/


#include "sosodbc.hrc"

// Bitmaps and ICONs -------------------------------------------------------
#define IDI_ICON		100
#define IDI_BITMAP		200


// Controls ----------------------------------------------------------------
#define IDX				300


// String IDs --------------------------------------------------------------
#define IDS_FRAMETITLE	0x0010
#define IDS_BADODBCI	0x0011
#define IDS_BADODBC		0x0012
#define IDS_BADDS		0x0013
#define IDS_BADINST		0x0014
#define IDS_BADOPT		0x0015
#define IDS_WIN32S		0x0016


// Dialogs IDs -------------------------------------------------------------
#define WELCOME			1000
#define ASKQUIT			1001
#define EXITSUCCESS		1002
#define EXITQUIT		1003
#define EXITFAILURE		1004
#define AWELCOME		2000
#define AEXITSUCCESS	2001
#define AEXITFAILURE	2002

//
//	Common defines used by ODBC Setup
//

#define ODBCSETUPCLASS "ODBCSETUP"
/*****************************************************************
** Localized messages and strings
**
** (C) Copyright 1993 - 1994 By Microsoft Corp.
*********************************************************************/


//	Localize the standard error messages
#define ET_00000 "Success"
#define ET_01000 "General warning"
#define ET_01002 "Disconnect error"
#define ET_01004 "Data truncated"
#define ET_01006 "Privilege not revoked"
#define ET_01S00 "Invalid connection string attribute"
#define ET_01S01 "Error in row"
#define ET_01S02 "Option value changed"
#define ET_01S03 "No rows updated or deleted"
#define ET_01S04 "More than one row updated or deleted"
#define ET_01S05 "Cancel treated as FreeStmt/Close"
#define ET_07001 "Wrong number of parameters"
#define ET_07006 "Restricted data type attribute violation"
#define ET_08001 "Unable to connect to data source"
#define ET_08002 "Connection in use"
#define ET_08003 "Connection not open"
#define ET_08004 "Data source rejected establishment of connection"
#define ET_08007 "Connection failure during transaction"
#define ET_08S01 "Communication link failure"
#define ET_21S01 "Insert value list does not match column list"
#define ET_21S02 "Degree of derived table does not match column list"
#define ET_22001 "String data right truncation"
#define ET_22003 "Numeric value out of range"
#define ET_22005 "Error in assignment"
#define ET_22008 "Datetime field overflow"
#define ET_22012 "Division by zero"
#define ET_22026 "String data, length mismatch"
#define ET_23000 "Integrity constraint violation"
#define ET_24000 "Invalid cursor state"
#define ET_25000 "Invalid transaction state"
#define ET_28000 "Invalid authorization specification"
#define ET_34000 "Invalid cursor name"
#define ET_37000 "Syntax error or access violation"
#define ET_3C000 "Duplicate cursor name"
#define ET_40001 "Serialization failure"
#define ET_42000 "Syntax error or access violation"
#define ET_70100 "Operation aborted"
#define ET_IM001 "Driver does not support this function"
#define ET_IM002 "Data source name not found and no default driver specified"
#define ET_IM003 "Specified driver could not be loaded"
#define ET_IM004 "Driver's SQLAllocEnv failed"
#define ET_IM005 "Driver's SQLAllocConnect failed"
#define ET_IM006 "Driver's SQLSetConnectOption failed"
#define ET_IM007 "No data source or driver specified; dialog prohibited"
#define ET_IM008 "Dialog failed"
#define ET_IM009 "Unable to load translation DLL"
#define ET_IM010 "Data source name too long"
#define ET_IM011 "Driver name too long"
#define ET_IM012 "DRIVER keyword syntax error"
#define ET_IM013 "Trace file error"
#define ET_S0001 "Base table or view already exists"
#define ET_S0002 "Base table not found"
#define ET_S0011 "Index already exists"
#define ET_S0012 "Index not found"
#define ET_S0021 "Column already exists"
#define ET_S0022 "Column not found"
#define ET_S0023 "No default for column"
#define ET_S1000 "General error"
#define ET_S1001 "Memory allocation failure"
#define ET_S1002 "Invalid column number"
#define ET_S1003 "Program type out of range"
#define ET_S1004 "SQL data type out of range"
#define ET_S1008 "Operation canceled"
#define ET_S1009 "Invalid argument value"
#define ET_S1010 "Function sequence error"
#define ET_S1011 "Operation invalid at this time"
#define ET_S1012 "Invalid transaction operation code specified"
#define ET_S1015 "No cursor name available"
#define ET_S1090 "Invalid string or buffer length"
#define ET_S1091 "Descriptor type out of range"
#define ET_S1092 "Option type out of range"
#define ET_S1093 "Invalid parameter number"
#define ET_S1094 "Invalid scale value"
#define ET_S1095 "Function type out of range"
#define ET_S1096 "Information type out of range"
#define ET_S1097 "Column type out of range"
#define ET_S1098 "Scope type out of range"
#define ET_S1099 "Nullable type out of range"
#define ET_S1100 "Uniqueness option type out of range"
#define ET_S1101 "Accuracy option type out of range"
#define ET_S1103 "Direction option out of range"
#define ET_S1104 "Invalid precision value"
#define ET_S1105 "Invalid parameter type"
#define ET_S1106 "Fetch type out of range"
#define ET_S1107 "Row value out of range"
#define ET_S1108 "Concurrency option out of range"
#define ET_S1109 "Invalid cursor position"
#define ET_S1110 "Invalid driver completion"
#define ET_S1111 "Invalid bookmark value"
#define ET_S1C00 "Driver not capable"
#define ET_S1DE0 "No data at execution values pending"
#define ET_S1T00 "Timeout expired"

//	Add the appropriate entries to the following tables for a new language
#define USENGLISH_ANSI		"040904E4"	// String of 0x0409 and 1252
#define FRENCH_ANSI 		"040C04E4"	// String of 0x040C and 1252
#define GERMAN_ANSI 		"040704E4"	// String of 0x0407 and 1252
#define ITALIAN_ANSI		"041004E4"	// String of 0x0410 and 1252
#define PORTUGUESE_ANSI 	"041604E4"	// String of 0x0416 and 1252
#define SWEDISH_ANSI		"041D04E4"	// String of 0x041D and 1252
#define SPANISH_ANSI		"040A04E4"	// String of 0x040A and 1252
#define DANISH_ANSI			"040604E4"	// String of 0x0406 and 1252
#define FINNISH_ANSI		"040B04E4"	// String of 0x040B and 1252
#define DUTCH_ANSI			"041304E4"	// String of 0x0413 and 1252
#define NORWEGIAN_ANSI		"041404E4"	// String of 0x0414 and 1252
#define CHINESE_TRAD_ANSI	"040404E4"	// String of 0x0404 and 1252
#define CHINESE_SIMP_ANSI	"080404E4"	// String of 0x0804 and 1252

#define USENGLISH_TRANS 	0x0409, 1252  // 0x0409 and 1252
#define FRENCH_TRANS		0x040C, 1252  // 0x040C and 1252
#define GERMAN_TRANS		0x0407, 1252  // 0x0407 and 1252
#define ITALIAN_TRANS		0x0410, 1252  // 0x0410 and 1252
#define PORTUGUESE_TRANS	0x0416, 1252  // 0x0416 and 1252
#define SWEDISH_TRANS		0x041D, 1252  // 0x041D and 1252
#define SPANISH_TRANS		0x040A, 1252  // 0x040A and 1252
#define DANISH_TRANS		0x0406, 1252  // 0x0406 and 1252
#define FINNISH_TRANS		0x040B, 1252  // 0x040B and 1252
#define DUTCH_TRANS 		0x0413, 1252  // 0x0413 and 1252
#define NORWEGIAN_TRANS 	0x0414, 1252  // 0x0414 and 1252
#define CHINESE_TRAD_TRANS	0x0404, 1252  // 0x0404 and 1252
#define CHINESE_SIMP_TRANS	0x0804, 1252  // 0x0804 and 1252

//	Replace the USENGLISH with the correct language
#define LANGUAGE_ANSI	USENGLISH_ANSI
#define LANGUAGE_TRANS	USENGLISH_TRANS

//	Localize the legal strings
//#define VER_LEGALTRADEMARKS_STR "Windows(TM) and ODBC(TM) are trademarks of Microsoft Corporation.  Microsoft\256 is a registered trademark of Microsoft Corporation.\0"

//	Localize the diskette label
#define DISKETTE_LABEL_STR		"ODBC Drivers Setup"

//	Localize the SETUP.exe program name
#define SETUP_PROG_STR			"setup.exe"
