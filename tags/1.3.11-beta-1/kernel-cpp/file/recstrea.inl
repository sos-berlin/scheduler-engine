// recstrea.inl

#define DEFINE_OUTPUT_STREAM_OPERATIONS(Type,stream) \
/*inline Type& Type::operator << ( char p )                  { stream << p; return *this; } */ \
inline Type& Type::operator << ( unsigned char p )         { stream << p; return *this; }  \
inline Type& Type::operator << ( signed char p )           { stream << p; return *this; }  \
/*inline Type& Type::operator << ( const char* p )         { stream << p; return *this; }*/\
/*inline Type& Type::operator << ( const unsigned char* p )  { stream << p; return *this; }*/\
/*inline Type& Type::operator << ( const signed char* p )    { stream << p; return *this; }*/\
inline Type& Type::operator << ( short p )                 { stream << p; return *this; }  \
inline Type& Type::operator << ( unsigned short p )        { stream << p; return *this; }  \
inline Type& Type::operator << ( int p )                   { stream << p; return *this; }  \
inline Type& Type::operator << ( unsigned int p )          { stream << p; return *this; }  \
inline Type& Type::operator << ( long p )                  { stream << p; return *this; }  \
inline Type& Type::operator << ( unsigned long p )         { stream << p; return *this; }  \
inline Type& Type::operator << ( float p )                 { stream << p; return *this; }  \
inline Type& Type::operator << ( long double p )           { stream << p; return *this; }  \
/*inline Type& Type::operator << ( void* p )                 { stream << p; return *this; } */ \
inline Type& Type::operator << ( streambuf* p )            { stream << p; return *this; }  \
/*inline Type& Type::operator << ( ostream& (*p)(ostream&) ) { stream << p; return *this; }*/\
inline Type& Type::operator << ( ios& (*p)(ios&) )         { stream << p; return *this; }

#define DEFINE_INPUT_STREAM_OPERATIONS(Type,stream) \
/*inline Type& Type::operator >> ( istream & (*p)(istream&) )  { stream >> p; return *this; }*/\
inline Type& Type::operator >> ( ios & (*p)(ios&) )          { stream >> p; return *this; }  \
/*inline Type& Type::operator >> (          char& p )          { stream >> p; return *this; } */ \
inline Type& Type::operator >> ( unsigned char& p )          { stream >> p; return *this; }  \
inline Type& Type::operator >> (   signed char& p )          { stream >> p; return *this; }  \
inline Type& Type::operator >> ( short& p )                  { stream >> p; return *this; }  \
inline Type& Type::operator >> ( int& p )                    { stream >> p; return *this; }  \
inline Type& Type::operator >> ( long& p )                   { stream >> p; return *this; }  \
inline Type& Type::operator >> ( unsigned short& p )         { stream >> p; return *this; }  \
inline Type& Type::operator >> ( unsigned int& p )           { stream >> p; return *this; }  \
inline Type& Type::operator >> ( unsigned long& p )          { stream >> p; return *this; }  \
inline Type& Type::operator >> ( float& p )                  { stream >> p; return *this; }  \
inline Type& Type::operator >> ( double& p )                 { stream >> p; return *this; }  \
inline Type& Type::operator >> ( long double& p )            { stream >> p; return *this; }


#define DEFINE_RECORD_STREAM_OPERATIONS(Type,stream) \
    DEFINE_OUTPUT_STREAM_OPERATIONS(Type,stream) \
    DEFINE_INPUT_STREAM_OPERATIONS(Type,stream) \
    inline Type& Type::operator << ( Const_area p )  { write( p ); return *this; }  \
    inline Output_stream& Type::operator << ( Output_stream& (*p)(Output_stream&))  { return p( *this ); }  \
    inline Input_stream& Type::operator >> ( Input_stream& (*p)(Input_stream&))  { return p( *this ); }  \
    inline Type& Type::operator >> ( Area& p )       { read( p ); return *this; }

//-------------------------------------------Record_streambuf::Record_streambuf

inline Record_streambuf::Record_streambuf( Record_stream _FAR * f, uint buffer_size )
  : _f           ( f ),
    _buffer_size ( buffer_size ),
    _get_next_record( true )
{ }

//-----------------------------------------------------------------------------
//DEFINE_RECORD_STREAM_OPERATIONS( Record_stream, *(Stream*)this )

//inline void Record_stream::write_end_of_record()  { flush(); }
//inline void Record_stream::flush()  { Streambuf_stream::flush(); }

