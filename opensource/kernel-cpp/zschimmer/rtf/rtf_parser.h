// rtf_parser.h                                         ©2000 Joacim Zschimmer
// $Id: rtf_parser.h 12231 2006-09-01 06:37:30Z jz $

#ifndef __RTF_PARSER_H
#define __RTF_PARSER_H

#include "../zschimmer.h"
#include <stdio.h>
#include <vector>
#include <stack>

namespace zschimmer {
namespace rtf {

struct Parser : Has_rtf_context
{
    struct Token
    {
                                Token                   ( Has_rtf_context* c )          : _zero_(this+1), _rtf(c) {}

        Fill_zero              _zero_;
        Entity                 _rtf;
        Source_pos             _pos;
    };


                                Parser                  ( const ptr<Doc>& );
                               ~Parser                  (); 

    void                        init                    ();
  //ptr<Doc>                    parse                   ( FILE* );
    void                        parse                   ();
    void                        parse_rest_of_group     ( Code outer_destination_code );
    void                        parse_destination       ();
    void                        parse_any_destination   ( Code, int param = 0 );
    void                        parse_non_destination   ();

    void                        parse_stylesheet        ();
    void                        parse_fonttbl           ();

    void                        ignore_group            ( int start_nesting = 0 );      // Parsiert {}-Verschachtelung zuende
    void                        ignore_rest_of_group    ()                              { ignore_group( 1 ); }
    void                        get_next_token          ();
    void                        eat                     ( Code );
    int                         eat_char                ();

    void                        restore_state           ();


    Fill_zero                  _zero_;
    string                     _filename;
    FILE*                      _input;                  // Eingabedatei
    const char*                _input_ptr;              // Oder Zeiger auf RTF-Dokument im Speicher
    int                        _next_char;
    Source_pos                 _char_pos;
    Source_pos                 _eaten_char_pos;
    Token                      _token;
    ptr<Doc>                   _doc;
    bool                       _non_doc_property_discovered;
    int                        _group_nesting;          // Aktuelle {}-Verschachtelung
  //bool                       _dont_ignore_pgdsctbl;   // eMail von Andreas Liebert 31.8.2006

    std::vector< Properties >  _properties_stack;
                      
  //std::stack<Destination_entity*>   _entity_stack;           // Für {}-Verschachtelung

  //string                     _hilfspuffer;
    char*                      _hilfspuffer;
};

} //namespace rtf
} //namespace zschimmer

#endif
