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
  val S_OK = 0
}

sealed trait DispatchType
object MethodDispatch extends DispatchType
object PropertyGetDispatch extends DispatchType
object PropertyPutDispatch extends DispatchType
