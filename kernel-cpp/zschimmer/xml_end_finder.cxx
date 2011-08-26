// $Id: xml_end_finder.cxx 13199 2007-12-06 14:15:42Z jz $

#include "zschimmer.h"
#include "xml_end_finder.h"

namespace zschimmer
{

//-------------------------------------------------------------------Xml_end_finder::Xml_end_finder

Xml_end_finder::Xml_end_finder()
: 
    _zero_(this+1),
    _at_begin(true)
{
    _tok[cdata_tok  ]._begin = "<![CDATA[";
    _tok[cdata_tok  ]._end   = "]]>";
    _tok[comment_tok]._begin = "<!--";
    _tok[comment_tok]._end   = "-->";
}

//-----------------------------------------------------------------------Xml_end_finder::step_begin

bool Xml_end_finder::Tok_entry::step_begin( char c )      
{ 
    if( c == _begin[_index] )
    {
        _index++; 
    
        if( _begin[_index] == '\0' ) 
        {
            _index = 0; 
            _active = true;
            return true;
        }
    }
    else  
    {
        _index = 0; 
    }

    return false;
}

//--------------------------------------------------------------Xml_end_finder::Tok_entry::step_end

void Xml_end_finder::Tok_entry::step_end( char c )      
{ 
    if( c == _end[_index] )
    {
        _index++; 
        if( _end[_index] == '\0' )  _index = 0, _active = false;
    }
    else  
        _index = 0; 
}

//----------------------------------------------------------------------Xml_end_finder::is_complete

bool Xml_end_finder::is_complete( const char* p, int len )
{
    if( _at_begin  &&  len >= 1 )
    {
        _at_begin = false;
        _is_xml = *p == '<';
    }

    const char* p0 = p;
    while( p < p0+len )
    {
        if( *p != '\0' ) 
        {
            if( _tok[cdata_tok  ]._active )  _tok[cdata_tok  ].step_end( *p );
            else
            if( _tok[comment_tok]._active )  _tok[comment_tok].step_end( *p );
            else
            if( _in_tag )
            {
                if( _at_start_tag ) 
                {
                    if( *p == '/' )  _in_end_tag = true;               // "</"
                    else
                    if( !isalpha( (Byte)*p ) )  _in_special_tag = true;
                    _at_start_tag = false;
                }
                else
                if( *p == '>' ) 
                {
                    _in_tag = false;
                    
                    if( _in_end_tag ) 
                    {   
                        _in_end_tag = false;
                        _open_elements--;
                        if( _open_elements == 0 )  
                        { 
                            _xml_is_complete = true; 
                            break; 
                        }
                    }
                    else
                    if( !_in_special_tag )
                    {
                        if( _last_char != '/' )  _open_elements++;
                        else 
                        if( _open_elements == 0 )    // Das Dokument ist nur ein leeres XML-Element
                        { 
                            _xml_is_complete = true; 
                            break; 
                        }     
                    }
                    
                    _in_special_tag = false;
                }
            }
            else
            {
                bool in_something = false;
                in_something |= _tok[cdata_tok  ].step_begin( *p );    
                in_something |= _tok[comment_tok].step_begin( *p );

                if( !_is_xml  &&  *p == '\n' )  // Kein XML? Dann nehmen wir \n als Ende
                { 
                    _xml_is_complete = true; 
                    break; 
                }  
                else
                if( in_something )  _at_start_tag = false, _in_tag = false, _in_special_tag = false;
                else
                if( *p == '<' )  _in_tag = true, _at_start_tag = true;
            }
        }

        _last_char = *p;
        p++;
    }

    return _xml_is_complete;
}

//-------------------------------------------------------------------------------------------------

} // namespace zschimmer
