// $Id: regex_class.cxx 13682 2008-09-29 15:44:28Z jz $

#include "zschimmer.h"
#include "regex_class.h"


using namespace std;


namespace zschimmer {

//-----------------------------------------------------------------------------------Regex::compile

void Regex::compile( const char* regex, Regex::Flags flags )
{
    regfree( &_regex );

    int regcomp_flags = REG_EXTENDED;
    if( flags & rx_ignore_case )  regcomp_flags |= REG_ICASE;

    int error = regcomp( &_regex, regex, regcomp_flags );
    if( error )  throw_error( error, regex );

    _is_compiled = true;
}

//--------------------------------------------------------------------------Regex::match_subresults

Regex_submatches Regex::match_subresults( const string& str ) const
{
    if( !_is_compiled )  throw_xc( Z_FUNCTION, "Regex not compiled" );


    Regex_submatches result;

    result._matches = 0;

    int error = regexec( &_regex, str.c_str(), NO_OF( result._match_array ), result._match_array, 0 );
    
    if( error ) 
    {
        if( error != REG_NOMATCH 
         && error != 17          // Das ist für regexec() von Apache 2 unter Linux, das statt regexec() aus libc() eingebunden wird. jz 2004-10-15
                                 )  throw_error( error, str );
        //int REGEX_17_FUER_APACHE;
    }
    else
    {
        result._is_matching = true;
        int n = NO_OF(result._match_array);
        while( n > 1  &&  result._match_array[n-1].rm_so == -1 )  --n;
        result._matches = n - 1;
    }

    result._string = str;
    return result;
}

//-------------------------------------------------------------------------------------Regex::match

Regex_match Regex::match( const char* str ) const
{
    if( !_is_compiled )  throw_xc( Z_FUNCTION, "Regex not compiled" );


    Regex_match result;

    int error = regexec( &_regex, str, 1, &result._match, 0 );
    
    if( error  &&  error != REG_NOMATCH 
               &&  error != 17          // Das ist für regexec() von Apache 2 unter Linux, das statt regexec() aus libc() eingebunden wird. jz 2004-10-15
                                        )  throw_error( error, str );
        //int REGEX_17_FUER_APACHE;

    return result;
}

//-------------------------------------------------------------------------------Regex::throw_error

void Regex::throw_error( int error, const string& str ) const
{
    char error_text [300];
    regerror( error, &_regex, error_text, sizeof error_text - 1 );
    throw_pattern( "REGEX-%d", error, error_text, str.c_str() );
}

//---------------------------------------------------------------Regex_submatches::Regex_submatches

Regex_submatches::Regex_submatches()
: 
    _zero_(this+1) 
{
    for( int i = 0; i < NO_OF( _match_array ); i++ )
    {
        _match_array[ i ].rm_so = -1;
        _match_array[ i ].rm_eo = -1;
    }
}

//--------------------------------------------------------------------Regex_submatches::operator []

string Regex_submatches::operator [] ( int i ) const
{ 
    const regmatch_t* m = _match_array + i; 
    return _string.substr( m->rm_so, m->rm_eo - m->rm_so ); 
}

//------------------------------------------------------------------------------------replace_regex

string replace_regex( const string& str, const string& regex_str, const string& neu, int limit )
{
    string       result = "";
    Regex        regex  = regex_str;
    const char*  p      = str.c_str();

    result.reserve( str.length() + 100 * neu.length() );

    for( int i = 0; i < limit; i++ )
    {
        Regex_match match = regex.match( p );
        if( !match )  break;

        result.append( p, match.offset() );
        result.append( neu );

        p += match.end();
    }

    return result + p;
}

//--------------------------------------------------------------------------------replace_regex_ref

string replace_regex_ref( const string& str, const string& regex_str, const string& replacement_with_refs, int limit )
{
    string       result = "";
    Regex        regex  = regex_str;
    const char*  p      = str.c_str();

    result.reserve( str.length() + 100 * replacement_with_refs.length() );

    for( int i = 0; i < limit; i++ )
    {
        Regex_submatches submatches = regex.match_subresults( p );
        if( !submatches )  break;

        int count = min( submatches.count(), 31 );   // 32 ist schon ein Blank

        result.append( p, submatches.offset() );

        const char* r = replacement_with_refs.c_str();
        while( *r )
        {
            // \1
            const char* s = r;
            while( (unsigned char)*s >= count )  s++;   // bei \0 bis \9 stoppen.
            result.append( r, s-r );
            if( !*s )  break;
            r = s;

            result.append( str.data() + submatches.offset(*r), submatches.length(*r) );
            r++;
        }

        p += submatches.end();
        if( !*p )  break;
    }

    return result + p;
}

//--------------------------------------------------------------------------------replace_regex_ext
// NICHT GETESTET!

string replace_regex_ext( const string& str, const string& regex_str, const string& replacement_with_refs, int limit )
{
    string       result     = "";
    Regex        regex      = regex_str;
    const char*  p          = str.c_str();
    char         cmd        = 0;
    char         cmd_idx    = 0;

    for( int i = 0; i < limit; i++ )
    {
        Regex_submatches submatches = regex.match_subresults( p );
        if( !submatches )  break;

        result.append( p, submatches.offset() );

        const char* r = replacement_with_refs.c_str();
        while( *r )
        {
            const char* s = r;
            while( *s  &&  *s != '\\' )  s++;
            result.append( r, s-r );

            if( *r )
            {
                r = s;

                switch( r[1] )
                {
                    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                        if( r[1] <= '0' + submatches.count()  )
                        {
                            result.append( str.data() + submatches.offset( r[1]-'0' ), submatches.length( r[1]-'0' ) );
                            r += 2;
                        }
                        else
                            result += *r++;

                        break;

                    case 'l': 
                    case 'u': 
                        cmd = r[1];
                        cmd_idx = result.length();
                        r += 2;
                        break;

                    default: 
                        result += *r++;
                }
            }

            if( cmd  &&  (int)result.length() > cmd_idx )  
            {
                if( cmd == 'u' )  result[cmd_idx] = toupper( result[cmd_idx] );
                else
                if( cmd == 'l' )  result[cmd_idx] = tolower( result[cmd_idx] );
                cmd = 0;
            }
        }

        p += submatches.end();
        if( !*p )  break;
    }

    return result + p;
}

//----------------------------------------------------------------------------------------has_regex

bool has_regex( const string& str, const string& regex_str )
{
    return Regex(regex_str).match( str );
}

//-------------------------------------------------------------------------------------vector_split

void vector_split( const string& regex_str, const string& str, int limit, std::vector<string>* result )
{
    result->resize(0);

    if( str.empty() )  return;

    Regex       regex = regex_str;
    const char* p     = str.c_str();
    int         n     = limit? limit : INT_MAX;
    int         i;

    for( i = 0; i < n-1; i++ )
    {
        Regex_match match = regex.match( p );
        if( !match )  break;

        result->push_back( string( p, match.offset() ) );

        p += match.end();
    }

    result->push_back( p );
    i++;

    if( limit == 0 )
    {
        while( i > 0  &&  (*result)[i-1].empty() )  i--;
        result->resize(i);
    }
    else
        assert( (int)result->size() <= limit );
}

//----------------------------------------------------------------------------------------set_split

void set_split( std::set<string>* result, const string& regex_str, const string& str )
{
    result->clear();

    if( str.empty() )  return;

    Regex       regex = regex_str;
    const char* p     = str.c_str();

    while(1)
    {
        Regex_match match = regex.match( p );
        if( !match )  break;

        result->insert( string( p, match.offset() ) );

        p += match.end();
    }

    result->insert( p );
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
