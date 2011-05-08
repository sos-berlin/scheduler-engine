//


struct Area_streambuf : Area, streambuf
{
    void put( uchar );
    void put( const Const_area& );
    void flush();
    unsigned char get();
    void get( Area& );
    void get_ptr( Area* );
  
  protected:
    int overflow( int = EOF );
    int underflow();
};

inline void Area_streambuf::put( uchar c )
{
    int rc = sputc( c );
    if (rc == EOF)  _XC.raise_exception( "PUT", "streambuf" );
}

inline void Area_streambuf::put( const Const_area& area )
{
    int rc = sputn( area, area.length() );
    if (rc != area.length()) _XC.raise_exception( "PUT", "streambuf" );
}

inline void Area_streambuf::flush()
{
    int rc = overflow();
    if (rc == EOF)  _XC.raise_exception( "PUT", "streambuf" );
}
