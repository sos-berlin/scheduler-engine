//#define MODULE_NAME "mapifl"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Jörg Schwiemann"

#include "precomp.h"

#if 0   //jz 12.7.01  MFC

// Mapi-Klient

// Datum: 30.10.1996
// Stand: 30.10.1996

#include <stdio.h>
#include <string.h>

#include "../kram/sysdep.h"

#if defined SYSTEM_WIN       // gesamtes mapifl.cxx


#if defined SYSTEM_MICROSOFT
#   include <afx.h>
#else
#   define STRICT
#   include <windows.h>
#endif

#if !defined SYSTEM_WIN32
    #define WIN16
#endif
#include <mapiwin.h>
#include <mapi.h>


#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/sosarray.h"
#include "../kram/log.h"
#include "../file/absfile.h"
#include "../kram/sosdll.h"

namespace sos {

// ----------------------------------------------------------------------------- Mapi_functions

//typedef ULONG FLAGS;
//typedef ULONG FAR * LPLHANDLE;

struct Mapi_functions : Sos_self_deleting, Sos_dll
{

                                DECLARE_AUTO_LOADING_DLL_PROC_6(    ULONG, MAPILogon,
                                                                    ULONG       /*ulUIParam*/,
                                                                    LPSTR       /*lpszProfileName*/,
                                                                    LPSTR       /*lpszPassword*/,
                                                                    FLAGS       /*flFlags*/,
                                                                    ULONG       /*ulReserved*/,
                                                                    LPLHANDLE   /*lplhSession*/
                                );

                                DECLARE_AUTO_LOADING_DLL_PROC_4(    ULONG, MAPILogoff,
                                                                    LHANDLE     /*lhSession*/,
                                                                    ULONG       /*ulUIParam*/,
                                                                    FLAGS       /*flFlags*/,
                                                                    ULONG       /*ulReserved*/
                                );

                                DECLARE_AUTO_LOADING_DLL_PROC_5(    ULONG, MAPISendMail,
                                                                    LHANDLE     /*lhSession*/,
                                                                    ULONG       /*ulUIParam*/,
                                                                    lpMapiMessage   /*lpMessage*/,
                                                                    FLAGS       /*flFlags*/,
                                                                    ULONG       /*ulReserved*/
                                );


                                DECLARE_AUTO_LOADING_DLL_PROC_5(    ULONG, MAPISendDocuments,
                                                                    ULONG       /*ulUIParam*/,
                                                                    LPSTR       /*lpszDelimChar*/,
                                                                    LPSTR       /*lpszFilePaths*/,
                                                                    LPSTR       /*lpszFileNames*/,
                                                                    ULONG       /*ulReserved*/
                                );

                                DECLARE_AUTO_LOADING_DLL_PROC_7(    ULONG, MAPIFindNext,
                                                                    LHANDLE     /*lhSession*/,
                                                                    ULONG       /*ulUIParam*/,
                                                                    LPSTR       /*lpszMessageType*/,
                                                                    LPSTR       /*lpszSeedMessageID*/,
                                                                    FLAGS       /*flFlags*/,
                                                                    ULONG       /*ulReserved*/,
                                                                    LPSTR       /*lpszMessageID*/
                                );
                                DECLARE_AUTO_LOADING_DLL_PROC_6(    ULONG, MAPIReadMail,
                                                                    LHANDLE     /*lhSession*/,
                                                                    ULONG       /*ulUIParam*/,
                                                                    LPSTR       /*lpszMessageID*/,
                                                                    FLAGS       /*flFlags*/,
                                                                    ULONG       /*ulReserved*/,
                                                                    lpMapiMessage FAR *  /*lppMessage*/
                                );

                                DECLARE_AUTO_LOADING_DLL_PROC_6(    ULONG, MAPISaveMail,
                                                                    LHANDLE     /*lhSession*/,
                                                                    ULONG       /*ulUIParam*/,
                                                                    lpMapiMessage   /*lpMessage*/,
                                                                    FLAGS      /*flFlags*/,
                                                                    ULONG       /*ulReserved*/,
                                                                    LPSTR       /*lpszMessageID*/
                                );

                                DECLARE_AUTO_LOADING_DLL_PROC_5(    ULONG, MAPIDeleteMail,
                                                                    LHANDLE     /*lhSession*/,
                                                                    ULONG       /*ulUIParam*/,
                                                                    LPSTR       /*lpszMessageID*/,
                                                                    FLAGS       /*flFlags*/,
                                                                    ULONG       /*ulReserved*/
                                );

                                DECLARE_AUTO_LOADING_DLL_PROC_1(    ULONG, MAPIFreeBuffer,
                                                                    LPVOID      /*pv*/
                                );

//                                DECLARE_AUTO_LOADING_DLL_PROC_11(   ULONG, MAPIAddress,
//                                                                    LHANDLE     /*lhSession*/,
//                                                                    ULONG       /*ulUIParam*/,
//                                                                    LPSTR       /*lpszCaption*/,
//                                                                    ULONG       /*nEditFields*/,
//                                                                    LPSTR       /*lpszLabels*/,
//                                                                    ULONG       /*nRecips*/,
//                                                                    lpMapiRecipDesc     /*lpRecips*/,
//                                                                    FLAGS       /*flFlags*/,
//                                                                    ULONG       /*ulReserved*/,
//                                                                    LPULONG     /*lpnNewRecips*/,
//                                                                    lpMapiRecipDesc FAR *   /*lppNewRecips*/
//                                );

                                DECLARE_AUTO_LOADING_DLL_PROC_5(    ULONG, MAPIDetails,
                                                                    LHANDLE     /*lhSession*/,
                                                                    ULONG       /*ulUIParam*/,
                                                                    lpMapiRecipDesc      /*lpRecip*/,
                                                                    FLAGS       /*flFlags*/,
                                                                    ULONG       /*ulReserved*/
                                );
                                DECLARE_AUTO_LOADING_DLL_PROC_6(    ULONG, MAPIResolveName,
                                                                    LHANDLE     /*lhSession*/,
                                                                    ULONG       /*ulUIParam*/,
                                                                    LPSTR       /*lpszName*/,
                                                                    FLAGS       /*flFlags*/,
                                                                    ULONG       /*ulReserved*/,
                                                                    lpMapiRecipDesc FAR *   /*lppRecip*/
                                );

};

#   define MAPI_LIB() _mapi_manager._functions.


struct Sos_mapi_manager : Sos_object
{
                                Sos_mapi_manager          () : _initialized(false) {}

    void                        init                      ();

private:
    friend struct               Mapi_file;
    Bool                        _initialized;
    Mapi_functions              _functions;

};

//---------------------------------------------------------------------- Sos_mapi_manager::init

void Sos_mapi_manager::init()
{
    if ( _initialized ) return;
    _initialized = true;

#if defined SYSTEM_WIN32
    _functions.init( "MAPI32.DLL" );
#else
    _functions.init( "MAPI.DLL" );
#endif
}

static Sos_mapi_manager _mapi_manager;

//----------------------------------------------------------------------------------Mapi_error

struct Mapi_error : Xc
{
            Mapi_error( const char*, const char* );
};


//-------------------------------------------------------------------------throw_mapi_error

void throw_mapi_error( const char* func, long err )
{
    char buf[20];
    sprintf( buf, "MAPI-%d", err );

    throw Mapi_error( buf, func );
}

//----------------------------------------------------------------------Mapi_error::Mapi_error

Mapi_error::Mapi_error( const char* msg, const char* func )
:
    Xc( msg, func )
{
}


//----------------------------------------------------------------------------------Mapi_file

struct Mapi_file : Abs_file
{
                                    Mapi_file            ();
                                   ~Mapi_file            ();
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

  protected:
    void                            put_record          ( const Const_area& );

  private:
    void                            newline();
    void                            fputs( const Const_area& );
    Dynamic_area                    _area;

    Sos_simple_array<Sos_string>    _recipients;
    Sos_string                      _attach;
    Sos_string                      _subject;
};

//----------------------------------------------------------------------Mapi_file_type

struct Mapi_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "mail"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Mapi_file> f = SOS_NEW_PTR( Mapi_file() );
        return +f;
    }
};

const Mapi_file_type     _mapi_file_type;
const Abs_file_type&     mapi_file_type = _mapi_file_type;

// --------------------------------------------------------------------Mapi_file::Mapi_file

Mapi_file::Mapi_file()
:
    _area(32768)       //jz 6.12.97  Vorsicht in 16bit
{
    _recipients.obj_const_name( "Mapi_file::_recipients" );
}

// -------------------------------------------------------------------Mapi_file::~Mapi_file

Mapi_file::~Mapi_file()
{
}

// -------------------------------------------------------------------------Mapi_file::open

void Mapi_file::open( const char* param, Open_mode open_mode, const File_spec& /*file_spec*/ )
{
    _mapi_manager.init(); // DLL laden

    _recipients.last_index(_recipients.first_index()-1);

    if( !( open_mode & out ) )  throw_xc( "D127" );

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.with_value( 's', "subject"  ) )  _subject = opt.value();
        else
#if defined SYSTEM_WIN32
        if( opt.with_value( "self" ) ) {
            char name [ 100+1 ];
            DWORD size = sizeof name;
            BOOL ok = GetUserName( name, &size );
            if( !ok )  throw_mswin_error( "GetUserName" );
            _recipients.add( name );
        }
        else
#endif
        if( opt.with_value( 'a', "attach"  ) )   _attach  = opt.value(); // nur ein Attachment
        else
        if( opt.param() )
        {
            _recipients.add( opt.value() );
        }
        else throw_sos_option_error( opt );
    }

    if ( _attach != "" ) newline();
}

// --------------------------------------------------------------------------Mapi_file::fputs

void Mapi_file::fputs( const Const_area& area )
{
    _area.allocate_min( _area.length()+area.length()+2+1 );
    _area.append( area );
}


// --------------------------------------------------------------------------Mapi_file::newline

void Mapi_file::newline()
{
    fputs( "\r\n" );
}


// --------------------------------------------------------------------------Mapi_file::close

void Mapi_file::close( Close_mode close_mode )
{
    Sos_string mapi_user;
    Sos_string mapi_pass;
    long err;
    LHANDLE session_handle = 0;
    MapiRecipDesc* mapi_recipients_ptr = NULL;
    MapiFileDesc mapi_attach;
    MapiMessage note;
    Bool with_attach =  _attach != "";

    if ( close_mode == close_error ) return; // keine Mail verschicken !

    mapi_user = read_profile_string( "", "mapi", "user" );
    mapi_pass = read_profile_string( "", "mapi", "pass" );

    err = MAPI_LIB()MAPILogon( 0, (LPSTR)c_str(mapi_user), (LPSTR)c_str(mapi_pass), 0, 0, &session_handle );
        if ( err != SUCCESS_SUCCESS ) throw_mapi_error( "MAPILogon", err );

    // Message vorbereiten und verschicken
    mapi_recipients_ptr = new MapiRecipDesc[_recipients.count()];
        if ( !mapi_recipients_ptr ) throw_no_memory_error();

    for ( int i=0; i < _recipients.count(); i++ ) {
        mapi_recipients_ptr[i].ulReserved   = 0;
        mapi_recipients_ptr[i].ulRecipClass = MAPI_TO;
        mapi_recipients_ptr[i].lpszName     = (LPSTR) c_str(_recipients[i+_recipients.first_index()]);
        mapi_recipients_ptr[i].lpszAddress  = NULL;
        mapi_recipients_ptr[i].ulEIDSize    = 0;
        mapi_recipients_ptr[i].lpEntryID    = NULL;
    }

    mapi_attach.ulReserved      = 0;	                    // Reserved for future use. Must be 0.
    mapi_attach.flFlags         = 0;	                    // Flags.
    mapi_attach.nPosition       = 0;	                    // Attachment position in message body.
    mapi_attach.lpszPathName    = (LPSTR) c_str(_attach);	// Full path name of attached file.
    mapi_attach.lpszFileName    = (LPSTR) c_str(_attach);    // Original filename (optional).
    mapi_attach.lpFileType      = NULL;	                    //

    _area.allocate_min(_area.length()+1);
    _area += '\0';

    note.ulReserved         = 0;                                        // reserved
    note.lpszSubject        = (LPSTR) c_str(_subject);                   // Betreff
    note.lpszNoteText       = (LPSTR) _area.char_ptr();                 // Text
    note.lpszMessageType    = NULL;                                     // Message Type
	note.lpszDateReceived   = NULL;                                     // ReceiveDate
	note.lpszConversationID = NULL;                                     // conversation thread ID
	note.flFlags            = 0;                                        // flags
	note.lpOriginator       = NULL;                                     // Originator descriptor
	note.nRecipCount        = _recipients.count();                      // Empfäner Anzahl
	note.lpRecips           = mapi_recipients_ptr;                      // Empfänger
	note.nFileCount         = with_attach ? 1            : 0;           // Anzahl Attachments
	note.lpFiles            = with_attach ? &mapi_attach : NULL;        // File Attachment(s)

    err = MAPI_LIB()MAPISendMail( 0L, 0L, &note, 0L, 0L );
    delete [] mapi_recipients_ptr;
        if ( err != SUCCESS_SUCCESS ) throw_mapi_error( "MAPISendMail", err );

    err = MAPI_LIB()MAPILogoff( session_handle, 0, 0, 0 );
        if ( err != SUCCESS_SUCCESS ) throw_mapi_error( "MAPILogoff", err );
}

// ---------------------------------------------------------------------Mapi_file::put

void Mapi_file::put_record( const Const_area& record )
{
    fputs( record );
    newline();
}

} //namespace sos

#endif // if defined SYSTEM_WIN

#endif