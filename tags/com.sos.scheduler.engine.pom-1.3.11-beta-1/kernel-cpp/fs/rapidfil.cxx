//#define MODULE_NAME "rapidfil"
/* rapidfil.cpp                                     (c) SOS GmbH Berlin
                                                    Joacim Zschimmer

   Rapid-Dateiverarbeitung:
      Rapid::Fd
      Rapid::Openpar
*/


//#include <stdlib.h>

//#include <precomp.h>

#include "../kram/sos.h"
#include "../kram/xception.h"
#include "../kram/sosstrea.h"
#include "../file/absfile.h"
#include "rapid.h"

namespace sos {

//--------------------------------------------------------------------------------------read_fdattr

void read_fdattr( Sos_binary_istream* s, Rapid::Fd::Fdattr* fdattr_ptr )
{
    read_byte( s, &fdattr_ptr->_fdattr1 );
    read_byte( s, &fdattr_ptr->_fdattr2 );
    read_byte( s, &fdattr_ptr->_fdattr3 );
    read_byte( s, &fdattr_ptr->_fdattr4 );
}

//-------------------------------------------------------------------------------------write_fdattr

void write_fdattr( Sos_binary_ostream* s, const Rapid::Fd::Fdattr& fdattr )
{
    write_byte( s, fdattr._fdattr1 );
    write_byte( s, fdattr._fdattr2 );
    write_byte( s, fdattr._fdattr3 );
    write_byte( s, fdattr._fdattr4 );
}

//------------------------------------------------------------------------------------Rapid::Fd::Fd

Rapid::Fd::Fd()
:
  //_fdattr  ( 0 ),
    _fdftdad ( 0 ),
    _fdobfln  ( 0 ),
    _fdobfof  ( 0 ),
    _fdkp     ( 0x8000 ),
    _fdkl     ( 0 ),
    _fdgetloc ( 0 ),
    _fdbufadr ( 0 ),
    _fdibfptr ( 0 ),
    _fdibflen ( 0 ),
    _fdbufsiz ( 0 )
{
}

//------------------------------------------------------------------------------------------read_fd

void read_fd( Sos_binary_istream* s, Rapid::Fd* fd_ptr )
{
    read_bytes ( s,  fd_ptr->_link, 8 );
    read_fdattr( s, &fd_ptr->_fdattr );
                     fd_ptr->_fdftdad = s->read_uint4();
    read_uint2 ( s, &fd_ptr->_fdobfln );
    read_uint1 ( s, &fd_ptr->_fdobfof );
    read_byte  ( s, &fd_ptr->_reserve_1 );
    read_uint2 ( s, &fd_ptr->_fdkp );
    read_uint2 ( s, &fd_ptr->_fdkl );
                     fd_ptr->_fdgetloc = s->read_uint4();
                     fd_ptr->_fdbufadr = s->read_uint4();
                     fd_ptr->_fdibfptr = s->read_uint4();
    read_uint2 ( s, &fd_ptr->_fdibflen );
    read_uint2 ( s, &fd_ptr->_fdbufsiz );
}

//-----------------------------------------------------------------------------------------write_fd

void write_fd( Sos_binary_ostream* s, const Rapid::Fd& fd )
{
    write_bytes ( s, fd._link, 8 );
    write_fdattr( s, fd._fdattr   ) ;
    write_uint4 ( s, fd._fdftdad  );
    write_uint2 ( s, fd._fdobfln  );
    write_uint1 ( s, fd._fdobfof  );
    write_byte  ( s, fd._reserve_1 );
    write_uint2 ( s, fd._fdkp     );
    write_uint2 ( s, fd._fdkl     );
    write_uint4 ( s, fd._fdgetloc );
    write_uint4 ( s, fd._fdbufadr );
    write_uint4 ( s, fd._fdibfptr );
    write_uint2 ( s, fd._fdibflen );
    write_uint2 ( s, fd._fdbufsiz );
}

//-------------------------------------------------------Rapid::Fd::object_load
/*
Rapid::Fd& Rapid::Fd::object_load( istream& s )
{
    s.read( (char*)this, sizeof *this );
    return *this;
}

//-------------------------------------------------------Rapid::Fd::object_load

void Rapid::Fd::object_store( ostream& s )
{
    s.write( (const char*)this, sizeof *this );
}
*/

//-----------------------------------------------------------------Rapid::Openpar::Opflags::Opflags

Rapid::Openpar::Opflags::Opflags()
:
    _opflag1 ( 0 ),
    _opflag2 ( 0 ),
    _opflag3 ( 0 ),
    _opflag4 ( 0 )
{
}

//-------------------------------------------------------------------------------------read_opflags

void read_opflags( Sos_binary_istream* s, Rapid::Openpar::Opflags* opflags_ptr )
{
    read_byte( s, &opflags_ptr->_opflag1 );
    read_byte( s, &opflags_ptr->_opflag2 );
    read_byte( s, &opflags_ptr->_opflag3 );
    read_byte( s, &opflags_ptr->_opflag4 );
}

//------------------------------------------------------------------------------------write_opflags

void write_opflags( Sos_binary_ostream* s, const Rapid::Openpar::Opflags& opflags )
{
    s->write_byte( opflags._opflag1 );
    s->write_byte( opflags._opflag2 );
    s->write_byte( opflags._opflag3 );
    s->write_byte( opflags._opflag4 );
}

//------------------------------------------------------Rapid::Openpar::Openpar

Rapid::Openpar::Openpar()
:
    _opfirec  ( 0 ),
    _oprl     ( 0 ),
    _opdatadr ( 0 ),
    _oplibid  ( 0 ),
  //_opflags  ( 0 ),
    _opcmdaid ( 0 ),
    _opfddaid ( 0 ),
    _opcrdate ( 0 ),
    _opmddate ( 0 ),
    ___opkl   ( 0 ),
    ___opkp   ( 0 )
{
    //_optyp_string0   [ 0 ] = 0;
    memset( _optyp_string0, 0, sizeof _optyp_string0 );
    //_opname_string0  [ 0 ] = 0;
    memset( _opname_string0, 0, sizeof _opname_string0 );
    memset( _oppass, 0, sizeof _oppass );
    //_opnucnam_string0[ 0 ] = 0;
    memset( _opnucnam_string0, 0, sizeof _opnucnam_string0 );
    //_opcontxt_string0[ 0 ] = 0;
    memset( _opcontxt_string0, 0, sizeof _opcontxt_string0 );
    memset( _alignment_1, 0, sizeof _alignment_1 );
}

//--------------------------------------------------Rapid::Openpar::object_load
/*
Rapid::Openpar& Rapid::Openpar::object_load( istream& s )
{
    s.read( (char*)this, sizeof *this );
    return *this;
}

//--------------------------------------------------Rapid::Openpar::object_load

void Rapid::Openpar::object_store( ostream& s )
{
    s.write( (const char*)this, sizeof *this );
}
*/

//-------------------------------------------------------------------------------------read_openpar

void read_openpar( Sos_binary_istream* s, Rapid::Openpar* openpar_ptr )
{
    read_string_ebcdic( s,  openpar_ptr->_optyp_string0  , 8 );
    read_string_ebcdic( s,  openpar_ptr->_opname_string0 , rapid_fnamelen );
    read_bytes        ( s,  openpar_ptr->_oppass         , sizeof openpar_ptr->_oppass );
    read_int4         ( s, &openpar_ptr->_opfirec           );
    read_uint2        ( s, &openpar_ptr->_oprl     );
    read_bytes        ( s,  openpar_ptr->_alignment_1, sizeof openpar_ptr->_alignment_1 );
                            openpar_ptr->_opdatadr = s->read_uint4();
    read_uint4        ( s, &openpar_ptr->_oplibid  );
    read_string_ebcdic( s,  openpar_ptr->_opnucnam_string0 , 8 );
    read_opflags      ( s, &openpar_ptr->_opflags  );
    read_uint4        ( s, &openpar_ptr->_opcmdaid );
    read_uint4        ( s, &openpar_ptr->_opfddaid );
    read_integer8     ( s, &openpar_ptr->_opcrdate );
    read_integer8     ( s, &openpar_ptr->_opmddate );
    read_string_ebcdic( s,  openpar_ptr->_opcontxt_string0 , 8 );
}

//------------------------------------------------------------------------------------write_openpar

void write_openpar( Sos_binary_ostream* s, const Rapid::Openpar& openpar )
{
    write_string_ebcdic( s, openpar._optyp_string0   , 8 );
    write_string_ebcdic( s, openpar._opname_string0  , rapid_fnamelen );
    write_bytes        ( s, openpar._oppass          , sizeof openpar._oppass );
    write_uint4        ( s, openpar._opfirec  );
    write_uint2        ( s, openpar._oprl     );
    write_bytes        ( s, openpar._alignment_1, sizeof openpar._alignment_1 );
    write_uint4        ( s, openpar._opdatadr );
    write_uint4        ( s, openpar._oplibid  );
    write_string_ebcdic( s, openpar._opnucnam_string0, 8 );
    write_opflags      ( s, openpar._opflags  );
    write_uint4        ( s, openpar._opcmdaid );
    write_uint4        ( s, openpar._opfddaid );
    write_integer8     ( s, openpar._opcrdate );
    write_integer8     ( s, openpar._opmddate );
    write_string_ebcdic( s, openpar._opcontxt_string0, 8 );
}

//------------------------------------------------------------------------------------read_erasepar
/* Baustelle 1.10.98
void read_erasepar( Sos_binary_istream* s, Rapid::Erasepar* erasepar_ptr )
{
    read_int4         ( s, &erasepar._eralibid );
    read_int4         ( s, &erasepar._eracomid );
    read_string_ebcdic( s,  erasepar._eraname_string0, sizeof erasepar._eraname_string0 - 1 );
    read_string_ebcdic( s,  erasepar._eracontx_string0, sizeof erasepar._eracontx_string0 - 1 );
}
*/
//--------------------------------------------------------------------Rapid::Parflget::Flags::Flags

Rapid::Parflget::Flags::Flags()
{
    memset( _byte, 0, sizeof _byte );
}

//------------------------------------------------------------------------------read_parflget_flags

void read_parflget_flags( Sos_binary_istream* s, Rapid::Parflget::Flags* flags_ptr )
{
    read_bytes( s, flags_ptr->_byte, 4 );
}

//-----------------------------------------------------------------------------write_parflget_flags

void write_parflget_flags( Sos_binary_ostream* s, const Rapid::Parflget::Flags& flags )
{
    write_bytes( s, flags._byte, 4 );
}

//------------------------------------------------------------------------Rapid::Parflget::Parflget

Rapid::Parflget::Parflget()
:
  //_flags    ( 0 ),
    _getfdadr ( 0 ),
    _getbfadr ( 0 ),
    _getkyadr ( 0 ),
    _getkylen ( 0 ),
    _getnamad ( 0 ),
    _getnamln ( 0 ),
    _getendad ( 0 ),
    _getrccnt ( 0 )
{
}

//-----------------------------------------------------------------------------------write_parflget

void write_parflget( Sos_binary_ostream* s, const Rapid::Parflget& parflget )
{
    write_parflget_flags( s, parflget._flags    );
    write_uint4         ( s, parflget._getfdadr );
    write_uint4         ( s, parflget._getbfadr );
    write_int4          ( s, parflget._getbfsiz );
    write_uint4         ( s, parflget._getkyadr );
    write_int4          ( s, parflget._getkylen );
    write_uint4         ( s, parflget._getnamad );
    write_int4          ( s, parflget._getnamln );
    write_uint4         ( s, parflget._getendad );
    write_int4          ( s, parflget._getrccnt );
}

//------------------------------------------------------------------------------------read_parflget

void read_parflget( Sos_binary_istream* s, Rapid::Parflget* parflget_ptr )
{
    read_parflget_flags( s, &parflget_ptr->_flags    );
                    parflget_ptr->_getfdadr = s->read_uint4();
                    parflget_ptr->_getbfadr = s->read_uint4();
    read_int4 ( s, &parflget_ptr->_getbfsiz );
                    parflget_ptr->_getkyadr = s->read_uint4();
    read_int4 ( s, &parflget_ptr->_getkylen );
                    parflget_ptr->_getnamad = s->read_uint4();
    read_int4 ( s, &parflget_ptr->_getnamln );
                    parflget_ptr->_getendad = s->read_uint4();
    read_int4 ( s, &parflget_ptr->_getrccnt );
}

//--------------------------------------------------------------------Rapid::Parflput::Flags::Flags

Rapid::Parflput::Flags::Flags()
{
    memset( _byte, 0, sizeof _byte );
}

//-----------------------------------------------------------------------------write_parflput_flags

void write_parflput_flags( Sos_binary_ostream* s, const Rapid::Parflput::Flags& flags )
{
    write_bytes( s, flags._byte, 4 );
}

//------------------------------------------------------------------------------read_parflput_flags

void read_parflput_flags( Sos_binary_istream* s, Rapid::Parflput::Flags* flags_ptr )
{
    read_bytes( s, flags_ptr->_byte, 4 );
}

//------------------------------------------------------------------------Rapid::Parflput::Parflput

Rapid::Parflput::Parflput()
:
    _putfilid ( 0 ),
  //_flags    ( 0 ),
    _putadr   ( 0 ),
    _putlen   ( 0 ),
    _putkyadr ( 0 ),
    _putkylen ( 0 ),
    _putcalid ( 0 )
{
}

//------------------------------------------------------------------------------------read_parflput

void read_parflput( Sos_binary_istream* s, Rapid::Parflput* parflput_ptr )
{
    read_uint4         ( s, &parflput_ptr->_putfilid );
    read_parflput_flags( s, &parflput_ptr->_flags    );
                    parflput_ptr->_putadr   = s->read_uint4();
    read_int4 ( s, &parflput_ptr->_putlen   );
                    parflput_ptr->_putkyadr = s->read_uint4();
    read_int4 ( s, &parflput_ptr->_putkylen );
    read_uint4( s, &parflput_ptr->_putcalid );
}

//------------------------------------------------------------------------Rapid::Erasepar::Erasepar

Rapid::Erasepar::Erasepar()
:
    _eraflags ( 0 ),
    _eralibid ( 0 ),
    _eracomid ( 0 )
{
    memset( _reserve, 0, 3 );
    _eraname_string0[ 0 ] = 0;
    _eracontx_string0[ 0 ] = 0;
}

//------------------------------------------------------------------------------------read_erasepar

void read_erasepar( Sos_binary_istream* s, Rapid::Erasepar* erasepar_ptr )
{
    read_bytes        ( s, &erasepar_ptr->_reserve, 3 );
    read_byte         ( s, &erasepar_ptr->_eraflags );
    read_uint4        ( s, &erasepar_ptr->_eralibid );
    read_uint4        ( s, &erasepar_ptr->_eracomid );
    read_string_ebcdic( s,  erasepar_ptr->_eraname_string0, rapid_fnamelen );
    read_string_ebcdic( s,  erasepar_ptr->_eracontx_string0, 8 );
}

//-----------------------------------------------------------------------------------write_erasepar

void write_erasepar( Sos_binary_ostream* s, const Rapid::Erasepar& erasepar )
{
    write_bytes        ( s, erasepar._reserve, 3 );
    write_byte         ( s, erasepar._eraflags );
    write_uint4        ( s, erasepar._eralibid );
    write_uint4        ( s, erasepar._eracomid );
    write_string_ebcdic( s, erasepar._eraname_string0, rapid_fnamelen );
    write_string_ebcdic( s, erasepar._eracontx_string0, 8 );
}

//------------------------------------------------------------------------Rapid::Renampar::Renampar

Rapid::Renampar::Renampar()
:
    _renflags ( 0 ),
    _renlibid ( 0 ),
    _rencomid ( 0 )
{
    memset( _reserve, 0, sizeof _reserve );
    _renoldna_string0[ 0 ] = 0;
    _rennewna_string0[ 0 ] = 0;
    _rencontx_string0[ 0 ] = 0;
}

//------------------------------------------------------------------------------------read_renampar

void read_renampar( Sos_binary_istream* s, Rapid::Renampar* renampar_ptr )
{
    read_bytes        ( s, &renampar_ptr->_reserve, 3 );
    read_byte         ( s, &renampar_ptr->_renflags );
    read_uint4        ( s, &renampar_ptr->_renlibid );
    read_uint4        ( s, &renampar_ptr->_rencomid );
    read_string_ebcdic( s,  renampar_ptr->_renoldna_string0, rapid_fnamelen );
    read_string_ebcdic( s,  renampar_ptr->_rennewna_string0, rapid_fnamelen );
    read_string_ebcdic( s,  renampar_ptr->_rencontx_string0, 8 );
}

//------------------------------------------------------------------------Rapid::Fileinfo::Fileinfo

Rapid::Fileinfo::Fileinfo()
:
    _ficrstck        ( 0 ),
    _fimdstck        ( 0 ),
    _filastck        ( 0 ),
    _fiexstck        ( 0 ),
    _fisize          ( 0 ),
    _fireccnt        ( 0 ),
    _fispace         ( 0 ),
    _fispace2        ( 0 ),
  //_fiattr          ( 0 ),
    _fiblksiz        ( 0 ),
    _firl            ( 0 ),
    _fikl            ( 0 ),
    _fikp            ( 0 ),
    _filibid         ( 0 ),
    _finameln        ( 0 ),
    _fimbnaln        ( 0 )
{
    memset( _access_flags, 0, sizeof _access_flags );
    memset( _x_flags     , 0, sizeof _x_flags      );
    memset( _status_flags, 0, sizeof _status_flags );
    memset( _reserve4    , 0, sizeof _reserve4     );
    _ficlass          [ 0 ] = '\0';
    _fityp_string0    [ 0 ] = '\0';
    _finame_string0   [ 0 ] = '\0';
    _ficommen_string0 [ 0 ] = '\0';
    _fimbname_string0 [ 0 ] = '\0';
}

//------------------------------------------------------------------------------------read_fileinfo

void read_fileinfo( Sos_binary_istream* s, Rapid::Fileinfo* fileinfo_ptr )
{
    read_string_ebcdic( s,  fileinfo_ptr->_fityp_string0   , 8 );
    read_string_ebcdic( s,  fileinfo_ptr->_finame_string0  , rapid_fnamelen );
    read_bytes        ( s,  fileinfo_ptr->_ficlass         , 8 );
    read_string_ebcdic( s,  fileinfo_ptr->_ficommen_string0, 80 );
    read_integer8     ( s, &fileinfo_ptr->_ficrstck            );
    read_integer8     ( s, &fileinfo_ptr->_fimdstck            );
    read_integer8     ( s, &fileinfo_ptr->_filastck            );
    read_integer8     ( s, &fileinfo_ptr->_fiexstck            );
    read_int4         ( s, &fileinfo_ptr->_fisize );
    read_int4         ( s, &fileinfo_ptr->_fireccnt );
    read_int4         ( s, &fileinfo_ptr->_fispace);
    read_int4         ( s, &fileinfo_ptr->_fispace2 );
    read_bytes        ( s,  fileinfo_ptr->_access_flags    , 4 );
    read_bytes        ( s,  fileinfo_ptr->_x_flags         , 4 );
    read_bytes        ( s,  fileinfo_ptr->_status_flags    , 4 );
    read_fdattr       ( s, &fileinfo_ptr->_fiattr );
    read_int4         ( s, &fileinfo_ptr->_fiblksiz );
    read_uint2        ( s, &fileinfo_ptr->_firl );
    read_uint2        ( s, &fileinfo_ptr->_fikl );
    read_uint2        ( s, &fileinfo_ptr->_fikp );
    read_bytes        ( s,  fileinfo_ptr->_reserve4        , 8 );
    read_string_ebcdic( s,  fileinfo_ptr->_fimbname_string0 , rapid_membername_size );
    read_uint4        ( s, &fileinfo_ptr->_filibid );
    read_int2         ( s, &fileinfo_ptr->_finameln );
    read_int2         ( s, &fileinfo_ptr->_fimbnaln );
}

//-----------------------------------------------------------------------------------write_fileinfo

void write_fileinfo( Sos_binary_ostream* s, const Rapid::Fileinfo& fileinfo )
{
    write_string_ebcdic( s, fileinfo._fityp_string0   , 8 );
    write_string_ebcdic( s, fileinfo._finame_string0  , rapid_fnamelen );
    write_bytes        ( s, fileinfo._ficlass         , 8 );
    write_string_ebcdic( s, fileinfo._ficommen_string0, 80 );
    write_integer8     ( s, fileinfo._ficrstck            );
    write_integer8     ( s, fileinfo._fimdstck            );
    write_integer8     ( s, fileinfo._filastck            );
    write_integer8     ( s, fileinfo._fiexstck            );
    write_int4         ( s, fileinfo._fisize );
    write_int4         ( s, fileinfo._fireccnt );
    write_int4         ( s, fileinfo._fispace);
    write_int4         ( s, fileinfo._fispace2 );
    write_bytes        ( s, fileinfo._access_flags    , 4 );
    write_bytes        ( s, fileinfo._x_flags         , 4 );
    write_bytes        ( s, fileinfo._status_flags    , 4 );
    write_fdattr       ( s, fileinfo._fiattr );
    write_int4         ( s, fileinfo._fiblksiz );
    write_uint2        ( s, fileinfo._firl );
    write_uint2        ( s, fileinfo._fikl );
    write_uint2        ( s, fileinfo._fikp );
    write_bytes        ( s, fileinfo._reserve4        , 8 );
    write_string_ebcdic( s, fileinfo._fimbname_string0 , rapid_membername_size );
    write_uint4        ( s, fileinfo._filibid );
    write_int2         ( s, fileinfo._finameln );
    write_int2         ( s, fileinfo._fimbnaln );
}

//------------------------------------------------------------------------Rapid::Parflinf::Parflinf

Rapid::Parflinf::Parflinf()
:
    _inffid   ( 0 ),
    _infnamad ( 0 ),
    _infnamln ( 0 ),
    _infaradr ( 0 ),
    _infarsiz ( 0 )
{
    memset( _flags, 0, sizeof _flags );
}

//------------------------------------------------------------------------------------read_parflinf

void read_parflinf( Sos_binary_istream* s, Rapid::Parflinf* parflinf_ptr )
{
    read_bytes        ( s, &parflinf_ptr->_flags    , 4 );
    read_uint4        ( s, &parflinf_ptr->_inffid );
                            parflinf_ptr->_infnamad = s->read_uint4();
    read_int4         ( s, &parflinf_ptr->_infnamln );
                            parflinf_ptr->_infaradr = s->read_uint4();
    read_int4         ( s, &parflinf_ptr->_infarsiz );
}


} //namespace sos
