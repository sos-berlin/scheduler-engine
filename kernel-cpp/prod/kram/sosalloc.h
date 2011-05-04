// sosalloc.h

#ifndef __SOSALLOC_H
#define __SOSALLOC_H

//#define USE_SOSALLOC


namespace sos
{

void*   sos_alloc       ( int4 size, const char* info = NULL );
void    sos_free        ( void* );

void    sos_alloc_check         ( const void* );        // Prüft einen mit sos_alloc() angeforderten Datenbereich
void    sos_alloc_check         ();                     // Prüft alle Datenbereiche, nur bei [debug] check-new=yes
void    sos_alloc_list          ( ::std::ostream* );    // Zeigt alle offenen Datenbereiche, nur bei [debug] check-new=yes
void    sos_alloc_log_statistic ( ::std::ostream*, const char* info = "" );

// Meldet, wenn p..p+s einen angeforderten Speicherbereich ungültig überlappt: (nur bei check-new=yes)
void    sos_alloc_check_allocated( const void* p, uint length, const char* info = NULL );

#define SOS_ALLOC_CHECK() sos_alloc_check()


} //namespace sos

#endif
