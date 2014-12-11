package com.sos.scheduler.taskserver.comrpc

/**
 * @author Joacim Zschimmer
 */
case class ProxyId(value: Long)

object ProxyId {
  val Null = ProxyId(0)
}
