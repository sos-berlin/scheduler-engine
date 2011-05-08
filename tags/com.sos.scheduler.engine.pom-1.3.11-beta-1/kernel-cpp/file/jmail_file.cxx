#include "precomp.h"
#include "../kram/sysdep.h"

#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../kram/sos_mail.h"
#include "../kram/sos_mail_jmail.h"
#include "../zschimmer/regex_class.h"


namespace sos {
using namespace std;
using zschimmer::vector_split;


//---------------------------------------------------------------------------------------Jmail_file

struct Jmail_file : Abs_file
{
                                Jmail_file              ();
                               ~Jmail_file              ();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );

  protected:
    void                        put_record              ( const Const_area& );

  private:

    Fill_zero                  _zero_;

    Sos_ptr<mail::Message>     _msg;
    string                     _content_type;
    string                     _encoding;
    string                     _data;
    string                     _attachment_filename;
};

//----------------------------------------------------------------------------------Jmail_file_type

struct Jmail_file_type : Abs_file_type
{
    virtual const char*         name                    () const  { return "mail"; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Jmail_file> f = SOS_NEW_PTR( Jmail_file() );
        return +f;
    }
};

const Jmail_file_type  _jmail_file_type;
const Abs_file_type&    jmail_file_type = _jmail_file_type;

//--------------------------------------------------------------------------------------------jmail

namespace jmail_file {


//-------------------------------------------------------------------------------------------------

}; // namespace jmail

using namespace jmail_file;

//---------------------------------------------------------------------------Jmail_file::Jmail_file

Jmail_file::Jmail_file()
:
    _zero_(this+1)
{
}

//--------------------------------------------------------------------------Jmail_file::~Jmail_file

Jmail_file::~Jmail_file()
{
}

//---------------------------------------------------------------------------------Jmail_file::open

void Jmail_file::open( const char* param, Open_mode, const File_spec& )
{
    _msg = mail::create_message();
    _msg->init();

    for( Sos_option_iterator opt = param; !opt.end(); opt.next() )
    {
        if( opt.with_value( "queue-dir" ) )         _msg->set_queue_dir( opt.value() );
        else
        if( opt.with_value( "from" ) )              _msg->set_from( opt.value() );
        else
        if( opt.with_value( "reply-to" ) )          _msg->set_reply_to( opt.value() );
        else
        if( opt.with_value( "to" ) )                _msg->set_to( opt.value() );
        else
        if( opt.with_value( "cc" ) )                _msg->set_cc( opt.value() );
        else
        if( opt.with_value( "bcc" ) )               _msg->set_bcc( opt.value() );
        else
        if( opt.with_value( "subject" ) )           _msg->set_subject( opt.value() );
        else
        if( opt.with_value( "body" ) )              _msg->set_body( opt.value() );
        else
        if( opt.with_value( "filename" ) )          _attachment_filename = opt.value(); 
        else
        if( opt.with_value( "content-type" ) )      _content_type = opt.value();
        else
        if( opt.with_value( "encoding"     ) )      _encoding = opt.value();
        else
        if( opt.with_value( "attach" ) )   
        {
            string content_type;
            string encoding;
            string filename;
            string rename;

            for( Sos_option_iterator o = opt.value_debracked(); !o.end(); o.next() )
            {
                if( o.with_value( "content-type" ) )      content_type = o.value();
                else
                if( o.with_value( "encoding"     ) )      encoding     = o.value();
                else
                if( o.with_value( "rename"       ) )      rename       = o.value();
                else
                if( o.param(1)                     )      filename     = o.value();
                else
                    throw_sos_option_error( o );
            }

            _msg->add_file( filename, rename, content_type, encoding );
        }
        else
        if( opt.with_value( "header" ) )
        { 
            string header = opt.value();
            if( !header.empty() )
            {
                vector<string> h = vector_split( " *: *", opt.value(), 2 );
                h.resize(2);
                _msg->add_header_field( h[0].c_str(), h[1].c_str() );
            }
        }
        else 
        if( opt.with_value( "smtp" ) )              _msg->set_smtp( opt.value() );
        else 
            throw_sos_option_error( opt );
    }
}

//--------------------------------------------------------------------------------Jmail_file::close

void Jmail_file::close( Close_mode )
{
    try
    {
        if( !_msg->queue_dir().empty()  &&  _msg->to().empty()  &&  _msg->subject().empty()  &&  _msg->from().empty()  &&  _data.empty() )
        {
            _msg->auto_dequeue();
        }
        else
        {
            if( !_attachment_filename.empty() )
            {
                _msg->add_attachment( _data, _attachment_filename.c_str(), _content_type, _encoding );
            }
            else
            {
                if( !_content_type.empty() )  _msg->set_content_type( _content_type );
                if( !_encoding.empty() )      _msg->set_encoding( _encoding );
                _msg->set_body( _data );
            }

            bool ok = _msg->send();

            if( ok )
            {
                try
                {
                    _msg->auto_dequeue();
                }
                catch( const Xc& ) { LOG( "FEHLER BEIM VERARBEITEN DER EMAIL-WARTESCHLANGE WIRD IGNORIERT\n" ); }
            }
        }
    }
  //catch( const _com_error& x )  { throw_com_error(x,"jmail"); }
    catch( const exception& x )   { throw_xc(x); }
}

//----------------------------------------------------------------------------------Jmail_file::put

void Jmail_file::put_record( const Const_area& record )
{
    if( _data.capacity() < _data.length() + record.length() )  _data.reserve( _data.length() + record.length() + 200000 );
    _data.append( record.char_ptr(), record.length() );
}


} //namespace sos

