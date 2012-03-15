// $Id: anyfile.cxx 12464 2006-12-26 10:48:38Z jz $

#include "precomp.h"
#include "../kram/sysdep.h"

#include <ctype.h>
#include <string.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosctype.h"
#include "../kram/sosopt.h"
#include "../kram/sosfield.h"
#include "../kram/dynobj.h" 
#include "../file/anyfile.h"
#include "../file/anyfile2.h"
#include "../file/sosdb.h"
#include "../kram/log.h"


namespace sos {

struct Any_file_param
{
                                Any_file_param          () : _zero_(this+1) {}

    Fill_zero                  _zero_;
    Sos_string                 _field_names;
    Sos_string                 _modify_field_names;
    Sos_string                 _key_names;
    Bool                       _write_null_as_empty;
    Bool                       _flat;
    Bool                       _with_groups;
    bool                       _log;
};

//---------------------------------------------------------------------------------------consts

DEFINE_SOS_STATIC_PTR( Any_file )
DEFINE_SOS_STATIC_PTR( Any_file_obj )

//---------------------------------------------------------------------------------default_type

const char Any_file_obj::_default_type[max_typename] = "file";

//-------------------------------------------------------------------------------------SOS_INIT

SOS_INIT( anyfile )
{
    init_file_types();
}

//-------------------------------------------------------------------------------------Any_file

Any_file::Any_file()
{
    create();
}

//-------------------------------------------------------------------------------Any_file::Any_file

Any_file::Any_file( const string& filename, Open_mode open_mode, const File_spec& file_spec )
{
    init_file_types();

    Sos_ptr<Any_file_obj> file = SOS_NEW_PTR( Any_file_obj );
    _file = +file;
    open( filename, open_mode, file_spec );
}

//-------------------------------------------------------------------------------Any_file::Any_file

Any_file::Any_file( const char* filename, Open_mode open_mode, const File_spec& file_spec )
{
    init_file_types();

    Sos_ptr<Any_file_obj> file = SOS_NEW_PTR( Any_file_obj );
    _file = +file;
    open( filename, open_mode, file_spec );
}

//------------------------------------------------------------------------------Any_file::~Any_file

Any_file::~Any_file() {}

//---------------------------------------------------------------------------------Any_file::create

void Any_file::create()
{
    if( !_file )
    {
        Sos_ptr<Any_file_obj> file = SOS_NEW_PTR( Any_file_obj );
        _file = +file;
    }
}

void Any_file::open             ( const char*       fn, Open_mode om, const File_spec& fs ) { open( Sos_string( fn ), om, fs ); }
void Any_file::prepare_open     ( const Sos_string& fn, Open_mode om, const File_spec& fs ) { create(); _file->prepare_open( fn, om, fs ); }
void Any_file::clear_parameters ()                                                          { _file->clear_parameters(); }
void Any_file::bind_parameters  ( const Record_type* t, const Byte* p )                     { _file->bind_parameters( (Record_type*)t, p ); }
void Any_file::set_parameter    ( int i, int o )                                            { _file->set_parameter( i, Dyn_obj(o) ); }
void Any_file::set_parameter    ( int i, const string& o )                                  { _file->set_parameter( i, Dyn_obj(o) ); }
void Any_file::set_parameter    ( int i, const Dyn_obj& o )                                 { _file->set_parameter( i, o ); }
void Any_file::bind_parameter   ( int i, Dyn_obj* o )                                       { _file->bind_parameter( i, o ); }

void Any_file::open             ()                                                          { _file->open(); }
void Any_file::close            ( Close_mode close_mode  )                                  { if( _file )  _file->close( close_mode ); }
void Any_file::destroy          ()                                                          { _file = NULL; }
void Any_file::execute          ()                                                          { _file->execute(); }

void Any_file::insert           ( const Const_area& record )                                { _file->insert( record ); }
void Any_file::insert_key       ( const Const_area& record, const Key& key )                { _file->insert_key( record, key ); }
void Any_file::store            ( const Const_area& record )                                { _file->store( record ); }
void Any_file::store_key        ( const Const_area& record, const Key& key )                { _file->store_key( record, key ); }
void Any_file::update           ( const Const_area& record )                                { _file->update( record ); }
void Any_file::update_direct    ( const Const_area& record )                                { _file->update_direct( record ); }
void Any_file::set              ( const Key& key )                                          { _file->set( key ); }
void Any_file::rewind           ( Key::Number n )                                           { _file->rewind( n ); }
void Any_file::del              ()                                                          { _file->del(); }
void Any_file::del              ( const Key& key )                                          { _file->del( key ); }
void Any_file::lock             ( const Key& key, Record_lock lock )                        { _file->lock( key, lock ); }
Bool Any_file::opened           ()                                                          { return _file != NULL && _file->_opened; }

Const_area         Any_file::current_key             ()                                     { return _file->current_key(); }
Record_position    Any_file::key_position            ( Key::Number n )                      { return _file->key_position( n ); }
Record_length      Any_file::key_length              ( Key::Number n )                      { return _file->key_length( n ); }
Bool               Any_file::key_in_record           ()                                     { return _file->key_in_record(); }
const File_spec&   Any_file::spec                    ()                                     { return _file->spec(); }
File_info          Any_file::info                    ()                                     { return _file->info(); }
string             Any_file::name                    () const                               { return zschimmer::remove_password( _file->_filename ); }
int4               Any_file::record_count            () const                               { return _file->_record_count; }
        void       Any_file::put                     ( const Const_area& area )                        { _file->put_record( area ); }
        Bool       Any_file::eof                     ()                                     { return _file->eof(); }
        void       Any_file::get                     ( Area* area_ptr )                                { _file->get_record( *area_ptr ); }
        void       Any_file::get                     ( Area* area_ptr, Record_lock record_lock )       { _file->get_record_lock( *area_ptr, record_lock ); }
        void       Any_file::get_until               ( Area* area_ptr, const Const_area& until_key )   { _file->get_until( area_ptr, until_key ); }
Record             Any_file::get_record              ()                                                { return _file->get_dyn_obj(); }
        void       Any_file::get_key                 ( Area* area_ptr, const Key& key          )       { _file->get_record_key( *area_ptr, key );         }
void               Any_file::get_position            ( Area* pos, const Const_area* until_key  )       { _file->get_position( pos, until_key ); }
        string     Any_file::get_string              ()                                                { return _file->get_string(); }


//-------------------------------------------------------------------------------Any_file::open

void Any_file::open( const Sos_string& filename, Open_mode open_mode, const File_spec& file_spec )
{
    create(); 
    _file->open( filename, open_mode, file_spec );
}

//-------------------------------------------------------------Any_file::identifier_quote_begin

Sos_string Any_file::identifier_quote_begin()
{
    if( !_file  ||  !_file->_f  )   throw_xc( "SOS-1164", "identifier_quote_begin" );
    if( !_file->_f->obj_is_type( tc_Sos_database_file ) )  return empty_string;
    return ((Sos_database_file*)+_file->_f)->identifier_quote_begin();
}

//---------------------------------------------------------------Any_file::identifier_quote_end

Sos_string Any_file::identifier_quote_end()
{
    if( !_file  ||  !_file->_f  )   throw_xc( "SOS-1164", "identifier_quote_end" );
    if( !_file->_f->obj_is_type( tc_Sos_database_file ) )  return empty_string;
    return ((Sos_database_file*)+_file->_f)->identifier_quote_end();
}

//------------------------------------------------------------------------Any_file::date_format

Sos_string Any_file::date_format()
{
    if( !_file  ||  !_file->_f  )   throw_xc( "SOS-1164", "date_format" );
    if( !_file->_f->obj_is_type( tc_Sos_database_file ) )  return empty_string;
    return ((Sos_database_file*)+_file->_f)->date_format();
}

//-------------------------------------------------------------------Any_file::date_time_format

Sos_string Any_file::date_time_format()
{
    if( !_file  ||  !_file->_f  )   throw_xc( "SOS-1164", "date_time_format" );
    if( !_file->_f->obj_is_type( tc_Sos_database_file ) )  return empty_string;
    return ((Sos_database_file*)+_file->_f)->date_time_format();
}

//---------------------------------------------------------------------------Any_file::filename

string Any_file::filename()
{
    if( !_file )   throw_xc( "SOS-1164", "filename" );
    return _file->_filename;
}

//-----------------------------------------------------------Any_file::sos_database_session_or_null

Sos_database_session* Any_file::sos_database_session_or_null()
{
    if( !_file  ||  !_file->_f  )   throw_xc( "SOS-1164", __FUNCTION__ );
    if( !_file->_f->obj_is_type( tc_Sos_database_file ) )  return NULL;
    return ((Sos_database_file*)+_file->_f)->_session;
}

//-------------------------------------------------------------------Any_file::sos_database_session

Sos_database_session* Any_file::sos_database_session()
{
    if( !_file  ||  !_file->_f  )   throw_xc( "SOS-1164", "sos_database_session" );
    if( !_file->_f->obj_is_type( tc_Sos_database_file ) )  zschimmer::throw_xc( __FUNCTION__ );
    Sos_database_session* session = ((Sos_database_file*)+_file->_f)->_session;
    if( !session )  throw_xc( "SOS-1464", filename() );
    return session;
}

//------------------------------------------------------------------------------Any_file::dbms_kind

Dbms_kind Any_file::dbms_kind()
{
    Sos_database_session* session = sos_database_session_or_null();
    return session? session->_dbms : dbms_unknown;
}

//------------------------------------------------------------------------------Any_file::dbms_name

string Any_file::dbms_name()
{
    Sos_database_session* session = sos_database_session_or_null();
    return session? session->_dbms_name : "";
}

//--------------------------------------------------------------------Any_file::is_transaction_used

bool Any_file::is_transaction_used()
{
    Sos_database_session* session = sos_database_session_or_null();
    return session && session->is_transaction_used();
}

//-----------------------------------------------------------------Any_file::need_commit_or_rollback

bool Any_file::need_commit_or_rollback()
{
    Sos_database_session* session = sos_database_session_or_null();
    return session && session->need_commit_or_rollback();
}

//----------------------------------------------------------------------Any_file::create_record

Record Any_file::create_record()
{
    if( !_file  ||  !_file->_f  )   throw_xc( "SOS-1164", "create_record" );

    return Record( _file->_spec._field_type_ptr );
}

//-------------------------------------------------------------------Any_file_obj::default_type
/*
void Any_file_obj::default_type( const char* new_type )
{
    // man kann noch ueberpruefen, ob es diesen new_type ueberhaupt gibt!

    if( strlen(new_type) < max_typename )  throw_too_long_error();

    strcpy( _default_type, new_type );
}
*/
//-------------------------------------------------------------------Any_file_obj::default_type

const char* Any_file_obj::default_type()
{
    return _default_type;
}

//------------------------------------------------------------------parse_file_type_and_options

static void parse_file_type_and_options( const Sos_string& filename,
                                         Sos_string* type_name_ptr,
                                         int* parameter_start_ptr )
{
    int type_name_length;

    if( length( filename ) == 0  /*||  filename[ 0 ] == '-'*/ ) {
        type_name_length = 0;
    } else {
        const char* p = c_str( filename );
        while( *p && !isspace( *p ) && *p != ':' && *p != '|' )  p++;
        type_name_length = p - c_str( filename );
    }
    *type_name_ptr = as_string( c_str( filename ), type_name_length );

    if( type_name_length == 1 )  type_name_length = 0; // zur Erkennung von DOS-Laufwerksbuchstaben
                                                       // d.h. File-Typen müssen mindestens 2-buchstabig sein!
    if( !type_name_length || empty( c_str( filename ) + type_name_length ) )  // Nur Dateiname?
    {
        *type_name_ptr = "file";                                 // Default-Dateityp
        *parameter_start_ptr = 0;
        return;
    }

    if( stricmp( c_str( *type_name_ptr ), "SELECT" ) == 0 ) {
        *type_name_ptr = "sossql";
        *parameter_start_ptr = 0;
        return;
    }

    // Objektparameter:
    const char* p = c_str( filename ) + type_name_length;
    while( sos_isspace( *p ) )  p++;
    if   ( *p == ':' )  p++;   // trennt Objekttyp von den Parametern
    while( sos_isspace( *p ) )  p++;
    *parameter_start_ptr = p - c_str( filename );  // = as_string( p, call_length - ( p - p0 ) );
}

//---------------------------------------------------------------------------------remove_file

void remove_file( const char* filename )
{
   Abs_file_type* ft_ptr = 0;
   Sos_string     type;
   int            parameter_start;

   parse_file_type_and_options( filename, &type, &parameter_start );

   ft_ptr = Abs_file_type::_lookup( c_str( type ) );

   if ( !ft_ptr )  throw_wrong_type_error( "D101" );

   ft_ptr->erase( filename + parameter_start );
}

//---------------------------------------------------------------------------------rename_file

void rename_file( const char* old_name, const char* new_name )
{
   Abs_file_type* ft_ptr = 0;
   Sos_string     type;
   int            parameter_start;

   parse_file_type_and_options( old_name, &type, &parameter_start );
   ft_ptr = Abs_file_type::_lookup( c_str( type ) );

   if( !ft_ptr )  throw_wrong_type_error( "D101" );

   ft_ptr->rename( old_name + parameter_start, new_name );
}

//-------------------------------------------------------------------Any_file_obj::Any_file_obj

Any_file_obj::Any_file_obj()
:
    _opened      ( false ),
    _prepared    ( false ),
    _prepare_open_not_implemented(false),
    _f           ( 0 ),
    _record_count( 0 )
{
    obj_const_name( "Any_file_obj" );
}

//-------------------------------------------------------------------Any_file_obj::Any_file_obj

Any_file_obj::Any_file_obj( const char* filename, Open_mode open_mode, const File_spec& file_spec )
:
    _opened ( false ),
    _prepared( false ),
    _prepare_open_not_implemented(false),
    _f      ( 0 ),
    _record_count( 0 )
{
    open( filename, open_mode, file_spec );
}

//------------------------------------------------------------------Any_file_obj::~Any_file_obj

Any_file_obj::~Any_file_obj()
{
    try {
        close ( /*_opened ? close_error : close_normal*/ );
        SOS_DELETE( _f );
    }
    catch( const exception& x )
    {
        LOG( "~Anyfile: " << x << '\n' );
    }
/* jz 22.6.2002
    catch(...) {
        LOG( "~Anyfile: Unbekannte Exception\n" );
    }
*/
}

//---------------------------------------------------------------------Any_file_obj::_obj_print

void Any_file_obj::_obj_print( ostream* s ) const
{
    *s << "Datei " << _filename;
    if( obj_ref_count() != 1 )  *s << ", ref=" << obj_ref_count();
}

//---------------------------------------------------------------Any_file_obj::select_file_type

void Any_file_obj::select_file_type( Any_file_param* )
{
    Abs_file_type*   ft_ptr = 0;

    parse_file_type_and_options( _filename, &_type_name, &_parameter_start );
    ft_ptr = Abs_file_type::_lookup( c_str( _type_name ) );

    if( !ft_ptr ) {
        ft_ptr = Abs_file_type::_lookup( "object" );
        if( !ft_ptr )  throw_wrong_type_error( "D101" );
        _parameter_start = 0;           // für Sos_object wiederherstellen
    }

    _f = ft_ptr->create_base_file();
}

//------------------------------------------------------------Any_file_obj::callers_object_type

void Any_file_obj::callers_object_type( const Sos_ptr<Record_type>& t )
{
    if( _spec._field_type_ptr )  throw_xc( "SOS-1211", c_str( _spec._field_type_ptr->name() ) );
    _spec._field_type_ptr = +t;

    _open_mode = Open_mode( _open_mode | binary );
}

//--------------------------------------------------------------Any_file_obj::callers_key_descr

void Any_file_obj::callers_key_descr( const Sos_ptr<Field_descr>& f )
{
    if( _spec.key_specs()[ 0 ]._field_descr_ptr )  throw_xc( "SOS-1212", _spec.key_specs()[ 0 ]._field_descr_ptr->name() );
    if( !f->type_ptr() )  throw_xc( "SOS-1255" );  // jz 960411 (?)
    _spec._key_specs._key_spec._field_descr_ptr = +f;
    if( !_spec._key_specs._key_spec._field_descr_ptr->type_ptr() )  throw_xc( "SOS-1255" );  // jz 960411 (?)
}

//------------------------------------------------------------------Any_file_obj::modify_fields

void Any_file_obj::modify_fields( Any_file_param* param )
{
    if( _spec._field_type_ptr ) 
    {
        if( param->_flat ) {
            Sos_ptr<Record_type> t;
            t = flattend_record_type( _spec._field_type_ptr, param->_with_groups );
            _spec._field_type_ptr = +t;

            if( _spec.key_specs()[ 0 ]._field_descr_ptr ) {
                Sos_ptr<Field_descr> f;
                Sos_ptr<Record_type> t;
                f = Field_descr::create( *_spec.key_specs()[ 0 ]._field_descr_ptr );
                t = flattend_record_type( SOS_CAST( Record_type, f->type_ptr() ), param->_with_groups );
                f->_type_ptr = t;
                _spec._key_specs._key_spec._field_descr_ptr = +f;
            }
        }

        if( !empty( param->_field_names ) ) {
            Sos_ptr<Record_type> t = record_type_of_selected_fields( _spec._field_type_ptr, param->_field_names, "-fields=" );
            _spec._field_type_ptr = +t;
            param->_field_names = "";
        }

        if( !empty( param->_modify_field_names ) ) {
            Sos_ptr<Record_type> t = modified_fields( _spec._field_type_ptr, param->_modify_field_names );
            _spec._field_type_ptr = +t;
            param->_modify_field_names = "";
        }


        if( param->_key_names != ""  &&  _spec._field_type_ptr ) {
            callers_key_descr( key_field_descr_by_name( _spec._field_type_ptr, param->_key_names ) );
            param->_key_names = "";
        }

        if( param->_write_null_as_empty ) {
            Sos_ptr<Record_type> t = copy_record_type( _spec._field_type_ptr );
            _spec._field_type_ptr = +t;
            for( int i = 0; i < _spec._field_type_ptr->field_count(); i++ ) {
                _spec._field_type_ptr->field_descr_ptr( i )->write_null_as_empty( true );
            }
            param->_write_null_as_empty = false;
        }
    }
}

//-------------------------------------------------------------------------------------set_flag
/*
void set_flag( int* p, int flag, Bool set )
{
    if( set )  *p |= flag;
         else  *p &= ~flag;
}

//-------------------------------------------------------------------------------------set_flag

inline void Any_file_obj::set_open_mode( int flag, Bool set )
{
    set_flag( (int*)_open_mode, flag, set );
}
*/
//----------------------------------------------------Any_file_obj::_prepare_open_preprocessing

void Any_file_obj::prepare_open_preprocessing( Any_file_param* param )
{
    Sos_string value;

    param->_with_groups = false;

    {
        Open_mode opt_open_mode = (Open_mode)0;

        for( Sos_option_iterator o ( _filename ); !o.end(); o.next() )
        {
            if( o.flag      ( "in" ) )          if( o.set() )  opt_open_mode = Open_mode( opt_open_mode | in  ); else _open_mode = Open_mode( _open_mode & ~in  );
            else
            if( o.flag      ( "out" ) )         if( o.set() )  opt_open_mode = Open_mode( opt_open_mode | out ); else _open_mode = Open_mode( _open_mode & ~out );
            else
            if( o.flag      ( "seq" ) )         _open_mode = Open_mode( o.set()? _open_mode | seq    : _open_mode & ~seq );
            else
            if( o.flag      ( "binary" ) )      _open_mode = Open_mode( o.set()? _open_mode | binary : _open_mode & ~binary );
            else
            if( o.flag      ( "trunc" ) )       _open_mode = Open_mode( o.set()? _open_mode | trunc  : _open_mode & ~trunc );
            else
            if( o.flag      ( "append" ) )      _open_mode = Open_mode( o.set()? _open_mode | app    : _open_mode & ~app );
            else
            if( o.flag      ( "dupkey" ) )      _spec._key_specs._key_spec._duplicate = o.set();
            else
            if( o.with_value( "key-length" )
             || o.with_value( "kl"         ) )  _spec._key_specs._key_spec._key_length = as_uint( o.value() );
            else
            if( o.with_value( "key-position" )
             || o.with_value( "kp"         ) )  _spec._key_specs._key_spec._key_position = as_uint( o.value() );
            else
            if( o.with_value( "modify-fields")) param->_modify_field_names = o.value();
            else
            if( o.with_value( "fields"     ) )  param->_field_names = o.value();
            else
            if( o.with_value( "key"        ) )  param->_key_names = o.value();
            else
          //if( o.flag      ( "read-empty-as-null" ) )   param->_read_empty_as_null = o.set(); 
          //else
            if( o.flag      ( "write-null-as-empty" ) )  param->_write_null_as_empty = o.set();
            else
            if( o.flag      ( "flat"          ) )  param->_flat = o.set();   // Datensatz flachklopfen (Cobol)
            else
            if( o.flag      ( "group-fields"  ) )  param->_with_groups = o.set();   // Mit Gruppenfelder (Cobol), nur bei -flat
            else
            if( o.flag      ( "log"           ) )  param->_log = o.set();  
            else
				if( o.param() )                     { _filename = o.rest();  break; }
            else
            throw_sos_option_error( o );
        }

        if( opt_open_mode & inout ) {
            if( (opt_open_mode & inout) == in  && (_open_mode & inout) == out )  throw_xc( "SOS-1201" );
            if( (opt_open_mode & inout) == out && (_open_mode & inout) == in  )  throw_xc( "SOS-1202" );
            _open_mode = Open_mode( ( _open_mode & ~inout ) | opt_open_mode );
        }

        _open_mode = Open_mode( _open_mode | ( opt_open_mode & seq ) );

        modify_fields( param );
    }

    select_file_type( param );

    _f->_any_file_ptr = this;
    _f->any_file_ptr( this );
}

//------------------------------------------------------------------Any_file_obj::preprare_open

void Any_file_obj::prepare_open( const Sos_string& filename, Open_mode open_mode, const File_spec& file_spec )
{
    Any_file_param param;

    _filename  = filename;
    _open_mode = open_mode;
    _spec      = file_spec;

    _prepare_open( &param );
}

//------------------------------------------------------------------Any_file_obj::preprare_open

void Any_file_obj::_prepare_open( Any_file_param* param )
{
    Log_indent _indent;

    Sos_ptr<Abs_file>    file;
    Sos_ptr<Record_type> callers_type;

    if( _prepared ) {
        SOS_DELETE( _f );
        _param_record_type = NULL;
        _prepared = false;
    }

    try {
        prepare_open_preprocessing( param );
    

        if( param->_log )  LOG( "Any_file::prepare( \"" << _filename << "\" )\n" );


        callers_type = +_spec._field_type_ptr;
        if( callers_type )  _spec._field_type_is_from_caller = true;  // Damit Dateityp com _field_type_ptr unterdrückt, jz 18.2.00

        file = _f;                      // Abs_file halten, falls new_file() gerufen wird
        _f->prepare_open( c_str( _filename ) + _parameter_start, _open_mode, _spec );
        file = 0;


        if( callers_type  &&  callers_type != _spec._field_type_ptr )
        {
            // Aufrufer und Datei haben verschiedene Satzbeschreibungen, also Konverter einfügen:
            //LOG( "Any_file_obj::_prepare_open: Verschiedene Satzbeschreibungen. Aufrufer: " << *callers_type << ", Dateityp: " << _spec._field_type_ptr << '\n' );
            throw_xc( "SOS-1232" );     // Dateityp convert einfügen
/*
            LOG( "Any_file::prepare_open fügt convert ein vor " << filename << '\n' );

            Sos_ptr<Any_file_obj> any_file = SOS_NEW_PTR( Any_file_obj );
            any_file->open( "convert:", this );
            handle->_file = any_file;
*/
        }

        prepare_open_postprocessing( param );

        _prepared = true;
    }
    catch( Xc_base& x )
    {
        throw;
    }
}

//----------------------------------------------------------------Any_file_obj::build_key_type

void Any_file_obj::build_key_type( Record_type* key_type, Record_type* record_type, uint offset )
{
    for( int i = 0; i < record_type->field_count(); i++ ) 
    {
        Field_descr* f = record_type->field_descr_ptr( i );

        if( f ) 
        {
            uint off = offset + f->offset();

            if( off >= _f->_key_pos
             && off + f->type().field_size() <= _f->_key_pos + _f->_key_len )
            {
                Sos_ptr<Field_descr> f2 = SOS_NEW( Field_descr( f->type_ptr(),
                                                                f->name(),
                                                                off - _f->_key_pos ) );
                key_type->add_field( f2 );
            }
            else
            if( f->type_ptr()->obj_is_type( tc_Record_type ) ) 
            {
                build_key_type( key_type, ((Record_type*)f->type_ptr()), 
                                off - ((Record_type*)f->type_ptr())->_offset_base );
            }
        }
    }
}

//----------------------------------------------------Any_file_obj::prepare_open_postprocessing

void Any_file_obj::prepare_open_postprocessing( Any_file_param* param )
{
    modify_fields( param );

    // Alle Parameter bearbeitet?
    if( !empty( param->_field_names        ) )  throw_xc( "SOS-1193", "-fields=" );
    if( !empty( param->_modify_field_names ) )  throw_xc( "SOS-1193", "-modify-fields=" );
    if( !empty( param->_key_names          ) )  throw_xc( "SOS-1214", "-key=" );


    // Wenn Schlüsselbeschreibung fehlt, aber Satzbeschreibung und keypos&keylen da sind,
    // kann die Schlüsselbeschreibung aufgebaut werden.
    // Überlappende Felder im Schlüssel sollten zu einem FEHLER führen!?
    // Wenn Felder den Schlüssel nicht vollständig abdecken => Fehler!?

    if( _spec._field_type_ptr
     && !_spec.key_specs()._key_spec.field_descr_ptr() )
    {
        if( _f->_key_len  &&  _f->_key_pos >= 0 ) {
            Sos_ptr<Record_type> key_type = Record_type::create();

            build_key_type( key_type, _spec._field_type_ptr, 0 );
            if( key_type->field_count() ) {
                const char* name = "key";
                if( key_type->field_count() == 1 )  name = key_type->field_descr_ptr( 0 )->name();
                Sos_ptr<Field_descr> p = SOS_NEW( Field_descr( +key_type, name, _f->_key_pos ) );
                _spec._key_specs._key_spec._field_descr_ptr = +p;

                if( log_ptr ) {
                    *log_ptr << "kl=" << _f->_key_len << " kp=" << _f->_key_pos << " key=";
                    Text_format format;
                    format.separator( ',' );
                    key_type->print_field_names( log_ptr, format );
                    *log_ptr << endl;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------Any_file_obj::new_file

void Any_file_obj::new_file( Any_file* any_file )
{
    _spec                         = any_file->_file->_spec;
    _filename                     = any_file->_file->_filename;
    _parameter_start              = any_file->_file->_parameter_start;
    _opened                       = any_file->_file->_opened;
    _prepare_open_not_implemented = any_file->_file->_prepare_open_not_implemented;

    any_file->_file->_opened = false;   // damit zweites Any_file_obj die Datei nicht schließt

    Sos_ptr<Abs_file> f = any_file->_file->_f;
    _f = f;

    _f->_any_file_ptr = this;
    _f->any_file_ptr( this );

}

//---------------------------------------------------------------Any_file_obj::clear_parameters

void Any_file_obj::clear_parameters()
{
    _param_record_type = NULL;
}

//----------------------------------------------------------------Any_file_obj::bind_parameters

void Any_file_obj::bind_parameters( const Sos_ptr<Record_type>& t, const Byte* p )
{
    _param_record_type = NULL;

    if( !this  ||  !_f  ) {
        if( !t )  return;
        throw_xc( "SOS-1164", "bind_parameters" );
    }

    if( !_prepared ) {
        if( !t )  return;
        throw_xc( "SOS-1252", "bind_parameters" );
    }

    _f->bind_parameters( t, p );
}

//------------------------------------------------------------------Any_file_obj::set_parameter

void Any_file_obj::set_parameter( int i, const Dyn_obj& param )
{
    if( !this  ||  !_f  )   throw_xc( "SOS-1164", "set_parameter" );
    if( !_prepared )  throw_xc( "SOS-1252", "set_parameter" );

    if( !_param_record_type )  _param_record_type = Record_type::create();

    if( _param_record_type->field_count() == i-1 ) 
    {
        Sos_ptr<Field_descr> field = SOS_NEW( Field_descr( param.type(), "param" ) );
        field->add_to( _param_record_type );
        _param_record.resize_min( _param_record_type->field_size() );
        _param_record.length( _param_record_type->field_size() );
    } 
    else 
    {
        if( _param_record_type->field_count() < i-1 )  throw_xc( "SOS-1433", i );
        if( _param_record_type->field_descr_ptr(i-1)->type_ptr() != param.type() )  throw_xc( "SOS-1434", i );
    }

    Field_descr* f = _param_record_type->field_descr_ptr(i-1);

    memcpy( f->ptr( _param_record.byte_ptr() ), param.ptr(), f->type_ptr()->field_size() );

/*  Baustelle: Feld in Record_type ändern, sollte nach sosfield.cxx. 
    Größere Alignments der Felder werden noch nicht berücksichtigt.

    Solange kann set_parameter() den Typ (und damit die Feldlänge!) nicht ändern.
    Für wiederholtes Setzen der Parameter bind_parameter() sollte also bind_parameter() verwendet werden.

    while( _param_record_type->field_count() < i )  _param_record_type->add_field( NULL );

    Field_descr* f = _param_record_type->field_descr_ptr(i-1);
    Field_type*  t = f? f->type_ptr() : NULL;

    if( t  &&  t->field_size() == param.type()->field_size()  &&  t->alignment() == param.type()->alignment() )
    {
        _param_record_type->_field_descr_array[ i-1 ] = SOS_NEW( Field_descr( param->type(), "param", f->offset() ) );
    }
    else
    {
        int old_offset = f->_offset;
        int old_size   = t->field_size();

        _param_record_type->_field_descr_array[ i-1 ] = SOS_NEW( Field_descr( param->type(), "param", round_up( f->offset(), param.type()->alignment() ) );
        f = _param_record_type->field_descr_ptr(i-1);

        int diff = ( f->offset() + f->type_ptr()->field_size() ) - ( old_offset + old_size ) ;
        if( diff > 0 )
        {
            Dynamic_area buffer;
            int j;
            int s = 0;

            for( j = i; j < _param_record_type->field_count(); j++ )  s = max( s, _param_record_type->field_descr_ptr(j)->type_ptr()->field_size(); }

            for( j = _param_record_type->field_count() - 1; j >= i; j-- )
            {
                f = _param_record_type->field_descr_ptr(j-1);
                memmove( f->ptr( _param_record.byte_ptr() ) + diff, f->const_ptr( _param_record.byte_ptr() ), f->type_ptr()->field_size() );
            }
        }
    }
*/
   
}

//-----------------------------------------------------------------Any_file_obj::bind_parameter

void Any_file_obj::bind_parameter( int i, Dyn_obj* param )
{
    if( !this  ||  !_f  )   throw_xc( "SOS-1164", "bind_parameter" );
    if( !_prepared )  throw_xc( "SOS-1252", "bind_parameter" );

    if( !_param_record_type )  _param_record_type = Record_type::create();

    while( _param_record_type->field_count() < i - 1 )  _param_record_type->add_field( NULL );

    if( _param_record_type->field_count() < i ) 
    {
        _param_record_type->add_field( param->type(), "param", (long)param->ptr() );
    } 
    else 
    {
        _param_record_type->_field_descr_array[ i - 1 ] = SOS_NEW( Field_descr( param->type(), "param", (long)param->ptr() ) );
    }
}

//---------------------------------------------------------------------------Any_file_obj::open

void Any_file_obj::open( const char* filename, Open_mode open_mode )
{
    open( Sos_string( filename ), open_mode );
}

//---------------------------------------------------------------------------Any_file_obj::open

void Any_file_obj::open( const char* filename, Open_mode open_mode, const File_spec& spec )
{
    open( Sos_string( filename ), open_mode, spec );
}

//---------------------------------------------------------------------------Any_file_obj::open

void Any_file_obj::open( const Sos_string& filename, Open_mode open_mode )
{
    open( filename, open_mode, std_file_spec );
}

//---------------------------------------------------------------------------Any_file_obj::open

void Any_file_obj::open( const Sos_string& filename, Open_mode open_mode, const File_spec& file_spec )
{
    //LOGI( "Any_file::open( \"" << filename << "\" )\n" );
    Any_file_param param;

    //jz 6.8.97 Nu kan Any_file återvinnas.   if( _prepared )  throw_xc( "SOS-1254", "open" );
    if( _opened )  throw_xc( "SOS-1362", c_str( _filename ) );

    _filename  = filename;
    _open_mode = open_mode;
    _spec      = file_spec;

    _prepare_open( &param );

    if( !_opened ) {  // open() nicht von Abs_file::prepare_open() gerufen?
        try {
            _f->open( c_str( _filename ) + _parameter_start, _open_mode, _spec );
            _opened = true;
        }
        catch( Xc_base& x )
        {
            x.insert( _filename );
            throw;
        }
    }
}

//---------------------------------------------------------------------------Any_file_obj::open

void Any_file_obj::open()
{
    if( _opened ) {
        if( !_prepare_open_not_implemented )  throw_xc( "SOS-1253", "open" );
        // Abs_file::prepare_open() hat die Datei bereits geöffnet 
        return;
    }

    try {
        if( !_prepared )  throw_xc( "SOS-1252", "open" );

      //if( _param_record_type )  _f->bind_parameters( _param_record_type, (Byte*)0 );
        if( _param_record_type )  _f->bind_parameters( _param_record_type, _param_record.byte_ptr() );

        //jz 7.8.97 _f->open( c_str( _filename ) + _parameter_start, _open_mode, _spec );
        _f->open( "", _open_mode, _spec );

        _opened = true;
    }
    catch( Xc_base& x )
    {
        x.insert( _filename );
        throw;
    }
}

//------------------------------------------------------------------------Any_file_obj::execute

void Any_file_obj::execute()
{
    if( !_prepared )  throw_xc( "SOS-1252", "execute" );
    //jz 7.8.97 if( _opened    )  throw_xc( "SOS-1253", "execute" );
    //jz 7.8.97 _f->execute();
    //jz 7.8.97 Abs_file::execute() ist tot.

    // execute() kann wiederholt ohne close() gerufen werden, im Gegensatz zu open(), das ein close() verlangt
    if( _opened )  close( close_normal );    // close_cursor()!

    open(); //_f->open( empty_string, _open_mode, _spec );
}

//--------------------------------------------------------------------------Any_file_obj::close

void Any_file_obj::close( Close_mode close_mode )
{
    if( _f && _opened ) {
        _opened = false;
        //LOGI( "Any_file_obj::close: _f->close(" << (int)close_mode << ")\n" );
        _f->close( close_mode );
    }
}

//----------------------------------------------------------------------------Any_file_obj::eof

Bool Any_file_obj::eof()
{
    if (!_opened)   throw_xc( "SOS-1164" );

    if( _next_record_read )  return false;

    try {
        _f->get_record( _record );
    }
    catch( const Eof_error& )  { return true; }

    _next_record_read = true;
    return false;
}

//---------------------------------------------------------------------Any_file_obj::get_string

string Any_file_obj::get_string()
{
    if (!_opened)   throw_xc( "SOS-1164" );

    if( !_next_record_read )    // wenn nicht zuvor Any_file_obj::eof() gerufen
    {
        _f->get_record( _record );
    }

    _next_record_read = false;

    return as_string( _record );
}

//--------------------------------------------------------------------Any_file_obj::get_dyn_obj

Dyn_obj Any_file_obj::get_dyn_obj()
{
    if( _next_record_read ) { // wenn zuvor Any_file_obj::eof() gerufen
        _next_record_read = false;
    } else {
        get_record( _record );
    }

    return Dyn_obj( _spec._field_type_ptr, _record );
}

//---------------------------------------------------------------------Any_file_obj::get_record

void Any_file_obj::get_record( Area& area )
{
    // s.a. get_until()

    if( _next_record_read )  // wenn zuvor Any_file_obj::eof() gerufen
    {
        area.assign( _record );
        _next_record_read = false;
    }
    else
    {
        if (!_opened)   throw_xc( "SOS-1164" );
    
        area.length( 0 );
        _f->get_record( area );
    }
}

//----------------------------------------------------------------------Any_file_obj::get_until

void Any_file_obj::get_until( Area* buffer, const Const_area& until_key )
{
    if( _f->_key_len == 0  ||  _f->_key_pos < 0  )  throw_xc( "SOS-1380" );

    if( until_key.length() != _f->_key_len )  throw_xc( "SOS-1228", until_key.length(), _f->_key_len );


    if( _next_record_read )  // wenn zuvor Any_file_obj::eof() gerufen
    {
        buffer->assign( _record );
        _next_record_read = false;
    }
    else
    {
        if (!_opened)   throw_xc( "SOS-1164" );
    
        buffer->length( 0 );
        _f->get_until( buffer, until_key );
    }


    if( buffer->length() < _f->_key_pos + until_key.length() )  throw_xc( "SOS-1215", _record.length(), _f->_key_pos + until_key.length() );

    if( memcmp( buffer->byte_ptr() + _f->_key_pos, until_key.byte_ptr(), until_key.length() ) > 0 )  throw_eof_error( "SOS-1379" );
}

//---------------------------------------------------------------------Any_file_obj::get_record

void Any_file_obj::get_record_lock( Area& area, Record_lock lock )
{
    if (!_opened)   throw_xc( "SOS-1164" );  //raise( "NOTOPND", "D1?? not opened" );
    if( _next_record_read )  throw_xc( "get_record_lock", "eof() und get_lock() nicht kombinierbar" );
    area.length( 0 );
    _f->get_record_lock( area, lock );
}

//-----------------------------------------------------------------Any_file_obj::get_record_key

void Any_file_obj::get_record_key( Area& area, const Key& key )
{
    if (!_opened)   throw_xc( "SOS-1164" );

    _next_record_read = false;

    area.length( 0 );
    _f->get_record_key( area, key );
}

//---------------------------------------------------------------------Any_file_obj::get_record

void Any_file_obj::get_position( Area* buffer, const Const_area* until_key )
{
    if (!_opened)   throw_xc( "SOS-1164" );

    _next_record_read = false;
    _f->get_position( buffer, until_key );

    if( until_key ) {
        if( memcmp( buffer->ptr(), until_key->ptr(), until_key->length() ) > 0 )  throw_eof_error( "SOS-1379" );
    }
}

//---------------------------------------------------------------------Any_file_obj::put_record

void Any_file_obj::put_record( const Const_area& area )
{
    if (!_opened)   throw_xc( "SOS-1164" );  //raise( "NOTOPND", "D1?? not opened" );
    _next_record_read = false;
    _f->put_record( area );
}

//-------------------------------------------------------------------------Any_file_obj::insert

void Any_file_obj::insert( const Const_area& area )
{
    if (!_opened)   throw_xc( "SOS-1164" );
    _next_record_read = false;
    _f->insert( area );
}

//---------------------------------------------------------------------Any_file_obj::insert_key

void Any_file_obj::insert_key( const Const_area& area, const Key& key )
{
    if (!_opened)   throw_xc( "SOS-1164" );
    _next_record_read = false;
    _f->insert_key( area, key );
}

//--------------------------------------------------------------------------Any_file_obj::store

void Any_file_obj::store( const Const_area& area )
{
    if (!_opened)   throw_xc( "SOS-1164" );
    _next_record_read = false;
    _f->store( area );
}

//----------------------------------------------------------------------Any_file_obj::store_key

void Any_file_obj::store_key( const Const_area& area, const Key& key )
{
    if (!_opened)   throw_xc( "SOS-1164" );
    _next_record_read = false;
    _f->store_key( area, key );
}

//-------------------------------------------------------------------------Any_file_obj::update

void Any_file_obj::update( const Const_area& area )
{
    if (!_opened)   throw_xc( "SOS-1164" );
    _next_record_read = false;
    _f->update( area );
}

//------------------------------------------------------------------Any_file_obj::update_direct

void Any_file_obj::update_direct( const Const_area& area )
{
    if (!_opened)   throw_xc( "SOS-1164" );
    _next_record_read = false;
    _f->update_direct( area );
}

//----------------------------------------------------------------------------Any_file_obj::set

void Any_file_obj::set( const Key& key )
{
    if (!_opened)   throw_xc( "SOS-1164" );
    _next_record_read = false;
    _f->set( key );
}

//-------------------------------------------------------------------------Any_file_obj::rewind

void Any_file_obj::rewind( Key::Number key_number )
{
    if (!_opened)   throw_xc( "SOS-1164" );
    _next_record_read = false;
    _f->rewind( key_number );
}

//----------------------------------------------------------------------------Any_file_obj::del

void Any_file_obj::del ()
{
    if (!_opened)   throw_xc( "SOS-1164" );  //raise( "NOTOPND", "D1?? not opened" );
    _next_record_read = false;
    _f->del();
}

//----------------------------------------------------------------------------Any_file_obj::del

void Any_file_obj::del( const Key& key )
{
    if (!_opened)   throw_xc( "SOS-1164" );  //raise( "NOTOPND", "D1?? not opened" );
    _next_record_read = false;
    _f->del( key );
}

//------------------------------------------------------------------------Any_file_obj::execute
/*
void Any_file_obj::execute( const char* proc_name, const Dyn_obj& input, Dyn_obj* output )
{
    if (!_opened)   throw_xc( "SOS-1164" );  //raise( "NOTOPND", "D1?? not opened" );

    _f->execute( area );
}
*/
//---------------------------------------------------------------------------Any_file_obj::lock

void Any_file_obj::lock( const Key& key, Record_lock lock )
{
    if (!_opened)   throw_xc( "SOS-1164" );
    _next_record_read = false;
    _f->lock( key, lock );
}

//-------------------------------------------------------------------Any_file_obj::key_position

Const_area Any_file_obj::current_key()
{
    if (!_opened)   throw_xc( "SOS-1164" );  //raise( "NOTOPND", "D1?? not opened" );
    if( _next_record_read )  throw_xc( "get_record_lock", "eof() und current_key() nicht kombinierbar" );
    return _f->current_key();
}

//------------------------------------------------------------------Any_file_obj::key_in_record

Bool Any_file_obj::key_in_record()
{
    int kp = key_position();
    if( kp != -1  &&  key_length() )  return true;
    return false;
}

//-------------------------------------------------------------------Any_file_obj::key_position

Record_position Any_file_obj::key_position( Key::Number key_number )
{
    if( !_prepared  )   throw_xc( "SOS-1164" );  //raise( "NOTOPND", "D1?? not opened" );
    return _f->key_position( key_number );
}

//---------------------------------------------------------------------Any_file_obj::key_length

Record_length Any_file_obj::key_length( Key::Number key_number )
{
    if( !_prepared )   throw_xc( "SOS-1164" );  //raise( "NOTOPND", "D1?? not opened" );
    return _f->key_length( key_number );
}

//---------------------------------------------------------------------------Any_file_obj::info

File_info Any_file_obj::info()
{
    return _f->info();
}

//------------------------------------------------------------------------------------copy_file

int4 copy_file( const char* source_name, const char* dest_name, long count, long skip )
{
    return copy_file( Sos_string( source_name ), Sos_string( dest_name ), count, skip );
}

//------------------------------------------------------------------------------------copy_file

int4 copy_file( const Sos_string& source_name, const Sos_string& dest_name, long count, long skip )
{
    Any_file     source;
    Any_file     dest;
    File_spec    spec;
    Dynamic_area record;
    int4         no = 0;

    source.open ( source_name, File_base::Open_mode( File_base::in | File_base::seq ) );

    spec._field_type_ptr = source.spec()._field_type_ptr;
    dest.open( dest_name, File_base::out, spec );

    while( skip-- )  source.get( &record );    // Vorzeitiges EOF wird als Fehler gemeldet

    while(1) {
        if( count >= 0 )  if( count-- == 0 )  break;
        try {
            source.get( &record );
        }
        catch( const Eof_error& ) { break; }
        dest.put( record );
        no++;
    }

    dest.close();
    source.close();

    return no;
}

//-------------------------------------------------------------------------------file_as_string

Sos_string file_as_string( const Sos_string& filename, const char* newline )
{
    string result;

    Z_MUTEX( hostware_mutex )
    {
        Any_file file;

        file.open( filename, Any_file::in_seq );

        result = file_as_string( &file, newline );

        file.close();
    }

    return result;
}

//-------------------------------------------------------------------------------file_as_string

Sos_string file_as_string( Any_file* file, const char* newline )
{
    string result;

    Z_MUTEX( hostware_mutex )
    {
        const uint      max_line_length = 32*1024L;

        Dynamic_area    buffer;
        const int       newline_len = length( newline );

        buffer.allocate_min( 100*1024L );

        while(1) 
        {
            buffer.length( 0 );

            while( buffer.size() - buffer.length() >= max_line_length )
            {
                Area rest ( buffer.char_ptr() + buffer.length(), buffer.size() - buffer.length() - newline_len );

                try {
                    file->get( &rest );
                }
                catch( const Eof_error& ) { goto ENDE; }

                buffer.length( buffer.length() + rest.length() );
                buffer.append( newline, newline_len );
            }

            append( &result, buffer.char_ptr(), buffer.length() );
            buffer.allocate_min( 512*1024L );
        }

    ENDE:
        append( &result, buffer.char_ptr(), buffer.length() );
    }

    return result;
}

} //namespace sos
