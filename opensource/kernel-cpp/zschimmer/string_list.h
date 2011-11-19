// $Id: string_list.h 13199 2007-12-06 14:15:42Z jz $

#ifndef __ZSCHIMMER_STRING_LIST_
#define __ZSCHIMMER_STRING_LIST_

#include "z_io.h"
//#include "immutable_string.h"


namespace zschimmer {

//--------------------------------------------------------------------------------------String_list

struct String_list
{
                                String_list             ()                                          : _read_offset(0) {}
                                String_list             ( const string& str )                       : _read_offset(0) { append( str ); }

    String_list&                operator =              ( const string& str )                       { clear(); append( str ); return *this; }

  //void                        append                  ( const string& str )                       { if( !str.empty() )  _list.push_back( str ); }
    void                        append                  ( const io::Char_sequence& seq )            { if( seq.length() > 0 )  _list.push_back( seq.to_string() ); }
    size_t                      length                  () const;
    string                      to_string               () const;
                                operator string         () const                                    { return to_string(); }
    bool                        is_empty                () const                                    { return _list.empty(); }
    void                        clear                   ()                                          { _list.clear();  _read_offset = 0; }

    io::Char_sequence           next_char_sequence      () const;
    io::Byte_sequence           next_byte_sequence      () const;
    void                        eat                     ( size_t );
    string                      string_eat              ( size_t );
    //Immutable_string            to_immutable_string     () const;
    //                            operator Immutable_string() const                                   { return to_immutable_string(); }

  private:
    std::list<string>          _list;
    size_t                     _read_offset;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
