// $Id: spooler_http.h,v 1.2 2004/07/21 14:23:45 jz Exp $

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

struct Http_response
{
                                Http_response               ()                                      : _zero_(this+1) {}

    string                      content_type                ()                                      { return _content_type; }
    void                        finish                      ();

    bool                        eof                         ();
    string                      read                        ( int size );

    virtual bool                next_chunk_is_ready         ()                                      = 0;
    virtual bool                next_chunk                  ()                                      = 0;
    virtual int                 chunk_size                  ()                                      = 0;
    virtual string              read                        ( int size )                            = 0;


    Fill_zero                  _zero_;
    string                     _content_type;
    string                     _header;
    int                        _header_read_pointer;
    int                        _chunk_index;
};

//-----------------------------------------------------------------------------String_http_response

struct String_http_response : Http_response
{
                                String_http_response        ( const string& text )                  : _zero_(this+1), _text(text) {}

    bool                        next_chunk_is_ready         ()                                      { return true; }
    bool                        next_chunk                  ()                                      { return _chunk_index++ == 1; }
    int                         chunk_size                  ()                                      { return _text.length(); }
    string                      read                        ( int size );


    Fill_zero                  _zero_;
    string                     _text;
    int                        _read_pointer;
};

//--------------------------------------------------------------------------------Log_http_response

struct Log_http_response : Http_response
{
                                Log_http_response           ( Prefix_log* );

    bool                        next_chunk_is_ready         ()                                      { return true; }
    bool                        next_chunk                  ();
    int                         chunk_size                  ();
    string                      read                        ( int size );


    Fill_zero                  _zero_;
    ptr<Prefix_log>            _log;
    z::File                    _file;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
