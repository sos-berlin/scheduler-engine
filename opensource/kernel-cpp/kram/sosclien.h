// sosclien.h                               ©1996 SOS GmbH Berlin

#ifndef __SOSCLIEN_H
#define __SOSCLIEN_H

#if !defined __SOSSTRNG_H
#   include "sosstrng.h"
#endif

#if !defined __SOSOBJ_H
//#   include <sosobj.h>
#endif

#if !defined __SOSARRAY_H
#   include "sosarray.h"
#endif

namespace sos
{

struct Sos_database_session;

void init_std_client();
// Trägt Standard-Client in Sos_static ein.

//------------------------------------------------------------------------------------Sos_client

struct Sos_client : Sos_self_deleting
{
                                Sos_client              ();
                               ~Sos_client              ();

    void                        close                   ();

    void                       _obj_print               ( std::ostream* ) const;

    Sos_string                 _name;
    Sos_simple_array< Sos_static_ptr<Sos_database_session> > _session_array;
};


} //namespace sos

#endif
