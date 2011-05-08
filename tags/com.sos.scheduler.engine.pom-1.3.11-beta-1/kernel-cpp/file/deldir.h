// deldir.h

// dirname mu· natÅrlich ein korrekter DOS-Name sein (relativ o. absolut)
// Exceptions: - DOS-Errors ( LOCKED == EACCESS von NetWare! )
//             - DIRNAME (D1??) fÅr nicht korrektem Directory-Namen

void deldir ( const char* dirname );
// Lîscht rekursiv dirname (OHNE RÅckfrage!!!)