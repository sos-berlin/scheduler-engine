// logfile.cxx                                     ©1998 SOS Software Gmbh
// Joacim Zschimmer
/*
    Schreibt ins Log, wenn es geöffnet ist (s. soslog.cxx).
*/

#include "precomp.h"

#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/log.h"


namespace sos {
using namespace std;


//------------------------------------------------------------------------------Log_file

struct Log_file : Abs_file 
{
    void                            open                ( const char*, Open_mode, const File_spec& );

  protected:
    void                            put_record          ( const Const_area& );

    Open_mode                      _open_mode;
};

//--------------------------------------------------------------------------Log_file_type

struct Log_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "log"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Log_file> f = SOS_NEW( Log_file() );
        return +f;
    }
};

const Log_file_type      _log_file_type;
const Abs_file_type&      log_file_type = _log_file_type;

// -------------------------------------------------------------------------Log_file::open

void Log_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;
    Bool       start = false;
    Bool       stop  = false;

    if( ( open_mode & inout ) == 0 )  open_mode = File_base::Open_mode( open_mode | out );
    if( ( open_mode & inout ) != out )  throw_xc( "SOS-1388" );
    _open_mode = open_mode;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.flag( "start" ) )           start = opt.set();
        else
        if( opt.flag( "stop" ) )            stop = opt.set();
        else
        if( opt.param() || opt.pipe() )     filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    // Überprüfung der Argumente
  //int HIER_MUESSEN_NOCH_RICHTIGE_MELDUNGEN_REIN;
    if ( start && stop )                      throw_xc( "Log_file-start/stop" );
    if ( start && filename == empty_string )  throw_xc( "empty_log" );
                    
    if ( start ) {
        if ( log_ptr ) log_stop();
        log_start( c_str(filename) );
    } else if ( stop ) {
        log_stop();
    }
}

// ----------------------------------------------------------------------Log_file::put_record

void Log_file::put_record( const Const_area& record )
{
    if( log_ptr ) {
        *log_ptr << record;
        if( !( _open_mode & binary ) )  *log_ptr << '\n';
        *log_ptr << flush;
    }
}

} //namespace sos
