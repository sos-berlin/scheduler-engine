struct Is_key
{
    Key                         key                     ();
                                object_store            ();
                                object_load             ();
};




struct Text_streamable
{
                               _store_in_text_stream    ( ostream* );
                               _load_from_text_stream   ( istream* );
};

struct Text_storable
{
                               _store_in_text_area      ( Area* );
                               _load_from_text_area     ( const Const_area& );
};

struct Binary_streamable
{
                               _store_in_binary_stream  ( Sos_binary_ostream* );
                               _load_from_binary_stream ( Sos_binary_istream* );
};

struct Binary_storable
{
                               _store_in_binary_area    ( Area* );
                               _load_from_binary_area   ( const Const_area& );
};

struct Has_text_repr : Text_streamable, Text_storable {};
struct Has_binary_repr : Binary_streamable, Binary_storable {};
