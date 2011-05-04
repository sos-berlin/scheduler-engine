//#define MODULE_NAME "filetab"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Jörg Schwiemann, Joacim Zschimmer"

#include "precomp.h"
#include "../kram/sysdep.h"

// Makro(s) zum Deklarieren
#define DECLARE_FILE_TYPE_FILTER( ft, opt )   { ft file( opt ); }


#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../file/filter.h"

#if defined __BORLANDC__
#   pragma option -w-use       // 'Identifier declared but not used' abschalten
#endif

namespace sos {

//-------------------------------------------------------------------------------------------static

static long                     initialized                 = false;

//-------------------------------------------------------------------------------------------static


struct Sos_object_descr;

extern void call_for_linker( const void* );   // Damit *o eingebunden wird

#define REFERENCE_FOR_LINKER( TYPE, SYMBOL ) \
{                                            \
    extern TYPE SYMBOL;                      \
    call_for_linker( &SYMBOL );              \
}

// Initialisierung der Filetypen (s.anyfile.cpp und filetab.cpp)
void init_file_types()
{
    //if( InterlockedExchange( &initialized, true ) )  return;
    if( initialized )  return;
    initialized = true;

# if defined SYSTEM_WIN
//  REFERENCE_FOR_LINKER( const Sos_object_descr&, dde_client_descr );      // ddeobj.cxx
# endif
# if defined SYSTEM_WIN16
//jz 18.11.96    REFERENCE_FOR_LINKER( const Abs_file_type&, btrieve_file_type       );  // btrieve.cxx
# endif
  //REFERENCE_FOR_LINKER( const Sos_object_descr&, record_as_sam_descr );   // sam.cxx
  //REFERENCE_FOR_LINKER( const Sos_object_descr&, record_as_nl_descr );    // nl.cxx
    REFERENCE_FOR_LINKER( const Sos_object_descr&, sos_socket_descr );      // sossock.cxx
  //REFERENCE_FOR_LINKER( const Sos_object_descr&, get_as_put_descr );      // getput.cxx

    REFERENCE_FOR_LINKER( const Abs_file_type&, alias_file_type         );  // alias.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, cache_file_type         );  // cache_file.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, cache_key_file_type     );  // cachefl.cxx (alt)
    REFERENCE_FOR_LINKER( const Abs_file_type&, cache_seq_file_type     );  // cachsqfl.cxx (alt)
# ifdef SYSTEM_ODBC
    REFERENCE_FOR_LINKER( const Abs_file_type&, columns_file_type       );  // colsfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, create_table_file_type  );  // create_table_file.cxx
# endif
    REFERENCE_FOR_LINKER( const Abs_file_type&, com_file_type           );  // comfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, convert_file_type       );  // convfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, concat_file_type        );  // concatfl.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, delete_file_type        );  // deletefl.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, dir_file_type           );  // dir.cxx
  //DECLARE_FILE_TYPE_NEW( E370_file );
    REFERENCE_FOR_LINKER( const Abs_file_type&, ebcdic_ascii_file_type  );  // ebcascfl.cxx

#if defined SYSTEM_WIN //|| defined SYSTEM_LINUX
    REFERENCE_FOR_LINKER( const Abs_file_type&, ebo_file_type           );  // ebofile.cxx      // Eichenauer, jz 3.1.00
#endif

    REFERENCE_FOR_LINKER( const Abs_file_type&, error_file_type         );  // errfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, file_as_key_file_type   );  // filekey.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, head_file_type          );  // head_file.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, frame_vfl_file_type     );  // vflfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, frame_dm_file_type      );  // framedm.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, fs_file_type            );  // fsfile
    REFERENCE_FOR_LINKER( const Abs_file_type&, inline_file_type        );  // inlinefl.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, insert_file_type        );  // insertfl.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, jdbc_file_type          );  // jdbc.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, jmail_file_type         );  // jmail_file.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, key_seq_file_type       );  // keyseqfl.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, licence_key_file_type   );  // licenceg.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, lockwait_file_type      );  // lockwait.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, log_file_type           );  // logfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, null_file_type          );  // nullwait.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, nl_file_type            );  // nlfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, object_file_type        );  // objfile.cxx
#ifdef SYSTEM_ODBC
    REFERENCE_FOR_LINKER( const Abs_file_type&, odbc_file_type          );  // odbc.cxx
#endif
    REFERENCE_FOR_LINKER( const Abs_file_type&, p_file_type             );  // p_file.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, record_tabbed_file_type );  // rectab.cxx
  //REFERENCE_FOR_LINKER( const Abs_file_type&, record_file_type        );  // recfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, sam3_file_type          );  // sam3file.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, select_all_file_type    );  // selall.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, sossql_file_type        );  // sossqlfl.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, sql_file_type           );  // sqlfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, std_file_type           );  // stdfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, store_file_type         );  // storefl.cxx
  //REFERENCE_FOR_LINKER( const Abs_file_type&, tabbed_file_type        );  // tabbed.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, tabbed_record_file_type );  // tabrec.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, tail_file_type          );  // tail_file.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, tb_file_type            );  // tb.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, tee_file_type           );  // teefl.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, update_file_type        );  // updatefl.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, xml_file_type           );  // xml_file.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, zlib_file_type          );  // zlibfile.cxx
#if defined SYSTEM_WIN
  //REFERENCE_FOR_LINKER( const Abs_file_type&, oracle_file_type        );  // oracle.cxx
#endif

#if defined SYSTEM_WIN
  //REFERENCE_FOR_LINKER( const Abs_file_type&, dde_file_type           );  // ddefile.cxx
  //REFERENCE_FOR_LINKER( const Abs_file_type&, mapi_file_type          );  // mapifl.cxx
#endif

#if defined SYSTEM_WIN32
    REFERENCE_FOR_LINKER( const Abs_file_type&, clipboard_file_type     );  // clipbrd.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, hostole_file_type       );  // olefile.cxx
  //REFERENCE_FOR_LINKER( const Abs_file_type&, inet_file_type          );  // inetfile.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, printers_file_type      );  // printers.cxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, winapi_file_type        );  // winapifl.cxx
#endif

#if defined __BORLANDC__
    REFERENCE_FOR_LINKER( const Abs_file_type&, dir_file_type           );  // dosdir.cxx
#endif

#if defined SYSTEM_SOLARIS
    REFERENCE_FOR_LINKER( const Abs_file_type&, mail_file_type          );  // mailfl.cxx
#endif

#if defined SYSTEM_STARVIEW && defined SYSTEM_WIN
    REFERENCE_FOR_LINKER( const Abs_file_type&, window_file_type        );  // logwin.cxx
#endif


#if defined SYSTEM_UNIX
    REFERENCE_FOR_LINKER( const Abs_file_type&, program_file_type       );  // progfile.cxx
#endif

  //DECLARE_FILE_TYPE_FILTER( Filter_file, new Dachhex ); // unschoen

# if defined SYSTEM_STARVIEWxxxxxxxxx
    REFERENCE_FOR_LINKER( const Abs_file_type&, fontwidth_file_type     );  // wprtfile.cxx
/*
        DECLARE_FILE_TYPE_NEW( Print_file );
        {
            const Abs_file_type* ft = &Print_file::static_file_type(); //Abs_file_type::_lookup( "printer" );
            if (ft) {
                LOG( "Print_file: name=" << ft->name() << " , alias="   << ft->alias_name() << endl );
            }
            if ( !ft ) ::exit(0); // und tschuess ...
        }

        DECLARE_FILE_TYPE_NEW( Fontwidth_file );
*/
# endif

# if defined SYSTEM_DOS
        DECLARE_FILE_TYPE_NEW( Dosdir );
# endif

  //DECLARE_FILE_TYPE_NEW( System_file );
}

} //namespace sos
