// $Id: spooler_http.h,v 1.3 2004/07/21 20:40:09 jz Exp $

#ifndef __SPOOLER_HTTP_H
#define __SPOOLER_HTTP_H


namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

struct Http_request : Object
{
                                Http_request                 ()                                     : _zero_(this+1){}


    Fill_zero                  _zero_;
    string                     _http_cmd;
    string                     _protocol;
    string                     _path;
    map<string,string>         _header;
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

//------------------------------------------------------------------------------------Http_response

struct Http_response : Object
{
                                Http_response               ()                                      : _zero_(this+1) {}

    virtual void            set_event                       ( Event_base* )                         {}

    string                      content_type                ()                                      { return _content_type; }
    void                    set_content_type                ( string value )                        { _content_type = value; }
    void                        finish                      ();

    bool                        eof                         ();
    string                      read                        ( int recommended_size );

  protected:
    string                      start_new_chunk             ();

    virtual bool                next_chunk_is_ready         ()                                      = 0;
    virtual uint                get_next_chunk_size         ()                                      = 0;
    virtual string              read_chunk                  ( int size )                            = 0;


    Fill_zero                  _zero_;
    Event_base*                _event;
    string                     _content_type;
    string                     _header;
    int                        _chunk_index;                // 0: Header
    uint                       _chunk_size;
    uint                       _chunk_offset;               // Bereits gelesene Bytes
    bool                       _chunk_eof;
    bool                       _eof;
};

//-----------------------------------------------------------------------------String_http_response

struct String_http_response : Http_response
{
                                String_http_response        ( const string& text, string content_type );


  protected:
    bool                        next_chunk_is_ready         ()                                      { return true; }
    uint                        get_next_chunk_size         ();
    string                      read_chunk                  ( int size );


    Fill_zero                  _zero_;
    string                     _text;
};

//--------------------------------------------------------------------------------Log_http_response

struct Log_http_response : Http_response
{
                                Log_http_response           ( Prefix_log*, string content_type );
                               ~Log_http_response           ();

    void                    set_event                       ( Event_base* );


  protected:
    bool                        next_chunk_is_ready         ();
    uint                        get_next_chunk_size         ();
    string                      read_chunk                  ( int size );


    Fill_zero                  _zero_;
    ptr<Prefix_log>            _log;
    File                       _file;
    bool                       _file_eof;
    string                     _html_prefix;
    string                     _html_suffix;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
