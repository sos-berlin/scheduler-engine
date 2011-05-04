// sosopt.h (c) SOS GmbH (js,jz)

#ifndef __SOSOPT_H
#define __SOSOPT_H

#ifndef __SOSSTRNG_H
#include "sosstrng.h"
#endif

namespace sos
{

struct Sos_option_iterator
{
                                Sos_option_iterator     ( int argc, char** argv, Bool ignore_first = true );  // Default beginnt bei argv[1]
                                Sos_option_iterator     ( int argc, char** argv, const string& alt_params );  // alt_params, wenn argc=argv=0
                                Sos_option_iterator     ( const char* pointer );  // wird nicht kopiert!
#                               if defined __SOSSTRNG_H
                                Sos_option_iterator     ( const Sos_string& );
#                               endif
                               ~Sos_option_iterator     ();

    void                        next                    ();
    Bool                        end                     ()                                      { return _end; }

    void                        log                     ( Bool val = true )                     { _log = val; }
    Bool                        flag                    ( const char* str );
    Bool                        with_value              ( const char* ); // hier können Exceptions auftreten (Flag erkannt, aber kein Value!)
    Bool                        param                   ( int nummer = -1 );
    void                        skip_param              ();                                     // Um Programmdateinamen zu überspringen
    Bool                        pipe                    ();

    Bool                        set                     ()                                      { return _set; }
    const string&               value                   ();
    string                      value_debracked         ();
    int                         as_int                  ();
    uint                        as_uint                 ();
    uint4                       as_uint4                ();
    uint                        as_uintK                ();
    double                      as_double               ();
    char                        as_char                 ();

    string                      complete_parameter      ( char quote, char quote_quote );

    const char*                 option                  () const                                { return _option; }
    Sos_string                  rest                    (); // Rest ausgeben (mit Blank als Trenner)
    Source_pos                  pos                     () const; 

    void                        max_params              ( unsigned int m )                      { _max_params = m; }
    int                         params_count            ()                                      { return _param_count; }
    void                        compatible              ( Bool b )                              { _compatible = b; }
    void                        alt_option_char         ( char c )                              { _alt_option_char = c; }    
    void                        ignore_case             ( Bool b )                              { _ignore_case = b; }

 //obsolet:
    Bool                        flag                    ( char, const char* );
    Bool                        with_value              ( char, const char* );

  private:
    friend struct               Sos_option_error;
    Bool                        is_sos_option           ();
    void                        handle_sos_option       ();

    void                        read_option             ();
    void                        read_value              ();
    void                        log_option              ();

    Fill_zero                  _zero_;
    Sos_string                 _string;
    const char*                _ptr;
    const char*                _rest_ptr;
    const char*                _parameter_start;        // Für complete_parameter()

    Bool                       _use_argv;
    int                        _argc;
    const char**               _argv;
    int                        _arg_i;

    char                       _option [30+1];          // Ohne '-', Stellungsparameter wird hier nicht gespeichert
    Dynamic_area               _buffer;
    Sos_string                 _value;
    int                        _param_count;
    int                        _max_params;
    Bool                       _set;                    // Option endet nicht auf '-'
    bool                       _value_read;
    Bool                       _pipe;
    Bool                       _end;
    Bool                       _no_more_options;
	Bool                       _compatible;	
    char                       _alt_option_char;
    Bool                       _ignore_case;
    Bool                       _log;
};

struct Sos_option_error : Xc
{
    Sos_option_error( const Sos_option_iterator& );
};

void throw_sos_option_error( const Sos_option_iterator& );

/*
Beispiel:

for( Sos_option_iterator options( argc, argv ); !current_option.end(); current_option.next() }
{
    if ( options.is_sos_option() ) options.handle_sos_option();
    else
    if( options.with_value( "log" ) )  log_file = options.value();
    else
    if( options.flag( "switch" ) || options.flag( "s" ) )  sw = options.set();      ;
    else
    if( options.flag( "x" ) )  sw = true;
    else
    if( options.with_value( "number" ) )  number = as_int( options.value() );
    else
    if( options.param() )  filename = options.value();
    else throw Unknown_option_error();
}

-- und - erlaubt
- am Ende bei swtch(), set() liefert Bool
with_string() string liefert value()
= erlaubt ohne Blanks
" klammert
keine Buchstabenhaufen -kdskjfnasdfndcjknadkcndaskladncacn
\
max_params( int ) erlaubt die Einstellung der maximal erwarteten Stellungsparameter

-switch
-switch-
--switch
-x -s
--log datei
-log=datei
-log "datei xy" filename1 filename2 -x -- -x

*/

struct Sos_token_iterator
{
                // String muß die Lebensdauer von Sos_token_iterator haben!
                Sos_token_iterator( const Const_area& area, const char sep='\t' );
                Sos_token_iterator( const char*, const char sep='\t' );
                Sos_token_iterator( const Sos_string&, const char sep='\t' );
               ~Sos_token_iterator();

    Bool        end() { return _eof; }
    void        next();

    Sos_string  value() { if ( end() ) throw Xc( "Sos_token_iterator::value" ); return _value; }

 private:
    void        _init();
    char        _sep;
    Bool        _eof;
    ::std::istream*    _istream_ptr;        // zur Optimierung: mit char* implementieren! jz
    Sos_string  _value;
    Bool        _first;
};


} //namespace sos

#endif

