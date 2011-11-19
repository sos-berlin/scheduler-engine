// sosfile.h                                            (c) SOS GmbH Berlin

#if 0      // L÷SCHEN!

#ifndef __SOSFILE_H
#define __SOSFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _Windows
#  include <windows.h>
#  define SOSDLL pascal _far
#else
#  define SOSDLL _far
#endif

/* Die hostAPI DLL arbeitet mit FileHandles */
typedef int Sos_file;

typedef struct Sos_file_spec
{
    int key_count;
    /*.... Noch nicht definiert ...*/
} Sos_file_spec;

typedef enum Sos_open_mode {
	/* Folgende Zeilen stammen aus iostream.h, C++-Klasse ios: */
	sos_in	     = 0x01,	// open for reading
	sos_out	     = 0x02,	// open for writing
	sos_ate	     = 0x04,	// seek to eof upon original open
	sos_app	     = 0x08,	// append mode: all additions at eof
	sos_trunc    = 0x10,	// truncate file if already exists
	sos_nocreate = 0x20,	// open fails if file doesn't exist
	sos_noreplace= 0x40,	// open fails if file already exists
	sos_binary   = 0x80,	// binary (not text) file
	sos_standard_mask = 0x00FF
	,
	// Erweiterungen:
        sos_seq      = 0x1000,  // Nur sequentieller Zugriff
	sos_share    = 0x2000,  // Gemeinsamer Zugriff
	sos_unsafe   = 0x8000   // Nicht gesichert, dafÅr schnell
} Sos_open_mode;

/* Dateioperationen auf dem Typ Sos_file */

/*---------------------------------------------------------------------sos_open*/

extern Sos_file SOSDLL 
sos_open( const char* file_name, Sos_open_mode );

 /* ôffnet eine Datei. PrÑfix "nuc:" oder "fs:" îffnet eine Datei Åber den 
    Fileserver auf dem BS2000. Dateinamenskonventionen wie in RAPID 
    (s. Handbuch Programmschnittstellen)

    sos_file_spec = 0
 */

/*--------------------------------------------------------------------sos_close */

extern int SOSDLL 
sos_close( Sos_file );

 /* Schlie·t eine mit sos_open geîffnete Datei. */

/*----------------------------------------------------------------------sos_get */

extern int SOSDLL
sos_get( Sos_file, void* buffer, int buffer_size );

 /* Liest sequentiell den nÑchsten Satz. */

/*------------------------------------------------------------------sos_get_key */

extern int SOSDLL 
sos_get_key( Sos_file, void* area, int area_size,
             const void* key, const char* key_name );

 /* Liest direkt den mit key angegebenen Satz.
    Der Name des SchlÅssels (SekundÑrschlÅssels) kann mit key_name als 0-
    terminierter String angegeben werden. Der PrimÑrschlÅssel kann mit
    0 oder "" angegeben werden.
 */

/*----------------------------------------------------------------------sos_put */

extern int SOSDLL 
sos_put( Sos_file, const void* record, int record_length );

 /* Schreibt den Satz an das Ende der Datei.
    Beim Fileserver werden die SÑtze geblockt Åbertragen.
 */

/*----------------------------------------------------------------------sos_insert */

extern int SOSDLL
sos_insert( Sos_file, const void* record, int record_length );

 /* FÅgt den Satz entsprechend seinen SchlÅsseln in die Datei ein.
    Falls bereits ein Satz mit gleichem SchlÅssel vorliegt wird der
    Satz nicht eingefÅgt und ein Fehlercode zurÅckgegeben.
 */

/*----------------------------------------------------------------------sos_update */

extern int SOSDLL
sos_update( Sos_file, const void* record, int record_length );

 /* Schreibt einen zuvor gelesenen Satz in die Datei zurÅck.
    Zwischen der Leseoperatioen und diesem Aufruf darf keine andere Operation
    auf diese Datei durchgefÅhrt werden.
    Der PrimÑrschlÅssel darf sich bei manchen Dateitypen (ISAM, LEASY) nicht
    Ñndern.
 */

/*----------------------------------------------------------------------sos_store */

extern int SOSDLL
sos_store( Sos_file, const void* record, int record_length );

 /* FÅgt einen Satz entsprechend seinen SchlÅsseln in die Datei ein.
    Falls ein Satz mit gleichem PrimÑrschlÅssel existiert, wird dieser
    Åberschrieben.
    Diese Funktion ist nicht fÅr alle Dateitypen mîglich (z.Z. nur ISAM).
 */

/*----------------------------------------------------------------------sos_delete_key */

extern int SOSDLL
sos_delete_key( Sos_file, const void* key, const char* key_name );

 /* Lîscht den Åber key und key_name angegebenen Satz (s.a. sos_get_key). */


/*----------------------------------------------------------------------sos_set_key */

extern int SOSDLL
sos_set_key( Sos_file, const void* key, const char* key_name );

 /* Positionieren auf den angegebenen SchlÅssel. */

/*----------------------------------------------------------------------sos_rename_file */

extern int SOSDLL
sos_rename_file( const char* old_name, const char* new_name );

 /* Gibt der angegebenen Datei einen neuen Namen.
    Geplant: Mit library kann die Bibliothek angegeben werden, in der die Datei (das
    Member) enthalten ist. Ansonsten ist libary 0.
 */

/*--------------------------------------------------------------sos_delete_file */

extern int SOSDLL
sos_delete_file( const char* file_name );

 /* Lîscht die angegebene Datei.
    Geplant: Mit library kann die Bibliothek angegeben werden, in der die Datei (das
    Member) enthalten ist. Ansonsten ist library 0.
 */

/*--------------------------------------------------------------------sos_errno */

extern int SOSDLL
sos_errno();

 /* Liefert nochmal den Returncode des letzten Aufrufs. 0: Kein Fehler. */


/*---------------------------------------------------------------sos_error_code */

extern const char* SOSDLL 
sos_error_code();

 /* Liefert den Fehlercode des letzten Aufrufs als mit 0 terminierten String.
    Wenn kein Fehler auftrat, wird ein Leerstring zurÅckgegeben.
    Die Fehlercodes sind im ATLAS-Handbuch Programmierschnittstellen
    beschrieben.
 */

/*-----------------------------------------------------------sos_exception_name */

extern const char* SOSDLL 
sos_exception_name();

 /* Liefert den Namen der Exception des letzten Aufrufs als mit 0 terminierten
    String. Der Name der Exception klassifiziert den Fehler.
    Wenn kein Fehler auftrat, wird ein Leerstring zurÅckgegeben.
 */

//-----------------------------------------------------------------------------


#ifdef __cplusplus
}         
#endif

#endif


