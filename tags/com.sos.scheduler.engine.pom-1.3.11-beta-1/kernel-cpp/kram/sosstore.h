// sosstore.h                                             SOS GmbH Berlin

#ifndef __SOSSTORE_H
#define __SOSSTORE_H

struct string;

//----------------------------------------------------------------------------

struct Sos_output_store
{
  //enum Format     { binary, short_text, long_text };
  //enum Byte_order { highest_byte_first, lowest_byte_first };
  //enum Char_code  { ascii, ebcdic };

                                Sos_output_store        ( ostream& );
    virtual                    ~Sos_output_store        ();

    virtual void                write_fixed             ( const char*, unsigned int length );
    virtual void                write                   ( const char*, unsigned int length );
    virtual void                write_area              ( const Const_area& );
    virtual void                write_fixed_area        ( const Const_area& );
    virtual void                write_c_string          ( const char* );
            void                write_string            ( const string& );
            void                write_char              ( char );
            void                write_int               ( int );
            void                write_uint              ( uint );
    virtual void                write_int1              ( int1 );
    virtual void                write_uint1             ( uint1 );
    virtual void                write_int2              ( int2 );
    virtual void                write_uint2             ( uint2 );
    virtual void                write_int4              ( int4 );
    virtual void                write_uint4             ( uint4 );
    virtual void                write_bool              ( Bool );
    virtual void                flush                   ();
            int4                write_position          () const;

  //        void                byte_order              ( Byte_oder );
  //        void                char_code               ( Char_code );
  //        void                bytes_for_int           ( int );

  //DECLARE_OUTPUT_STREAM_OPERATIONS(Sos_store)

  protected:
    ostream&                   _s;

  private:
    int                        _double_quote;           // '\"' verdoppelt
};

struct Sos_binary_output_store : Sos_output_store
{
                                Sos_binary_output_store ( ostream& );
    virtual                    ~Sos_binary_output_store ();

    virtual void                write                   ( const char*, unsigned int length );
    virtual void                write_fixed             ( const char*, unsigned int length );
    virtual void                write_c_string          ( const char* );
    virtual void                write_int               ( int );
    virtual void                write_uint              ( uint );
    virtual void                write_int1              ( int1 );
    virtual void                write_uint1             ( uint1 );
    virtual void                write_int2              ( int2 );
    virtual void                write_uint2             ( uint2 );
    virtual void                write_int4              ( int4 );
    virtual void                write_uint4             ( uint4 );
    virtual void                write_bool              ( Bool );

    void                        flush                   ();
    int4                        write_position          () const;
};

//-----------------------------------------------------------------------------

struct Sos_input_store
{
                                Sos_input_store         ( istream& );
    virtual                    ~Sos_input_store         ();

            void                read                    ( char*, uint size, uint* length_ptr = 0 );
    virtual void                read_fixed              ( char*, uint size );
            void                read_area               ( Area* );
            void                read_fixed_area         ( Area& );
    virtual void                read_string             ( string* );
    virtual void                read_char               ( char* );
            void                read_int                ( int* );
            void                read_uint               ( uint* );
    virtual void                read_int1               ( int1* );
    virtual void                read_uint1              ( uint1* );
    virtual void                read_int2               ( int2* );
    virtual void                read_uint2              ( uint2* );
    virtual void                read_int4               ( int4* );
    virtual void                read_uint4              ( uint4* );

    int                         get                     ();
    char                        read_char               ();
    int                         read_int                ();
    uint                        read_uint               ();
    int1                        read_int1               ();
    uint1                       read_uint1              ();
    int2                        read_int2               ();
    uint2                       read_uint2              ();
    int4                        read_int4               ();
    uint4                       read_uint4              ();

    void                        sync                    ();

  protected:
    istream&                   _s;

  private:
    int                        _double_quote;           // '\"' verdoppelt
};

struct Sos_binary_input_store : Sos_input_store
{
                                Sos_binary_input_store  ( istream& );
    virtual                    ~Sos_binary_input_store  ();

    virtual void                read_fixed              ( char*, uint size );
    virtual void                read_char               ( char* );
    virtual void                read_int1               ( int1* );
    virtual void                read_uint1              ( uint1* );
    virtual void                read_int2               ( int2* );
    virtual void                read_uint2              ( uint2* );
    virtual void                read_int4               ( int4* );
    virtual void                read_uint4              ( uint4* );
};

//----------------------------------------------------------------------------
#if 0

struct Sos_store : Sos_input_store,
                   Sos_output_store
{
                                Sos_store               ( iostream& );
  //virtual                    ~Sos_input_store         ();
  //DECLARE_INPUT_STREAM_OPERATIONS(Sos_store)
  //DECLARE_OUTPUT_STREAM_OPERATIONS(Sos_store)
};
#endif


//==========================================================================================inlines

#define DEFINE_FUNCTIONAL_SOSSTORE_READ( Class, Type ) \
   inline Type Class::read_##Type()                    \
   {                                                   \
       Type object;                                    \
       read_##Type( &object );                         \
       return object;                                  \
   }

DEFINE_FUNCTIONAL_SOSSTORE_READ( Sos_input_store, char  )
DEFINE_FUNCTIONAL_SOSSTORE_READ( Sos_input_store, int   )
DEFINE_FUNCTIONAL_SOSSTORE_READ( Sos_input_store, uint  )
DEFINE_FUNCTIONAL_SOSSTORE_READ( Sos_input_store, int1  )
DEFINE_FUNCTIONAL_SOSSTORE_READ( Sos_input_store, uint1 )
DEFINE_FUNCTIONAL_SOSSTORE_READ( Sos_input_store, int2  )
DEFINE_FUNCTIONAL_SOSSTORE_READ( Sos_input_store, uint2 )
DEFINE_FUNCTIONAL_SOSSTORE_READ( Sos_input_store, int4  )
DEFINE_FUNCTIONAL_SOSSTORE_READ( Sos_input_store, uint4 )

#endif

