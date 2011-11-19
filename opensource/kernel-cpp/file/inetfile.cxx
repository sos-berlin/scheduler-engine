//#define MODULE_NAME "inetfile"
// stdfile.cpp
// ®1997 Software- und Organisations-Service GmbH, Berlin
// 19.5.97                                                    Joacim Zschimmer

#include "precomp.h"

#if 0 //jz 12.7.01 MFC

#ifdef _MSC_VER       // Die Internet-Klassen der MFC werden genutzt

#include <afxinet.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/log.h"
#include "../file/absfile.h"


namespace sos {

// Nach xception.cxx:

void throw_and_delete( CException* mfc_x )
{
    Xc   x    ( "MFC-Error" );
    char text [ 300 ];

    mfc_x->GetErrorMessage( text, sizeof text );

    char* p = text + strlen( text );
    if( p > text  &&  p[-1] == '\n' )  p--;
    if( p > text  &&  p[-1] == '\r' )  p--;

    x.insert( text );
    mfc_x->Delete();

    throw x;
}






const int buffer_size = 4096;

struct Inet_file : Abs_file
{
                                Inet_file               ();
                               ~Inet_file               ();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode = close_normal );

  protected:
    virtual void                get_record              ( Area& );
    virtual void                put_record              ( const Const_area& );

  private:
    Fill_zero                  _zero_;
    CInternetSession*          _inet_session;
    CFtpConnection*            _ftp_conn;
  //CInternetFile*             _file;
    CStdioFile*                _file;
};


//----------------------------------------------------------------------statics

struct Inet_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "url"; }
    virtual const char*         alias_name              () const { return "ftp"; }

    virtual Sos_ptr<Abs_file>   create_base_file() const
    {
        Sos_ptr<Inet_file> f = SOS_NEW( Inet_file );
        return +f;
    }
};

const Inet_file_type  _inet_file_type;
const Abs_file_type&   inet_file_type = _inet_file_type;


//-----------------------------------------------------------Inet_file::Inet_file

Inet_file::Inet_file()
:
    _zero_ (this+1)
{
}

//----------------------------------------------------------Inet_file::~Inet_file

Inet_file::~Inet_file()
{
    close();
}

//---------------------------------------------------------------Inet_file::open

void Inet_file::open( const char* param, Open_mode open_mode, const File_spec& )
{
    Sos_string server_name;
    Sos_string user_name;
    Sos_string password;
    Sos_string filename;

    const char* user_name_ptr = NULL;
    const char* password_ptr  = NULL;


    for( Sos_option_iterator opt = param; !opt.end(); opt.next() )
    {
        if( opt.with_value( "server" ) )  server_name = opt.value(); 
        else
        if( opt.with_value( "user" ) )  { user_name = opt.value(); user_name_ptr = c_str( user_name ); }
        else
        if( opt.with_value( "pass" ) )  { password = opt.value(); password_ptr = c_str( password ); }
        else
        if( opt.param() )  { filename = opt.value(); }
        else throw_sos_option_error( opt );
    }

    try {
        _inet_session = new CInternetSession( "hostWare" );

        if( !empty( server_name ) ) 
        {
            _ftp_conn = _inet_session->GetFtpConnection( c_str(server_name), c_str(user_name_ptr), c_str(password_ptr) );

            _file = _ftp_conn->OpenFile( c_str(filename), ( (open_mode & out)? GENERIC_WRITE : GENERIC_READ ) | 
                                                          ( (open_mode & binary)? FTP_TRANSFER_TYPE_BINARY
                                                                                : FTP_TRANSFER_TYPE_ASCII ) );
        }
        else
        {
            _file = _inet_session->OpenURL( filename.c_str(), 
                                            1,   // context
                                            ( open_mode & binary? INTERNET_FLAG_TRANSFER_BINARY
                                                                : INTERNET_FLAG_TRANSFER_ASCII )
                                            /*| INTERNET_OPEN_FLAG_USE_EXISTING_CONNECT  flag ist unbekannt? */
                                            | INTERNET_FLAG_PASSIVE,
                                            NULL,  0 // headers
                                          );
        }
    }
    catch( CException* x )
    {
        throw_and_delete( x );
    }
}

//------------------------------------------------------------------------------Inet_file::close

void Inet_file::close( Close_mode mode )
{
    if( _file ) {
        if( mode == close_error )  _file->Abort(); 
                             else  _file->Close();
    }

    SOS_DELETE( _file );

    if( _ftp_conn )  _ftp_conn->Close();
    SOS_DELETE( _ftp_conn );

    SOS_DELETE( _inet_session );
}

//-------------------------------------------------------------------------Inet_file::get_record

void Inet_file::get_record( Area& buffer )
{
    if( buffer.resizable() )  buffer.allocate_min( buffer_size );

    try {
        buffer.length( _file->Read( buffer.ptr(), buffer.size() ) );
        if( buffer.length() == 0 )  throw_eof_error();
    }
    catch( CException* x )
    {
        throw_and_delete( x );
    }
}

//-------------------------------------------------------------------------Inet_file::put_record

void Inet_file::put_record( const Const_area& record )
{
    try {
        _file->Write( record.ptr(), record.length() );
    }
    catch( CException* x )
    {
        throw_and_delete( x );
    }
}


} //namespace sos

#endif

#endif