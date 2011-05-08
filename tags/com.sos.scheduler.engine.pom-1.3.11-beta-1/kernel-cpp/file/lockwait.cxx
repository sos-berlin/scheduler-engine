//#define MODULE_NAME "lockwait"
//#define COPYRIGHT   "©1997 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#include "precomp.h"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/log.h"
#include "../kram/sleep.h"

#include <time.h>

namespace sos {
using namespace std;


#define LOCK_LOOP_BEGIN \
{                                                                    \
    time_t timeout_clock = time( NULL ) + _timeout;                  \
                                                                     \
    while(1) {          \
        try {

#define LOCK_LOOP_END \
            break;                                                   \
        }                                                            \
        catch( const Locked_error& )                                 \
        {                                                            \
            if( time( NULL ) > timeout_clock )  throw;               \
            sos_sleep( _retry_time );                                \
        }                                                            \
    }                                                                \
}

//--------------------------------------------------------------------------------Lockwait_file

struct Lockwait_file : Abs_file
{
    BASE_CLASS( Abs_file )
                                Lockwait_file          ();
                               ~Lockwait_file          ();

    virtual void                prepare_open           ( const char*, Open_mode, const File_spec& );
    virtual void                open                   ( const char*, Open_mode, const File_spec& );
    virtual void                close                  ( Close_mode = close_normal );

    virtual void                insert                 ( const Const_area& );
    virtual void                store                  ( const Const_area& );
    virtual void                update                 ( const Const_area& );
    virtual void                set                    ( const Key& );
  //virtual void                del                    ();
    virtual void                del                    ( const Key& );
    virtual void                del                    ()   { Base_class::del(); }
  //void                        invoke                 ( const Sos_string& proc_name, uint4 pass,
  //                                                     const Const_area& input, Area* output );

    static Bool                 get_msg_text           ( ostream*, const Key& );
    static Bool                 get_msg_text           ( Area*,    const Key& );

  protected:
    virtual void                get_record             ( Area& );
    virtual void                get_position           ( Area* );
    virtual void                put_record             ( const Const_area& );
  //virtual void                get_record_lock        ( Area&, Record_lock );
    virtual void                get_record_key         ( Area&, const Key& );

  private:
    Fill_zero                  _zero_;
    Any_file                   _file;
    int                        _timeout;
    int                        _retry_time;
};

//--------------------------------------------------------------------------Lockwait_file_type

struct Lockwait_file_type : Abs_file_type
{
    virtual const char*         name            () const { return "lockwait"; }
  //virtual const char*         alias_name      () const { return "xxx"; }
    virtual Sos_ptr<Abs_file>   create_base_file() const
    {
        Sos_ptr<Lockwait_file> o = SOS_NEW( Lockwait_file );
        return +o;
    }
};

const Lockwait_file_type  _lockwait_file_type;
const Abs_file_type&       lockwait_file_type = _lockwait_file_type;

//-----------------------------------------------------------------Lockwait_file::Lockwait_file

Lockwait_file::Lockwait_file()
:
    _zero_ ( this+1)
{
}

//----------------------------------------------------------------Lockwait_file::~Lockwait_file

Lockwait_file::~Lockwait_file()
{
}

//------------------------------------------------------------------Lockwait_file::prepare_open

void Lockwait_file::prepare_open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;

    _timeout    = read_profile_uint( "", "lockwait", "timeout", 30 );
    _retry_time = read_profile_uint( "", "lockwait", "retry-time", 1 );

    for( Sos_option_iterator opt = param; !opt.end(); opt.next() )
    {
        if( opt.with_value( "timeout" ) )      _timeout = opt.as_int();
        else
        if( opt.with_value( "retry-time" ) )   _retry_time = opt.as_int();
        else
        if( opt.pipe()                     )  { filename = opt.rest(); break; }
        else
        throw_sos_option_error( opt );
    }



    LOCK_LOOP_BEGIN
        _file.prepare( filename, open_mode, file_spec );
    LOCK_LOOP_END

    _any_file_ptr->_spec = _file.spec();
}

//--------------------------------------------------------------------------Lockwait_file::open

void Lockwait_file::open( const char*, Open_mode, const File_spec& )
{
    LOCK_LOOP_BEGIN
        _file.open();
    LOCK_LOOP_END
}

//-------------------------------------------------------------------------Lockwait_file::close

void Lockwait_file::close( Close_mode close_mode )
{
    LOCK_LOOP_BEGIN
        _file.close( close_mode );
    LOCK_LOOP_END
}

//----------------------------------------------------------Lockwait_file::get_record

void Lockwait_file::get_record( Area& buffer )
{
    LOCK_LOOP_BEGIN
        _file.get( &buffer );
    LOCK_LOOP_END
}

//-----------------------------------------------------------------------Lockwait_file::get_position

void Lockwait_file::get_position( Area* buffer )
{
    LOCK_LOOP_BEGIN
        _file.get_position( buffer );
    LOCK_LOOP_END
}

//--------------------------------------------------------------------Lockwait_file::put_record

void Lockwait_file::put_record( const Const_area& record )
{
    //LOCK_LOOP_BEGIN
    time_t timeout_clock = time( NULL ) + _timeout;

    while(1) {
        try {

            _file.put( record );

            break;
        }
        catch( const Locked_error& )
        {
            if( time( NULL ) > timeout_clock )  throw;
            sos_sleep( _retry_time );
        }
        catch( const Xc& x )
        {
            if ( strcmpi( x.code(), "INGRES-125E" ) != 0 ) throw;
            if( time( NULL ) > timeout_clock )  throw;
            sos_sleep( _retry_time );
        }
    }
    //LOCK_LOOP_END
}

//----------------------------------------------------------------Lockwait_file::get_record_key

void Lockwait_file::get_record_key( Area& area, const Key& key )
{
    LOCK_LOOP_BEGIN
        _file.get_key( &area, key );
    LOCK_LOOP_END
}

//---------------------------------------------------------------Lockwait_file::get_record_lock
/*
void Lockwait_file::get_record_lock( Area& area, Record_lock lock )
{
    LOCK_LOOP_BEGIN
        _file.get_lock( &area, lock );
    LOCK_LOOP_END
}
*/
//------------------------------------------------------------------------Lockwait_file::insert

void Lockwait_file::insert( const Const_area& area )
{
    LOCK_LOOP_BEGIN
        _file.insert( area );
    LOCK_LOOP_END
}

//-------------------------------------------------------------------------Lockwait_file::store

void Lockwait_file::store( const Const_area& area )
{
    LOCK_LOOP_BEGIN
        _file.store( area );
    LOCK_LOOP_END
}

//------------------------------------------------------------------------Lockwait_file::update

void Lockwait_file::update( const Const_area& area )
{
    LOCK_LOOP_BEGIN
        _file.update( area );
    LOCK_LOOP_END
}

//---------------------------------------------------------------------------Lockwait_file::set

void Lockwait_file::set( const Key& key )
{
    LOCK_LOOP_BEGIN
        _file.set( key );
    LOCK_LOOP_END
}

//---------------------------------------------------------------------------Lockwait_file::del

void Lockwait_file::del( const Key& key )
{
    LOCK_LOOP_BEGIN
        _file.del( key );
    LOCK_LOOP_END
}

//------------------------------------------------------------------------Lockwait_file::invoke
/*
void Lockwait_file::invoke( const Sos_string& proc_name, uint4 pass,
                            const Const_area& input, Area* output )
{
    LOCK_LOOP_BEGIN
        _file.invoke( proc_name, pass, input, output );
    LOCK_LOOP_END
}
*/

} //namespace sos
