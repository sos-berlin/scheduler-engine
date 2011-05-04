//#define MODULE_NAME "protfile"
// protfile.cpp
// Implementation ...

#include "precomp.h"
#if 0

#include "../kram/sysdep.h"
#include <string.h>

#include "../kram/sos.h"
#include "../kram/xception.h"
#include "../kram/log.h"
#include "../file/anyfile.h"

#include "protfile.h"

//--------------------------------------------------------------Prot_file_type

struct Prot_file_type : Abs_file_type
{
    Prot_file_type() : Abs_file_type() {};

    virtual const char* name() const { return "prot"; }
    virtual const char* alias_name() const { return ""; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Prot_file> f = SOS_NEW_PTR( Prot_file );
        return +f; 
    }
};

static Prot_file_type _Prot_file_type;

const Abs_file_type& Prot_file::static_file_type () {
  return _Prot_file_type;
};

//--------------------------------------------------------------Prot::report
void Prot::report ( const char* function,
                    const char* filename ) {
#ifndef __DLL__
  ostream* out_ptr;

  if ( log_ptr ) {
     out_ptr = log_ptr;
     *out_ptr << function << ": " << filename << endl;
  } else {
     //out_ptr = &cerr;
  };
#endif
};

void Prot::report ( const char* text ) {
#ifndef __DLL__
  ostream* out_ptr;

  if ( log_ptr ) {
     out_ptr = log_ptr;
     *out_ptr << text << endl;
  } else {
     //out_ptr = &cerr;
  };
#endif
};

#if 0
//-----------------------------------------------------------Prot_file::open
void Prot_file::open ( const char* filename,
                       Open_mode  open_mode ) {
    _filename = new char[strlen(filename)+1];
    strcpy( _filename, filename );

    report("OPEN", (char*) _filename);
    _f = new Any_file;
    _f->open( _filename, open_mode ); xc;
    report("OPEN done");

exceptions
};

//----------------------------------------------------------Prot_file::close
void Prot_file::close ( Close_mode close_mode ) {
    report("CLOSE", _filename);
    _f->close( close_mode ); delete (void*) _f; _f = 0; xc;
    report("CLOSE done");
exceptions
};


//----------------------------------------------------Prot_record_file::open
void Prot_record_file::open ( const char* filename,
                              Open_mode  open_mode ) {
    _filename = new char[strlen(filename)+1];
    strcpy( _filename, filename );

    report("OPEN", _filename);
    _f = new Any_record_file;
    _f->open( _filename, open_mode ); xc;
    report("OPEN done");

exceptions
};

//---------------------------------------------------Prot_record_file::close
void Prot_record_file::close ( Close_mode close_mode ) {
    report("CLOSE", _filename);
    _f->close( close_mode ); delete (void*) _f; _f = 0; xc;
    report("CLOSE done");
exceptions
};

//-----------------------------------------------------Prot_record_file::put_record
void Prot_record_file::put_record( Const_area area) {
    report("PUT", _filename);
    _f->put( area ); xc;
    report("PUT done");
exceptions
};

//-----------------------------------------------------Prot_record_file::get_record
void Prot_record_file::get_record( Area& area )
{
    report("GET", _filename);
    _f->get( area ); xc;
    report("GET done");
exceptions
};
#endif

//---------------------------------------------------Prot_file::open
void Prot_file::open ( const char* filename,
			       Open_mode         open_mode,
			       const File_spec&  file_spec )
{
    _filename = new char[strlen(filename)+1];
    strcpy( _filename, filename );

    report("OPEN", _filename);
    //_f = new Any_file;
    _f.open( _filename, open_mode, file_spec ); xc;
    report("OPEN done");

exceptions
};

//----------------------------------------------------Prot_file::close
void Prot_file::close ( Close_mode close_mode ) {
    report("CLOSE", _filename);
    _f.close( close_mode ); //delete (void*) _f; _f = 0; xc;
    report("CLOSE done");
exceptions
};

//----------------------------------------------------Prot_file::get_record

void Prot_file::get_record_lock (
	Area& area,
	Record_lock lock ) {
    report("GET", _filename);
    _f.get( area, lock ); xc;
    report("GET done");
exceptions
};

void Prot_file::get_record_key (
	Area& area,
	const Key& key
) {
    report("GETKEY", _filename);
    _f.get( area, key ); xc;
    report("GETKEY done");
exceptions
};

//-------------------------------------------------Prot_file::insert
void Prot_file::insert (
	//const void     *record,
	//Record_length   record_length,
	//int             key_number
	const Const_area& area
) {
    report("INSERT", _filename);
    _f.insert( area ); xc;
    report("INSERT done");
exceptions
};

//--------------------------------------------------Prot_file::store
void Prot_file::store (
	//const void     *record,
	//Record_length   record_length,
	//int             key_number
	const Const_area& area
) {
    report("STORE", _filename);
    _f.store( area ); xc;
    report("STORE done");
exceptions
};

//-------------------------------------------------Prot_file::update
void Prot_file::update (
	//const void     *record,
	//Record_length   record_length
	const Const_area& area
) {
    report("UPDATE", _filename);
    _f.update( area ); xc;
    report("UPDATE done");
exceptions
};

//----------------------------------------------------Prot_file::set
void Prot_file::set (
	//const void     *key,
	//int             key_number
	const Key& key
) {
    report("SET", _filename);
    _f.set( key ); xc;
    report("SET done");
exceptions
};

//-------------------------------------------------Prot_file::rewind
void Prot_file::rewind( Key::Number      key_number)
{
    report("REWIND", _filename);
    _f.rewind( key_number ); xc;
    report("REWIND done");
exceptions
};

//-----------------------------------------------------Prot_record_file::get_record

void Prot_file::get_record( Area& area )
{
    report("GET", _filename);
    _f.get( area ); xc;
    report("GET done");
exceptions
}

//----------------------------------------------------Prot_file::del
void Prot_file::del () {
    report("DEL", _filename);
    _f.del(); xc;
    report("DEL done");
exceptions
};

void Prot_file::del (
	//const void     *key,
	//int             key_number
	const Key& key
) {
    report("DEL", _filename);
    _f.del( key ); xc;
    report("DEL done");
exceptions
};

//-----------------------------------------------------Prot_file::lock
void Prot_file::lock (
	const Key& key,
	Record_lock lock
) {
    report("LOCK", _filename);
    _f.lock( key, lock ); xc;
    report("LOCK done");
exceptions
};

//-------------------------------------------Prot_file::key_position
Record_position Prot_file::key_position( Key::Number key_number ) {
    report("KEY_POSITION", _filename);
    return _f.key_position( key_number );
};

//---------------------------------------------Prot_file::key_length
Record_length   Prot_file::key_length( Key::Number key_number ) {
    report("KEY_LENGTH", _filename);
    return _f.key_length( key_number );
};

#endif
