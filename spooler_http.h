// $Id: spooler_http.h,v 1.10 2004/07/27 08:24:03 jz Exp $

#ifndef __SPOOLER_HTTP_H
#define __SPOOLER_HTTP_H


namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

const int                       recommended_chunk_size = 32768;

//-------------------------------------------------------------------------------------Http_request

struct Http_request : Object
{
                                Http_request                ()                                     : _zero_(this+1){}

    bool                        has_parameter               ( const string& name ) const            { return _parameters.find( name ) != _parameters.end(); }
    string                      parameter                   ( const string& name ) const;
    bool                        is_http_1_1                 () const;

    Fill_zero                  _zero_;
    string                     _http_cmd;
    string                     _protocol;
    string                     _path;
    map<string,string>         _header;
    map<string,string>         _parameters;
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
        read_chunk()  bis genau size Bytes gelesen
    }
*/

struct Chunk_reader : Object
{
                                Chunk_reader                ()                                      : _zero_(this+1), _recommended_chunk_size( recommended_chunk_size ) {}

    virtual void                recommend_chunk_size        ( size )                                { _recommended_chunk_size; }
    virtual void            set_event                       ( Event_base* )                         {}


    virtual bool                next_chunk_is_ready         ()                                      = 0;
    virtual int                 get_next_chunk_size         ()                                      = 0;
    virtual string              read_chunk                  ( int size )                            = 0;


    Fill_zero                  _zero_;
    int                        _recommended_chunk_size;
};

//-----------------------------------------------------------------------------String_chunk_reader

struct String_chunk_reader : Chunk_reader
{
                                String_chunk_reader         ( const string& text ) : _zero_(this+1), _text(text) {}

  protected:
    bool                        next_chunk_is_ready         ()                                      { return true; }
    int                         get_next_chunk_size         ();
    string                      read_chunk                  ( int size );


    Fill_zero                  _zero_;
    string                     _text;
    bool                       _get_next_chunk_size_called;
    uint                       _offset;                     // Bereits gelesene Bytes
};

//--------------------------------------------------------------------------------Log_chunk_reader

struct Log_chunk_reader : Chunk_reader
{
                                Log_chunk_reader           ( Prefix_log* );
                               ~Log_chunk_reader           ();

    void                    set_event                       ( Event_base* );


  protected:
    bool                        next_chunk_is_ready         ();
    int                         get_next_chunk_size         ();
    string                      read_chunk                  ( int size );


    Fill_zero                  _zero_;
    ptr<Prefix_log>            _log;
    Event_base*                _event;
    File                       _file;
    bool                       _file_eof;
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

    virtual void                recommend_chunk_size        ( size )                                { _chunk_reader->recommend_chunk_size( size );
                                                                                                      Chunk_reader_filter::recommend_chunk_size( size ); }

  protected:
    bool                        next_chunk_is_ready         ();
    int                         get_next_chunk_size         ();
    string                      read_chunk                  ( int size );
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
    int                        _blank_count;                // Nach dem vierten Blank haben den Log-Level und Job/Task/Order/Scheduler
    int                        _in_span;                    // Wir müssen am Zeilenende soviele </span> schreiben
    string                     _line_prefix;                // Zeilenanfang bis "[info"
};

//------------------------------------------------------------------------------------Http_response

struct Http_response : Object
{
                                Http_response               ( const Http_request*, Chunk_reader*, const string& content_type );
    
    void                        recommend_chunk_size        ( int size )                            { _chunk_reader->recommend_chunk_size( size ); }

  //bool                        is_http_1_1                 ()                                      { return _http_1_1; }
    bool                        close_connection_at_eof     ()                                      { return _close_connection_at_eof; }

    void                    set_event                       ( Event_base* event )                   { _chunk_reader->set_event( event ); }

    string                      content_type                ()                                      { return _content_type; }
    void                    set_content_type                ( string value )                        { _content_type = value; }
    void                        finish                      ();

    bool                        eof                         ();
    string                      read                        ( int recommended_size );


  protected:
    string                      start_new_chunk             ();


    Fill_zero                  _zero_;
    bool                       _chunked;
    bool                       _close_connection_at_eof;
    ptr<Chunk_reader>          _chunk_reader;
    string                     _content_type;
    string                     _header;
    int                        _chunk_index;                // 0: Header
    uint                       _chunk_size;
    uint                       _chunk_offset;               // Bereits gelesene Bytes
    bool                       _chunk_eof;
    bool                       _eof;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
