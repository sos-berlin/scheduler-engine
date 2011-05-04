//#define MODULE_NAME "mailfl"
//#define AUTHOR      "Jörg Schwiemann"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"


#include "precomp.h"
#include <stdio.h>

#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../kram/sosstrng.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"

// Für Windows evtl. Mapi-Zugriff

#if defined SYSTEM_SOLARIS

namespace sos {

const char* sendmail_exe = "/usr/lib/sendmail"; // Solaris 2.3

//----------------------------------------------------------------------------------Mail_file

struct Mail_file : Abs_file
{
                                    Mail_file            ();
                                   ~Mail_file            ();
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

  protected:
    void                            put_record          ( const Const_area& );

  private:
    void                            newline();
    void                            fputs( const char* );
    void                            fputs( const Const_area& );
    FILE*                           _cmd_ptr;
    int                             _rc;
    Dynamic_area                    _area;
};

//----------------------------------------------------------------------Mail_file_type

struct Mail_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "mail"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Mail_file> f = SOS_NEW_PTR( Mail_file() );
        return +f;
    }
};

const Mail_file_type     _mail_file_type;
const Abs_file_type&     mail_file_type = _mail_file_type;

// --------------------------------------------------------------------Mail_file::Mail_file

Mail_file::Mail_file()
:
    _cmd_ptr(0),
    _area(256)
{
}

// -------------------------------------------------------------------Mail_file::~Mail_file

Mail_file::~Mail_file()
{
}

// -------------------------------------------------------------------------Mail_file::open

void Mail_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string recipients;
    Sos_string from;
    Sos_string subject;
    char cmd[1024]; // evtl. variabel wg. beliebige Anzahl von Empfängern

    if( !( open_mode & out ) )  throw_xc( "D127" );

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.with_value( 's', "subject"  ) )  subject = opt.value();
        else
        if( opt.with_value( 'r', "from"  ) )     from = opt.value();
        else
        if( opt.param() )
        {
            recipients += " ";
            recipients += opt.value();
        }
/*        if ( opt.pipe() ) { // Ende der Fahnenstange
            throw Xc( "SOS-1309", "mail" );
        }
*/        else throw_sos_option_error( opt );
    }

    if ( from != "" ) {
        sprintf( cmd, "%s -r %s %s", sendmail_exe, c_str( from ), c_str( recipients ) );
    } else {
        sprintf( cmd, "%s %s", sendmail_exe, c_str( recipients ) );
    }

    _cmd_ptr = popen( cmd, "w" );
    if ( !_cmd_ptr ) throw Xc( "POPEN" /*, "POPEN"*/ );

    // Header schreiben
    fputs( "Subject: " );
    fputs( c_str( subject ) );
    newline();
    fputs( "X-Mailer: mailfl 0.1  (c) SOS GmbH 1996" );
    newline(); newline(); // Eine Leerzeile zwichen Header und Body: sonst wird die Nachricht nicht erkannt
}

// --------------------------------------------------------------------------Mail_file::fputs

void Mail_file::fputs( const char* text )
{
    ::fputs( text, _cmd_ptr );
}

// --------------------------------------------------------------------------Mail_file::fputs

void Mail_file::fputs( const Const_area& area )
{
    _area.allocate_min( area.length()+1 );
    memcpy( _area.byte_ptr(), (const Byte*)area.char_ptr(), area.length() );
    _area.char_ptr()[area.length()] = '\0';
    _area.length(  area.length()+1 );
    fputs( _area.char_ptr() );
}


// --------------------------------------------------------------------------Mail_file::newline

void Mail_file::newline()
{
    fputs( "\n" );
}


// --------------------------------------------------------------------------Mail_file::close

void Mail_file::close( Close_mode close_mode )
{
    if ( _cmd_ptr != NULL ) {
        _rc = pclose(_cmd_ptr); // Fehler ???
    }
}

// ---------------------------------------------------------------------Mail_file::put

void Mail_file::put_record( const Const_area& record )
{
    fputs( record );
    newline();
}

} //namespace sos
#endif

#if defined SYSTEM_WIN32

#endif
