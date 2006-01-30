// $Id$

#ifndef __SPOOLER_HTTP_H
#define __SPOOLER_HTTP_H


namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

const int                       recommended_chunk_size = 32768;

//-------------------------------------------------------------------------------------------------

struct Http_processor_channel;
struct Http_processor;

//-------------------------------------------------------------------------------------Http_request

struct Http_request : Object
{
                                Http_request                ()                                     : _zero_(this+1){}

    bool                        has_parameter               ( const string& name ) const            { return _parameters.find( name ) != _parameters.end(); }
    string                      parameter                   ( const string& name ) const;
    bool                        is_http_1_1                 () const;
    string                      header_field                ( const string& name ) const            { String_map::const_iterator it = _header.find( name );
                                                                                                      return it == _header.end()? "" : it->second; }
    string                      url                         () const;
    string                      url_path                    () const                                { return _path; }
    string                      host_and_port_field         () const;
    string                      charset_name                () const;


    Fill_zero                  _zero_;
    string                     _http_cmd;
    string                     _protocol;
    string                     _path;

    typedef stdext::hash_map<string,string>  String_map;
    String_map                 _header;
    String_map                 _parameters;
    string                     _body;
};

//---------------------------------------------------------------------------------------Http_parser

struct Http_parser : Object
{
                                Http_parser                 ( Http_request* );


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


    Fill_zero                  _zero_;
    string                     _text;
    bool                       _reading_body;
    int                        _body_start;
    int                        _content_length;
    const char*                _next_char;
    Http_request* const        _http_request;
};

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
                                Chunk_reader                ()                                      : _zero_(this+1), _recommended_block_size( recommended_chunk_size ) {}

    virtual void                recommend_block_size        ( int size )                            { _recommended_block_size = size; }
    int                         recommended_block_size      () const                                { return _recommended_block_size; }
    virtual void            set_event                       ( Event_base* )                         {}


    virtual bool                next_chunk_is_ready         ()                                      = 0;
    virtual int                 get_next_chunk_size         ()                                      = 0;
    virtual string              read_from_chunk             ( int size )                            = 0;
    virtual string              html_insertion              ()                                      { return ""; }      // Irgendeine HTML-Einfügung, z.B. <hr/>


    Fill_zero                  _zero_;
    int                        _recommended_block_size;
};

//-----------------------------------------------------------------------------String_chunk_reader

struct String_chunk_reader : Chunk_reader
{
                                String_chunk_reader         ( const string& text ) : _zero_(this+1), _text(text) {}

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
                                Chunk_reader_filter         ( Chunk_reader* r )                     : _chunk_reader(r) {}

    void                        set_event                   ( Event_base* event )                   { _chunk_reader->set_event( event ); }

    ptr<Chunk_reader>          _chunk_reader;
};

//------------------------------------------------------------------------Html_chunk_reader
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

//------------------------------------------------------------------------------------Http_response

struct Http_response : Object
{
                                Http_response               ( Http_request*, Chunk_reader*, const string& content_type );
    
    void                        recommend_block_size        ( int size )                            { if( _chunk_reader )  _chunk_reader->recommend_block_size( size ); }
    int                         recommended_block_size      () const                                { return _chunk_reader? _chunk_reader->_recommended_block_size : recommended_chunk_size; }

  //bool                        is_http_1_1                 ()                                      { return _http_1_1; }
    bool                        close_connection_at_eof     ()                                      { return _close_connection_at_eof; }

    void                    set_event                       ( Event_base* event )                   { if( _chunk_reader )  _chunk_reader->set_event( event ); }

    string                      content_type                ()                                      { return _content_type; }
    void                    set_content_type                ( const string& value )                 { _content_type = value; }
    void                    set_header_field                ( const string& name, const string& value ) { _header_fields[ name ] = value; }
    void                    set_status                      ( int code, const string& text )        { _status_code = code; _status_text = text; }
    void                        finish                      ();

    bool                        eof                         ();
    string                      read                        ( int recommended_size );
    string                      header_text                 () const                                { return _header; }


  protected:
    string                      start_new_chunk             ();


    Fill_zero                  _zero_;
    ptr<Http_request>          _http_request;
    bool                       _chunked;
    bool                       _close_connection_at_eof;
    ptr<Chunk_reader>          _chunk_reader;
    string                     _content_type;
    int                        _status_code;
    string                     _status_text;

    typedef map<string,string>  Header_fields;
    Header_fields              _header_fields;
    string                     _header;
    int                        _chunk_index;                // 0: Header
    uint                       _chunk_size;
    uint                       _chunk_offset;               // Bereits gelesene Bytes
    bool                       _chunk_eof;
    bool                       _eof;
    bool                       _finished;
};

//-----------------------------------------------------------------------------------Http_processor

struct Http_processor : Communication::Processor
{
    enum Response_code
    {
        code_bad_request                = 404,
        code_internal_server_error      = 500
    };

    struct Http_exception : exception
    {
        Http_exception( Response_code response_code ) : _response_code( response_code ), _what( response_messages[ (int)response_code ] ) {}
        ~Http_exception() throw () {}
        const char*             what                        ()                                      { return _what.c_str(); }
        Response_code          _response_code;
        string                 _what;
    };


    static stdext::hash_map<int,string>  response_messages;


                                Http_processor              ( Http_processor_channel* );


    void                        put_request_part            ( const char* data, int length )        { _http_parser->add_text( data, length ); }
    bool                        request_is_complete         ()                                      { return !_http_parser  ||  _http_parser->is_complete(); }

    void                        process                     ();

    bool                        response_is_complete        ();
    string                      get_response_part           ();
    bool                        should_close_connection     ();


    Fill_zero                  _zero_;
    ptr<Http_request>          _http_request;
    ptr<Http_parser>           _http_parser;
    ptr<Http_response>         _http_response;
};

//---------------------------------------------------------------------------Http_processor_channel

struct Http_processor_channel : Communication::Processor_channel
{
                                Http_processor_channel      ( Communication::Channel* ch )          : Communication::Processor_channel( ch ) {}

    ptr<Communication::Processor> processor                 ()                                      { ptr<Http_processor> result = Z_NEW( Http_processor( this ) ); 
                                                                                                      return +result; }

    string                      channel_type                () const                                { return "HTTP"; }
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
