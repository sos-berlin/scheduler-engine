// frmfield.h

// benutzt von frmfield.cxx und frm2rtf.cxx

#include "../file/flstream.h"
#include "ebcdifld.h"

namespace sos
{


struct Frame_parser
{
    struct Token
    {
        enum Kind
        {
            k_none = 0,
            k_eof = 1,
            k_new_line,
            k_comma,
            k_semikolon,
            k_gleich,
            k_point,
            k_klammer_auf,
            k_klammer_zu,
            k_plus,
            k_minus,
            k_slash,
            k_string,
            k_identifier,
            k_number,
            k__last             // repr_tab abgleichen!!
        };

        static const char*      repr                    ( Kind );

        static void             init                    ();

                                Token                   ();
                                Token                   ( std::istream* );

        Kind                    kind                    () const  { return _kind; }
        const String0_area&     name                    () const  { return _name; }
        uint4                   number                  () const  { return _number; }

    //private:
        Kind                   _kind;
        Sos_limited_text<256>  _name;
        uint4                  _number;
        Source_pos             _pos;

        struct Token_entry
        {
                                Token_entry    () : _kind ( k_none ), _name ( "" ) {}
                                Token_entry    ( Kind k, const char* n ) : _kind ( k ), _name( n ) {}

            Kind               _kind;
            const char*        _name;
        };

        static Sos_simple_array<Token_entry> _token_array;
    };

    struct Stmt
    {
                                Stmt                    ();

      //void                    init                    ( const Any_file& s, const Source_pos& pos );
        void                    im                      ( const Sos_string& );
        void                    read_next_stmt          ();
        char                    get                     ()  { return *_ptr++;}
        char                    peek                    ()  { return *_ptr; }
        Source_pos              pos                     ()  { Source_pos p; p = _pos; p._col = _ptr - _ptr0; return p; }


        Fill_zero              _zero_;
        Any_file_stream        _input_stack [ 10 ];
        Source_pos             _pos_stack [ 10 ];
        int                    _input_sp;
        std::istream*          _input;
        const char*            _ptr0;
        const char*            _last_ptr;               // Wird vom Scanner auf Anfang des aktuellen Tokens gesetzt
        const char*            _ptr;
        Sos_limited_text<1024> _stmt;
        Bool                   _eof;

      private:
        Source_pos             _pos;
    };

    struct Scanner
    {
                                Scanner                 ();

      //void                    init                    ( const Any_file&, const Source_pos& );
        void                    init                    ( const Sos_string& filename );
        void                    read_next_token         ();

        Stmt                   _stmt;
        Source_pos             _pos;
        Token                  _token;
        Bool                   _multi_line_stmt;
    };

    struct Field
    {
                                    Field               ();

        Fill_zero                  _zero_;
        char                       _name [ 100+1 ];
        int                        _offset;
        int                        _len;
        Bool                       _len_null;
        char                       _typ [ 10+1 ];
        Bool                       _typ_null;
        int                        _for_vorkomma;
        Bool                       _for_vorkomma_null;
        int                        _for_nachkomma;
        Bool                       _for_nachkomma_null;
        char                       _ini [200+1];
        Bool                       _ini_null;
        Bool                       _zif;
        Bool                       _upp;
        Bool                       _low;
        Bool                       _mus;
        Bool                       _jan;
        Bool                       _dat;
        Bool                       _fut;
        Bool                       _pas;
        Bool                       _tag;
        Bool                       _mon;
        Bool                       _jah;
        Bool                       _numeric;
        Bool                       _ign;
        int                        _one;
        Bool                       _one_null;
        int                        _aon;
        Bool                       _aon_null;
    };

                                Frame_parser            ();
                              //Frame_parser            ( const Any_file&, const Source_pos& );

  //void                        init                    ( const Any_file&, const Source_pos& );
    void                        init                    ( const Sos_string& filename );
    void                        im                      ( const Sos_string& filename )          { _scanner._stmt.im( filename ); }

    Bool                        next_token_is           ( Token::Kind k )   { return next_token().kind() == k; }
    Bool                        next_token_is           ( const char* );
    const Token&                next_token              ()                  { return _scanner._token; }
    Sos_string                  text                    ();
    void                        skip_stmt               ();
    void                        expect                  ( Token::Kind );
    void                        parse                   ( Token::Kind );
    void                        parse_token             ();
    uint4                       parse_number            ();
    Sos_string                  parse_identifier        ();
    void                        parse                   ( const char* );
    Sos_string                  parse_string            ();
    void                        parse_stmt_begin        ();
    Bool                        parse_on_off            ();
    Sos_string                  parse_rest              ();
    Bool                        end_of_stmt             ();
    void                        begin                   ();
    void                        end                     ();
    void                        parse_stmt_end          ();
    static Bool                 is_keyword              ( const char* word, const char* looking_for );
    Field                       parse_df_field          ();
    Sos_ptr<Field_descr>        parse_df_field_descr    ();
    Sos_ptr<Record_type>        parse_complete_df       ();
    void                        parse_until_field       ();
    void                        parse                   ();

    Fill_zero                  _zero_;
    int                        _offset;             // für create table
    int                        _df_nesting;
    Scanner                    _scanner;
    Ebcdic_type_flags          _ebcdic_flags;
};


} //namespace sos





