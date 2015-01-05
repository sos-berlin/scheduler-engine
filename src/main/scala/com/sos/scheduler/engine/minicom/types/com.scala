package com.sos.scheduler.engine.minicom.types

import java.util.UUID

/**
 * @author Joacim Zschimmer
 */
final case class CLSID(uuid: UUID)

object CLSID {
  val Null = CLSID(new UUID(0, 0))
}

final case class IID(uuid: UUID)

final case class DISPID(value: Int)

object hresult {
  final val S_OK = 0
  //  final val ERROR_INVALID_PARAMETER       = 87
  //  final val ERROR_INSUFFICIENT_BUFFER     = 122
  //
  //  final val NOERROR                       = 0
  //  final val S_OK                          = 0
  //  final val S_FALSE                       = 1
  //
  //  final val ERROR                         = 0x80000000
  //  final val FACILITY_WIN32                = 7
  //
  //  final val E_NOTIMPL                     = 0x80004001
  //  final val E_NOINTERFACE                 = 0x80004002
  //  final val E_POINTER                     = 0x80004003
  //  final val E_ABORT                       = 0x80004004
  //  final val E_FAIL                        = 0x80004005
  //  final val E_UNEXPECTED                  = 0x8000FFFF
  //
  //  final val DISP_E_UNKNOWNINTERFACE       = 0x80020001
  //  final val DISP_E_MEMBERNOTFOUND         = 0x80020003
  //  final val DISP_E_PARAMNOTFOUND          = 0x80020004
  //  final val DISP_E_TYPEMISMATCH           = 0x80020005
  //  final val DISP_E_UNKNOWNNAME            = 0x80020006
  //  final val DISP_E_NONAMEDARGS            = 0x80020007
  //  final val DISP_E_BADVARTYPE             = 0x80020008
  final val DISP_E_EXCEPTION              = 0x80020009
  //  final val DISP_E_OVERFLOW               = 0x8002000A
  //
  //  final val DISP_E_BADINDEX               = 0x8002000B
  //
  //  final val DISP_E_BADPARAMCOUNT          = 0x8002000E
  //
  //  final val TYPE_E_ELEMENTNOTFOUND        = 0x8002802B
  //
  //  final val CLASS_E_NOTLICENSED           = 0x80040112
  //  final val CLASS_E_CLASSNOTAVAILABLE     = 0x80040111
  //  final val CLASS_E_NOAGGREGATION         = 0x80040110
  //
  //  final val REGDB_E_WRITEREGDB            = 0x80040151
  //  final val REGDB_E_CLASSNOTREG           = 0x80040154
  //
  //  final val CO_E_NOTINITIALIZED           = 0x800401F0
  //  final val CO_E_DLLNOTFOUND              = 0x800401F8
  //  final val CO_E_ERRORINDLL               = 0x800401F9
  //
  //  final val E_ACCESSDENIED                = 0x80070005
  //  final val E_HANDLE                      = 0x80070006
  //  final val E_OUTOFMEMORY                 = 0x8007000E
  //  final val E_INVALIDARG                  = 0x80070057
  //
  //  final val CO_S_NOTALLINTERFACES         = 0x00080012
}

sealed trait DispatchType
object DISPATCH_METHOD extends DispatchType
object DISPATCH_PROPERTYPUT extends DispatchType
object DISPATCH_PROPERTYPUTREF extends DispatchType
object DISPATCH_PROPERTYGET extends DispatchType
