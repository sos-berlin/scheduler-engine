// $Id$

#ifndef __XML_END_FINDER_H
#define __XML_END_FINDER_H

namespace zschimmer {

//-----------------------------------------------------------------------------------Xml_end_finder

struct Xml_end_finder
{
    // Findet das Ende eines XML-Dokuments

    enum Tok 
    { 
        cdata_tok, 
        comment_tok,
        max_tok 
    };


    struct Tok_entry
    {
                                Tok_entry                   ()                          : _index(0),_active(false) {}

        void                    reset                       ()                          { _index = 0; _active = false; }
        bool                    step_begin                  ( char );
        void                    step_end                    ( char );

        int                    _index;
        bool                   _active;
        const char*            _begin;
        const char*            _end;
    };


                                Xml_end_finder              ();

    bool                        is_complete                 ( const char* p, int length );
    bool                        is_complete                 ()                          { return _xml_is_complete; }

    Fill_zero                  _zero_;

    int                        _open_elements;              // Anzahl der offenen Elemente (ohne <?..?> und <!..>)
    bool                       _at_start_tag;               // Letztes Zeichen war '<'
    bool                       _in_special_tag;             // <?, <!
    bool                       _in_tag;                 
    bool                       _in_end_tag;             
    bool                       _at_begin;
    bool                       _is_xml;                     // Ist es XML?
    bool                       _xml_is_complete;
    char                       _last_char;
    Tok_entry                  _tok [ max_tok ];
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
