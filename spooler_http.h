// $Id: spooler_http.h,v 1.1 2004/07/18 15:38:02 jz Exp $

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

//-------------------------------------------------------------------------------------------------



} //namespace spooler
} //namespace sos

#endif
