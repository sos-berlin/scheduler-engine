// $Id$

#ifndef __Z_CHARACTER_ENCODING_H
#define __Z_CHARACTER_ENCODING_H

namespace zschimmer {

//------------------------------------------------------------------------------------------Charset

struct Charset
{
    static const Charset*           for_name                ( const string& charset_name );

    virtual string                  name                    () const                                            = 0;
    
  //virtual size_t                  wchar_count_of_encoded  ( const string& ) const                             = 0;
    //virtual BSTR                    bstr_from_encoded       ( const string& ) const                             = 0;
    virtual HRESULT                 Encoded_to_bstr         ( const string&, BSTR* result ) const               = 0;
    virtual HRESULT                 Encoded_to_olechar      ( const string&, OLECHAR* result, size_t size ) const  = 0;
    virtual HRESULT                 Olechar_to_encoded      ( const OLECHAR*, size_t, string* result ) const       = 0;
    HRESULT                         Bstr_to_encoded         ( const BSTR bstr, string* result ) const             { return Olechar_to_encoded( bstr, SysStringLen( bstr ), result ); }
    string                          encoded_from_bstr       ( const BSTR ) const;

    /*
    virtual string                  utf8_from_encoded       ( const Byte* encoded, size_t ) const               = 0;
    virtual size_t                  encoded_length_of_utf8  ( const string& utf8 )                              = 0;
    virtual size_t                  utf8_to_encoded         ( const string&, Byte* encoded_result, int size ) const = 0;

    virtual size_t                  utf8_length_of_encoded  ( const Byte* encoded, size_t ) const               = 0;
    
  //virtual size_t                  iso8859_1_length_of_encoded( const Byte*, size_t ) const                   = 0;
    virtual string                  iso8859_1_from_encoded  ( const Byte*, size_t ) const                       = 0;
    virtual size_t                  iso8859_1_to_encoded    ( const string&, Byte* result, size_t ) const       = 0;

  protected:
    virtual HRESULT                 Encoded_to_wchar        ( const string&, wchar_t* result, size_t result_size ) const               = 0;
    */
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
