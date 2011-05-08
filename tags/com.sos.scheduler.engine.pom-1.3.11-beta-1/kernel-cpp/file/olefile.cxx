//#define MODULE_NAME "olefile"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1997 SOS GmbH Berlin"


#include "precomp.h"

#include "../kram/sysdep.h"

#if defined SYSTEM_WIN32

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/dynobj.h"
#include "../kram/sosole.h"
#include "../kram/sosopt.h"

#include "../kram/log.h"


namespace sos {

//---------------------------------------------------------------------------Ole_file

struct Ole_file : Abs_file
{
                                Ole_file();
                               ~Ole_file();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );

  protected:
    void                        get_record              ( Area& );
    void                        put_record              ( const Const_area& );

  private:
    Fill_zero                  _zero_;
    Ole_object                 _ole_object;

    Sos_dyn_obj_array          _put_params;
    Ole_method*                _put_method;
    
    Sos_dyn_obj_array          _get_params;
    Ole_method*                _get_method;
};

//----------------------------------------------------------------------Hostole_file_type

struct Hostole_file_type : Abs_file_type
{
    virtual const char*         name                    () const  { return "hostole"; }
  //virtual const char*         alias_name              () const  { return ""; };

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Ole_file> f = SOS_NEW_PTR( Ole_file() );
        return +f;
    }
};

const Hostole_file_type        _hostole_file_type;
const Abs_file_type&            hostole_file_type = _hostole_file_type;

// ------------------------------------------------------Ole_file::Ole_file

Ole_file::Ole_file()
:
    _zero_(this+1)
{
}

//------------------------------------------------------Ole_file::~Ole_file

Ole_file::~Ole_file()
{
}

//---------------------------------------------------------------------Ole_file::open

void Ole_file::open( const char* parameter, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string   filename;
    Ole_method*  open_method = NULL;


    if( ( open_mode & ( Any_file::in | Any_file::out ) ) == 0 )  throw_xc( "SOS-1357" );   // openmodus fehlt?

    for( Sos_option_iterator opt( parameter ); !opt.end(); opt.next() )
    {
        if( opt.param( 1 )                )   { _ole_object._class_name = opt.value(); }
        else
        if( opt.pipe()                    )   { filename = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    if( empty( _ole_object._class_name ) )   throw_xc( "SOS-1360" );

    _ole_object.prepare();

    try {
        open_method = _ole_object.method( "Open" );
    } 
    catch( const Xc& ) {}

    if( open_method ) 
    {
        Dyn_obj             result;  // dummy
        Sos_dyn_obj_array   params;

        params.size( 2 );
        params.add( filename );
        params.add( open_mode );
        open_method->invoke( &result, params );
    }

    _put_params.size( 1 );
    _put_params.add_empty();
}

//--------------------------------------------------------------------Ole_file::close

void Ole_file::close( Close_mode close_mode )
{
    Ole_method* close_method = NULL;

    try {
        close_method = _ole_object.method( "Close" );
    }
    catch( const Xc& ) {}

    if( close_method ) 
    {
        Dyn_obj             result;  // dummy
        Sos_dyn_obj_array   params;

        //params.size( 1 );
        //params.add( close_mode );
        close_method->invoke( &result, params );
    }
}

//---------------------------------------------------------------Ole_file::get_record

void Ole_file::get_record( Area& buffer )
{
    Dyn_obj             result;  // dummy
    Sos_dyn_obj_array   params;

    if( !_get_method )  {
        try {
            _get_method = _ole_object.method( "Get" );
        } 
        catch( const Xc& ) {}
        if( !_get_method )  _get_method = _ole_object.method( "hostOLE_get" );
    }

    _get_method->invoke( &result, params );

    result.write_text( &buffer );
}

//-----------------------------------------------------------Ole_file::get_record_key
/*
void Ole_file::get_record_key( Area& buffer, const Key& key )
{
}
*/
//--------------------------------------------------------------Ole_file::put_record

void Ole_file::put_record( const Const_area& record )
{
    Dyn_obj  result;  // dummy
    Dyn_obj* record_param = &_put_params[ 0 ];

    record_param->assign( SOS_NEW( Text_type( record.length() ) ), record.ptr() );  // Der SOS_NEW ist nicht besonders effizent

    if( !_put_method )  {
        try {
            _put_method = _ole_object.method( "Put" );
        } 
        catch( const Xc& ) {}
        if( !_put_method )  _put_method = _ole_object.method( "hostOLE_put" );
    }

    _put_method->invoke( &result, _put_params );
}

//-------------------------------------------------------------Ole_file::rewind
/*
void Ole_file::rewind( Key::Number )
{
}

//-------------------------------------------------------------------Ole_file::update

void Ole_file::update( const Const_area& record )
{
}

//----------------------------------------------------------------------Ole_file::del

void Ole_file::del()
{
}
*/


} //namespace sos

#endif

