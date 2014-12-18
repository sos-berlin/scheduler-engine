package com.sos.scheduler.engine.minicom.comrpc.calls

/**
 * @author Joacim Zschimmer
 */
case class ProxyId(value: Long)

object ProxyId {
  val Null = ProxyId(0)
}
