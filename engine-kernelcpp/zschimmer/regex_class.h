// $Id: regex_class.h 13315 2008-01-22 13:27:57Z jz $

#ifndef __ZSCHIMMER_REGEX_CLASS_H
#define __ZSCHIMMER_REGEX_CLASS_H

#ifdef Z_WINDOWS
#   include "../3rd_party/regexp/regex.h"
#else
#   include "regex.h"
#endif

namespace zschimmer {


const int max_regex_matches = 100;

struct Regex;

//--------------------------------------------------------------------------------------Regex_match

struct Regex_match
{
                                Regex_match             ()                                      { _match.rm_so = -1;  _match.rm_eo = -1; }

                                operator bool           () const                                { return _match.rm_eo > 0; }

    int                         offset                  () const                                { return _match.rm_so; }
    int                         end                     () const                                { return _match.rm_eo; } 
    int                         length                  () const                                { return _match.rm_eo - _match.rm_so; }

  private:
    friend struct               Regex;

    regmatch_t                 _match;
};

//---------------------------------------------------------------------------------Regex_submatches

struct Regex_submatches
{
                                Regex_submatches        ();

                                operator bool           () const                                { return _is_matching; }
    int                         count                   () const                                { return _matches; }
    string                      operator []             ( int i ) const;
    int                         offset                  ( int i = 0 ) const                     { return _match_array[i].rm_so; }
    int                         end                     ( int i = 0 ) const                     { return _match_array[i].rm_eo; } 
    int                         length                  ( int i = 0 ) const                     { return _match_array[i].rm_eo - _match_array[i].rm_so; }


  private:
    friend struct               Regex;

    Fill_zero                  _zero_;
    bool                       _is_matching;
    int                        _matches;
    string                     _string;
    regmatch_t                 _match_array [ max_regex_matches+1 ];
};

//--------------------------------------------------------------------------------------------Regex

struct Regex
{
    enum Flags
    {
        rx_none,
        rx_ignore_case,
    };


                                Regex                   ()                                      : _zero_(this+1) {}
                                Regex                   ( const char* regex )                   : _zero_(this+1) { compile( regex ); }
                                Regex                   ( const string& regex )                 : _zero_(this+1) { compile( regex ); }
                               ~Regex                   ()                                      { close(); }

    void                        close                   ()                                      { regfree( &_regex ); }
    void                        compile                 ( const char*, Flags = rx_none );
    void                        compile                 ( const string& regex, Flags f = rx_none )  { compile( regex.c_str(), f ); }

    Regex_submatches            match_subresults        ( const string& ) const;

    Regex_match                 match                   ( const char* ) const;
    Regex_match                 match                   ( const string& s ) const               { return match( s.c_str() ); }
    bool                        is_compiled             () const                                { return _is_compiled; }

  private:
    void                        throw_error             ( int error, const string& text = "" ) const;

    Fill_zero                  _zero_;
    regex_t                    _regex;
    bool                       _is_compiled;
};

//--------------------------------------------------------------------------------------operator ==

inline bool operator == ( const string& str, const Regex& regex ) { return regex.match( str ); }
inline bool operator == ( const Regex& regex, const string& str ) { return regex.match( str ); }

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
