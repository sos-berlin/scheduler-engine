// $Id: java_File.cxx 11797 2005-11-09 11:57:30Z jz $

#include "hostjava_common.h"

#if 0
//#ifdef _DEBUG
//#   include "Debug/java.h/File.h"
//# else
//#   include "Release/java.h/File.h"
//#endif

using namespace zschimmer;
using namespace zschimmer::com;
using namespace zschimmer::java;
using namespace sos::hostjava;

//----------------------------------------------------------------------Factory_processor.construct

JNIEXPORT void JNICALL Java_sos_hostware_File_construct( JNIEnv* jenv, jobject jo )
{
    try
    {
        HRESULT             hr;
        Ihostware_file*     file = NULL;
        ptr<Ihostware_file> p;

        hr = p.CoCreateInstance( CLSID_File );
/*
        if( hr == CO_E_NOTINITIALIZED )
        {
            CoInitialize(NULL);         // CoUninitialize() wird nicht gerufen.
            hr = p.CoCreateInstance( CLSID_File );
        }
*/
        if( FAILED(hr) )  throw_com( hr, "CoCreateInstance File" );

        p.CopyTo( &file );

        jfieldID my_data_id = jenv->GetFieldID( jenv->GetObjectClass( jo ), "my_data", "J" );
        jenv->SetLongField( jo, my_data_id, (size_t)file );
    }
    catch( const exception&  x ) { set_java_exception( jenv, x ); }
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }
}

//-----------------------------------------------------------------------Factory_processor.destruct

JNIEXPORT void JNICALL Java_sos_hostware_File_destruct( JNIEnv* jenv, jobject jo )
{
    jfieldID my_data_id = NULL;


    Ihostware_file* file = (Ihostware_file*)get_my_data( jenv, jo, &my_data_id );

    if( file )
    {
        file->Release();
        jenv->SetLongField( jo, my_data_id, 0 );
    }
}

//----------------------------------------------------------------------------------------File.open

JNIEXPORT void JNICALL Java_sos_hostware_File_open( JNIEnv* jenv, jobject jo, jstring filename_j )
{
    try
    {
        HRESULT          hr;
        Bstr             filename_bstr;
        Ihostware_file*  file = (Ihostware_file*)get_my_data( jenv, jo );

        jstring_to_bstr( jenv, filename_j, &filename_bstr );

        hr = file->open( filename_bstr );
        if( FAILED(hr) )  throw_com( hr, "sos.hostware.File.open()" );
    }
    catch( const exception&  x ) { set_java_exception( jenv, x ); }
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }
}

//---------------------------------------------------------------------------------------File.close

JNIEXPORT void JNICALL Java_sos_hostware_File_close( JNIEnv* jenv, jobject jo )
{
    try
    {
        HRESULT          hr;
        Ihostware_file*  file = (Ihostware_file*)get_my_data( jenv, jo );

        hr = file->close();
        if( FAILED(hr) )  throw_com( hr, "sos.hostware.File.close()" );
    }
    catch( const exception&  x ) { set_java_exception( jenv, x ); }
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }
}

//------------------------------------------------------------------------------------File.get_line
/*
JNIEXPORT jstring JNICALL Java_sos_hostware_File_get_1line( JNIEnv* jenv, jobject jo )
{
}
*/
//------------------------------------------------------------------------------------File.put_line

JNIEXPORT void JNICALL Java_sos_hostware_File_put_1line( JNIEnv* jenv, jobject jo, jstring line_j )
{
    try
    {
        HRESULT         hr;
        Bstr            line_bstr;
        Ihostware_file* file = (Ihostware_file*)get_my_data( jenv, jo );

        jstring_to_bstr( jenv, line_j, &line_bstr );

        hr = file->put_line( line_bstr );
        if( FAILED(hr) )  throw_com( hr, "sos.hostware.File.put_line()" );
    }
    catch( const exception&  x ) { set_java_exception( jenv, x ); }
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }
}

//-------------------------------------------------------------------------------File.create_record
/*
JNIEXPORT jobject JNICALL Java_sos_hostware_File_create_1record( JNIEnv* jenv, jobject jo )
{
}

//-----------------------------------------------------------------------------------File.create_key

JNIEXPORT jobject JNICALL Java_sos_hostware_File_create_1key( JNIEnv* jenv, jobject jo )
{
}

//------------------------------------------------------------------------------------------File.get

JNIEXPORT jobject JNICALL Java_sos_hostware_File_get( JNIEnv* jenv, jobject jo )
{
}

//------------------------------------------------------------------------------------------File.put

JNIEXPORT void JNICALL Java_sos_hostware_File_put__Lsos_hostware_Record_2( JNIEnv* jenv, jobject jo, jobject record )
{
}

//------------------------------------------------------------------------------------------File.put

JNIEXPORT void JNICALL Java_sos_hostware_File_put__Ljava_lang_String_2( JNIEnv* jenv, jobject jo, jstring line )
{
}

//--------------------------------------------------------------------------------------File.set_key

JNIEXPORT void JNICALL Java_sos_hostware_File_set_1key( JNIEnv* jenv, jobject jo, jobject key )
{
}

//-----------------------------------------------------------------------------------File.delete_key

JNIEXPORT void JNICALL Java_sos_hostware_File_delete_1key( JNIEnv* jenv, jobject jo, jobject key )
{
}

//--------------------------------------------------------------------------------------File.get_key

JNIEXPORT jobject JNICALL Java_sos_hostware_File_get_1key( JNIEnv* jenv, jobject jo, jobject key )
{
}

//---------------------------------------------------------------------------------------File.insert

JNIEXPORT void JNICALL Java_sos_hostware_File_insert( JNIEnv* jenv, jobject jo, jobject record )
{
}

//---------------------------------------------------------------------------------------File.update

JNIEXPORT void JNICALL Java_sos_hostware_File_update( JNIEnv* jenv, jobject jo, jobject record )
{
}

//-------------------------------------------------------------------------------File.update_direct

JNIEXPORT void JNICALL Java_sos_hostware_File_update_1direct( JNIEnv* jenv, jobject jo, jobject record )
{
}

//---------------------------------------------------------------------------------------File.store

JNIEXPORT void JNICALL Java_sos_hostware_File_store( JNIEnv* jenv, jobject jo, jobject record )
{
}

//-----------------------------------------------------------------------------------------File.eof

JNIEXPORT jboolean JNICALL Java_sos_hostware_File_eof( JNIEnv* jenv, jobject jo )
{
}

//-----------------------------------------------------------------------------File.set_date_format

JNIEXPORT void JNICALL Java_sos_hostware_File_set_1date_1format( JNIEnv* jenv, jobject jo, jstring format )
{
}

//--------------------------------------------------------------------------File.set_decimal_symbol

JNIEXPORT void JNICALL Java_sos_hostware_File_set_1decimal_1symbol( JNIEnv* jenv, jobject jo, jstring symbol )
{
}

//----------------------------------------------------------------------------------File.field_name

JNIEXPORT jstring JNICALL Java_sos_hostware_File_field_1name( JNIEnv* jenv, jobject jo, jint nr )
{
}

//---------------------------------------------------------------------------------File.field_count

JNIEXPORT jint JNICALL Java_sos_hostware_File_field_1count( JNIEnv* jenv, jobject jo )
{
}

//-------------------------------------------------------------------------------------File.prepare

JNIEXPORT jstring JNICALL Java_sos_hostware_File_prepare( JNIEnv* jenv, jobject jo, jstring filename )
{
}

//-------------------------------------------------------------------------------File.set_parameter

JNIEXPORT void JNICALL Java_sos_hostware_File_set_1parameter( JNIEnv* jenv, jobject jo, jint nr, jobject value )
{
}

//-------------------------------------------------------------------------------------File.execute

JNIEXPORT void JNICALL Java_sos_hostware_File_execute( JNIEnv* jenv, jobject jo )
{
}

//------------------------------------------------------------------------------File.set_parameters

JNIEXPORT void JNICALL Java_sos_hostware_File_set_1parameters( JNIEnv* jenv, jobject jo, jobjectArray par_array )
{
}

//--------------------------------------------------------------------------------------File.opened

JNIEXPORT jboolean JNICALL Java_sos_hostware_File_opened( JNIEnv* jenv, jobject jo )
{
}

//--------------------------------------------------------------------------------------File.rewind

JNIEXPORT void JNICALL Java_sos_hostware_File_rewind( JNIEnv* jenv, jobject jo )
{
}

//------------------------------------------------------------------------File.set_date_time_format

JNIEXPORT void JNICALL Java_sos_hostware_File_set_1date_1time_1format( JNIEnv* jenv, jobject jo, jstring format )
{
}

//---------------------------------------------------------------------File.set_write_empty_as_null

JNIEXPORT void JNICALL Java_sos_hostware_File_set_1write_1empty_1as_1null( JNIEnv* jenv, jobject jo, jboolean b )
{
}

//--------------------------------------------------------------------------------File.close_cursor

JNIEXPORT void JNICALL Java_sos_hostware_File_close_1cursor( JNIEnv* jenv, jobject jo )
{
}

//-------------------------------------------------------------------------------------------------

*/
#endif
