// $Id$

#ifndef __SPOOLER_HTTP_H
#define __SPOOLER_HTTP_H


namespace sos {
namespace spooler {
namespace http {

//--------------------------------------------------------------------------------------------const

const int                       recommended_chunk_size = 32768;

//-------------------------------------------------------------------------------------------------

struct Operation_connection;
struct Operation;
struct Request;
struct Response;

//-------------------------------------------------------------------------------------Chunk_reader
/*
    Verwendung:

    while(1)
    {
        while( !next_chunk_is_ready() )  wait();

        int size = get_next_chunk_size();
        if( size == 0 )  break;  // EOF
        read_from_chunk()  bis genau size Bytes gelesen
    }
*/

struct Chunk_reader : Object
{
                                Chunk_reader                ( const string& content_type, const string& charset = "" ) 
                                                                                                    : _zero_(this+1), _recommended_block_size( recommended_chunk_size ), _content_type(content_type), _charset(charset) {}

    virtual void                recommend_block_size        ( int size )                            { _recommended_block_size = size; }
    int                         recommended_block_size      () const                                { return _recommended_block_size; }
    virtual void            set_event                       ( Event_base* )                         {}


    virtual bool                next_chunk_is_ready         ()                                      = 0;
    virtual int                 get_next_chunk_size         ()                                      = 0;
    virtual string              read_from_chunk             ( int size )                            = 0;
    virtual string              html_insertion              ()                                      { return ""; }      // Irgendeine HTML-Einfügung, z.B. <hr/>


    Fill_zero                  _zero_;
    int                        _recommended_block_size;
    string                     _content_type;
    string                     _charset;
};

//-----------------------------------------------------------------------------String_chunk_reader

struct String_chunk_reader : Chunk_reader
{
                                String_chunk_reader         ( const string& text, const string& content_type = "text/plain" ) : Chunk_reader( content_type, "ISO-8859-1" ), _zero_(this+1), _text(text) {}

  protected:
    bool                        next_chunk_is_ready         ()                                      { return true; }
    int                         get_next_chunk_size         ();
    string                      read_from_chunk             ( int size );


    Fill_zero                  _zero_;
    string                     _text;
    bool                       _get_next_chunk_size_called;
    uint                       _offset;                     // Bereits gelesene Bytes
};

//--------------------------------------------------------------------------------Log_chunk_reader

struct Log_chunk_reader : Chunk_reader
{
                                Log_chunk_reader            ( Prefix_log* );
                               ~Log_chunk_reader            ();

    void                    set_event                       ( Event_base* );


  protected:
    bool                        next_chunk_is_ready         ();
    int                         get_next_chunk_size         ();
    string                      read_from_chunk             ( int size );
    virtual string              html_insertion              ();


    Fill_zero                  _zero_;
    ptr<Prefix_log>            _log;
    Event_base*                _event;
    File                       _file;
    bool                       _file_eof;
    string                     _html_insertion;
};

//------------------------------------------------------------------------------Chunk_reader_filter

struct Chunk_reader_filter : Chunk_reader
{
                                Chunk_reader_filter         ( Chunk_reader* r, const string& content_type, const string& charset = "" ) : Chunk_reader( content_type, charset ), _chunk_reader(r) {}

    void                        set_event                   ( Event_base* event )                   { _chunk_reader->set_event( event ); }

    ptr<Chunk_reader>          _chunk_reader;
};

//--------------------------------------------------------------------------------Html_chunk_reader
// Konvertiert Text nach HTML

struct Html_chunk_reader : Chunk_reader_filter
{
    enum State { reading_prefix, reading_text, reading_suffix, reading_finished };


                                Html_chunk_reader           ( Chunk_reader*, const string& title );
                               ~Html_chunk_reader           ();

    virtual void                recommend_block_size        ( int size )                            { _chunk_reader->recommend_block_size( size );
                                                                                                      Chunk_reader_filter::recommend_block_size( size ); }

  protected:
    bool                        next_chunk_is_ready         ();
    int                         get_next_chunk_size         ();
    string                      read_from_chunk             ( int size );
    bool                        try_fill_chunk              ();


    Fill_zero                  _zero_;
    State                      _state;
    string                     _html_prefix;
    string                     _html_suffix;
    int                        _available_net_chunk_size;
    string                     _chunk;
    bool                       _chunk_filled;

    // Für <span class="debug9">...[debug9]...</span>
    bool                       _awaiting_class;             // Wir erwarten [info] und dergleichen
  //int                        _blank_count;                // Nach dem vierten Blank haben den Log-Level und Job/Task/Order/Scheduler
    int                        _in_span;                    // Wir müssen am Zeilenende soviele </span> schreiben
    string                     _line_prefix;                // Zeilenanfang bis "[info"
};

//--------------------------------------------------------------------------------------Status_code

enum Status_code
{
    status_301_moved_permanently            = 301,
    status_404_bad_request                  = 404,
    status_500_internal_server_error        = 500,
    status_501_not_implemented              = 501,
    status_505_http_version_not_supported   = 505
};

//-------------------------------------------------------------------------------------------------

struct Http_exception : exception
{
    Http_exception( Status_code status_code, const string& error_text = "" ) 
    : 
        _status_code( status_code ), 
        _error_text( error_text )
    {
    }

    ~Http_exception() throw () 
    {
    }


    const char*             what                        ();
    Status_code            _status_code;
    string                 _what;
    string                 _error_text;
};

//-------------------------------------------------------------------------------------------------

extern stdext::hash_map<int,string>  http_status_messages;

//-------------------------------------------------------------------------------------------------

struct Headers
{
    struct Entry
    {
                                Entry                       ()                                          {}
                                Entry                       ( const string& name, const string& value ) : _name(name), _value(value) {}
        string _name;
        string _value;
    };

    bool                        contains                    ( const string& name )                  { return _map.find( lcase( name ) ) != _map.end(); }
    string                      operator[]                  ( const string& name ) const;
    void                        set                         ( const string& name, const string& value );
    void                        set_default                 ( const string& name, const string& value );


    typedef stdext::hash_map<string,Entry>   Map;
    Map                        _map;
};

//--------------------------------------------------------------------------------------------Parser

struct Parser : Object
{
                                Parser                      ( Request* );


    void                        add_text                    ( const char*, int len );
    bool                        is_complete                 ();

    void                        parse_header                ();
    void                        eat_spaces                  ();
    void                        eat                         ( const char* str );
    string                      eat_word                    ();
    string                      eat_until                   ( const char* character_set );
    string                      eat_path                    ();
    void                        eat_line_end                ();
    char                        next_char                   ()                                      { return *_next_char; }
    const string&               text                        () const                                { return _text; }               // Zum Debuggen

  private:
    Fill_zero                  _zero_;
    string                     _text;
    bool                       _reading_body;
    int                        _body_start;
    int                        _content_length;
    const char*                _next_char;
    Request* const             _request;
};

//------------------------------------------------------------------------------------------Request

struct Request : Object
{
                                Request                     ()                                      : _zero_(this+1){}

    bool                        has_parameter               ( const string& name ) const            { return _parameters.find( name ) != _parameters.end(); }
    string                      parameter                   ( const string& name ) const;
    bool                        is_http_1_1                 () const;
    string                      header                      ( const string& name ) const            { return _headers[ name ]; }
    string                      url                         () const;
    string                      url_path                    () const                                { return _path; }
    string                      character_encoding          () const;
    string                      content_type                () const;
    const string&               body                        () const                                { return _body; }


  //private:
    friend struct               Parser;
    friend struct               Operation;
    friend struct               Response;

    Fill_zero                  _zero_;
    string                     _http_cmd;
    string                     _protocol;                                                      
    string                     _path;

    Headers                    _headers;
    typedef stdext::hash_map<string,string>  String_map;
    String_map                 _parameters;
    string                     _body;
};

//-----------------------------------------------------------------------------------------Response

struct Response : Object
{
                                Response                    ( Operation* );
    
    void                        recommend_block_size        ( int size )                            { if( _chunk_reader )  _chunk_reader->recommend_block_size( size ); }
    int                         recommended_block_size      () const                                { return _chunk_reader? _chunk_reader->_recommended_block_size : recommended_chunk_size; }

  //bool                        is_http_1_1                 ()                                      { return _http_1_1; }
    bool                        close_connection_at_eof     ()                                      { return _close_connection_at_eof; }

    void                    set_event                       ( Event_base* event )                   { if( _chunk_reader )  _chunk_reader->set_event( event ); }

  //string                      content_type                ()                                      { return _content_type; }
  //void                    set_content_type                ( const string& value )                 { set_header( "Content-Type", value ); }
  //void                    set_character_encoding          ( const string& value );
    void                    set_header                      ( const string& name, const string& value ) { _headers.set( name, value ); }
    string                      header                      ( const string& name )                  { return _headers[ name ]; }
    void                    set_status                      ( Status_code, const string& text = "" );
    void                    set_chunk_reader                ( Chunk_reader* c )                     { _chunk_reader = c; }
    void                        finish                      ();

    bool                        eof                         ();
    string                      read                        ( int recommended_size );
  //string                      header_text                 () const                                { return _headers_stream; }


  protected:
    friend struct               Operation;

    string                      start_new_chunk             ();


    Fill_zero                  _zero_;
    Operation*                 _operation;
    bool                       _chunked;
    bool                       _close_connection_at_eof;
    ptr<Chunk_reader>          _chunk_reader;
  //string                     _content_type;
    Status_code                _status_code;

    Headers                    _headers;
    String_stream              _headers_stream;
    int                        _chunk_index;                // 0: Header
    uint                       _chunk_size;
    uint                       _chunk_offset;               // Bereits gelesene Bytes
    bool                       _chunk_eof;
    bool                       _eof;
    bool                       _finished;
};

//-----------------------------------------------------------------------------------Operation

struct Operation : Communication::Operation
{
                                Operation                   ( Operation_connection* );


    void                        put_request_part            ( const char* data, int length )        { _parser->add_text( data, length ); }
    bool                        request_is_complete         ()                                      { return !_parser  ||  _parser->is_complete(); }

    void                        begin                       ();
    virtual bool                async_continue_             ( Continue_flags );
    virtual bool                async_finished_             ()                                      { return _response != NULL; }
    virtual string              async_state_text_           ()                                      { return "none"; }

    bool                        response_is_complete        ();
    string                      get_response_part           ();
    bool                        should_close_connection     ();

    Request*                    request                     () const                                { return _request; }
    Response*                   response                    () const                                { return _response; }


  private:
    friend struct               Request;
    friend struct               Response;

    Fill_zero                  _zero_;
    ptr<Request>               _request;
    ptr<Parser>                _parser;
    ptr<Response>              _response;
    ptr<Web_service_operation> _web_service_operation;
};

//------------------------------------------------------------------------Operation_connection

struct Operation_connection : Communication::Operation_connection
{
                                Operation_connection   ( Communication::Connection* c )        : Communication::Operation_connection( c ) {}

    ptr<Communication::Operation> new_operation             ()                                      { ptr<Operation> result = Z_NEW( Operation( this ) ); 
                                                                                                      return +result; }

    string                      connection_type             () const                                { return "HTTP"; }
};

//-------------------------------------------------------------------------------------------------

} //namespace http
} //namespace spooler
} //namespace sos

#endif
