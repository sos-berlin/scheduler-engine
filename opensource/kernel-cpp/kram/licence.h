// licence.h                                    (c)1996 SOS GmbH Berlin
//                                              Joacim Zschimmer

#ifndef __LICENCE_H
#define __LICENCE_H

#include <list>

#ifndef __SOSARRAY_H
#   include "sosarray.h"
#endif

#ifndef __SOSDATE_H
#   include "sosdate.h"
#endif

namespace sos
{

const int max_licence_key_length = 100;     // L�nger sollte ein Linzenschl�ssel nicht sein
const int max_seriennr_anzahl   = 50;       // Soviele Seriennr in einem Programmlauf
const int max_ausstellerkuerzel = 8;
const int max_kundenkuerzel     = 20;
const int max_par               = 1000;     // H�chste Parameternummer
const int base                  = 10+26;    // 0-9,A-Z


// Lizenzkomponenten-Codes:
const int licence_hostapi               = 2;           // 2
const int licence_hostdde               = 3;           // 3
const int licence_hostole               = 4;           // 4
const int licence_hostodbc              = 5;           // 5
const int licence_hostphp               = 6;           // 6
const int licence_scheduler             = 7;           // 7
const int licence_soscopy               = 8;           // 8
const int licence_e9750                 = 9;           // 9
const int licence_sossql                = 10;          // A
const int licence_letter                = 11;          // B
const int licence_licence_key           = base*1 + 0;  // 10
const int licence_fs                    = base*1 + 1;  // 11
const int licence_e370                  = base*1 + 2;  // 12
const int licence_hostapi_hostdde       = base*1 + 3;  // 13  Nur einer von beiden
const int licence_fra2rtf               = base*1 + 4;  // 14
const int licence_saacke                = base*1 + 5;  // 15  SAB, SAN Firmenlizenzen
const int licence_fs_demo               = base*1 + 6;  // 16  Beschr�nkte Anzahl Clients
const int licence_foxgate               = base*1 + 7;  // 17
const int licence_scheduler_database_password = base*1 + 8;  // 18
const int licence_verfallsdatum_1900    = base*1 + 9;  // 19
const int licence_os                    = base*1 + 10; // 1A  "W" Windows, "S" Solaris,    "N" NT?
const int licence_odbc_blob             = base*1 + 11; // 1B  F�r B�ckmann, BfA: hostAPI f�r Blob <-> file
const int licence_verfallsdatum_2000    = base*2 + 0;  // 20
const int licence_scheduler_console     = base*2 + 1;  // 21
const int licence_scheduler_agent       = base*2 + 2;  // 22


// Lizenzkomponenten-Codes f�r hostAPI von 30 bis 4Z
const int licence_hostapi_first         = base*3 + 0;  // 30
const int licence_wbrz_logos_to_loga    = base*3 + 0;  // 30
const int licence_kis                   = base*3 + 1;  // 31
const int licence_sisy                  = base*3 + 3;  // 33
const int licence_saacke_winword        = base*3 + 3;  // 33
const int licence_hostapi_last          = base*4 + 26; // 4Z

const int licence_upper_limit           = base*5 + 0;


struct Sos_seriennr
{
                                Sos_seriennr            ();

    Bool                        operator ==             ( const Sos_seriennr& ) const;

    Fill_zero                  _zero_;
    char                       _ausstellerkuerzel [ max_ausstellerkuerzel ];
    char                       _kundenkuerzel [ max_kundenkuerzel ];
    uint4                      _lfdnr;
    Sos_date                   _date;
    Bool                       _invalid;
    Bool                       _old;
};

//--------------------------------------------------------------------------------------Sos_licence

struct Sos_licence : Sos_self_deleting
{
                                Sos_licence             ();
                                Sos_licence             ( const string& key );
                               ~Sos_licence             ();

    void                        init                    ();
    bool                        empty                   () const                   { return _installed_keys.empty(); }
    static string               string_from_licence_int ( int code );

    void                        check                   ();     // Wird von sosstat.cxx gerufen
    void                        check2                  ();     // Wird von sosprof.cxx set_sos_ini_filename() gerufen
    boolean                     is_demo_version         ();
    void                        log_licence_keys        ();

    const char*                 operator []             ( int ) const;

    Sos_string                  key                     () const;

    void                        set_par                 ( int n, const char* str = "" );
  //void                       _throw                   () const;

    static string               component_name          ( int code );
    static int                  component_code_of_name_or_minus( const string& );
  //static const char*         _component_names[ licence_upper_limit + 1 ];

//private:
    void                        check_from_file         ( const Sos_string& filename );
    void                        read_key                ( const char* );
    void                        read_key                ( const string& key )               { read_key( key.c_str() ); }
    void                        check_key               ();
    void                        merge_licence           ( const Sos_licence& );
    static int                  alnum_as_int            ( char );
    static int                  alnum_as_int_or_minus   ( char );
    static uint4                alnum_as_uint4          ( const char*, int );
    uint4                       berechneter_sicherungscode() const;
    void                        set_par                 ( int, const char*, int len );

  public:
    Fill_zero                  _zero_;
    string                     _empty_value_string;
    std::list<string>          _installed_keys;
    Sos_simple_array<Sos_seriennr> _seriennr;
    char                       _salz;
    Bool                       _zz;
    uint4                      _sicherungscode;

  //Sos_simple_array<const char*> _par;
    Sos_simple_array<Sos_string> _par;

    friend                      struct Licence_key_file;

};


void                            check_licence           ( const char* component_name );

#define SOS_LICENCE( licence_code )  ( (*sos_static_ptr()->_licence)[ licence_code ] )
// sos_static_ptr() ist evtl. Methode von Has_static_ptr


} //namespace sos


#endif

