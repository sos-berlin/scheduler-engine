// $Id: z_mail.h 13199 2007-12-06 14:15:42Z jz $

#ifndef __ZSCHIMMER_Z_MAIL_H__
#define __ZSCHIMMER_Z_MAIL_H__


namespace zschimmer {

//------------------------------------------------------------------------------------Email_address

struct Email_address
{
                                Email_address               ( const string& name_and_address = "" ) : _suppress_exceptions(false)  { assign( name_and_address ); }

    Email_address&              operator =                  ( const string& name_and_address      ) { assign( name_and_address );  return *this; }

    void                        suppress_exceptions         ( bool b )                              { _suppress_exceptions = b; }
    void                        assign                      ( const string& name_and_address );
    const string&               address                     () const                                { return _address; }
    void                    set_address                     ( const string& address )               { _address = address; }
    const string&               name                        () const                                { return _name; }
    void                    set_name                        ( const string& name )                  { _name = name; }
    string                      email_address               () const;
                                operator string             () const                                { return email_address(); }

    string                     _address;
    string                     _name;
    bool                       _suppress_exceptions;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
