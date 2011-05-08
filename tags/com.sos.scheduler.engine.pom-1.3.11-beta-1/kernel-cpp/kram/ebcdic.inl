// ebcdic.inl                                           (c) SOS GmbH Berlin

template< int mini, int maxi >
inline Ebcdic_array< mini, maxi > :: Ebcdic_array()
{
	 (*this)[mini] = 0;   // Borland will es wohl sonst nicht übersetzen.
}

template< int mini, int maxi >
Ebcdic_array<mini,maxi>::Ebcdic_array( const char* string0 )
{
    Index       i;
    const int   len = strlen( string0 );

    if( len ) {
        Index::check( len - 1 - Index::min() );
        xlat( (char *) this, string0, len, iso2ebc );
    }

    for( int j = Index::min() + len; j <= Index::max(); j++ ) {
        (*this) [ j ] = ' ';
    }
}
