package com.sos.scheduler.engine.minicom.comrpc.calls

import org.jetbrains.annotations.TestOnly
import scala.util.Random

/**
 * @author Joacim Zschimmer
 */
case class ProxyId(value: Long) {
  @TestOnly private[comrpc] def index = (value >> 32).toInt
}

object ProxyId {
  val Null = ProxyId(0)
  def newGenerator(): Iterator[ProxyId] =
    Iterator from 1 map { i ⇒ ProxyId((i.toLong << 32) | (Random.nextInt().toLong & 0xffffffffL)) }  // FIXME Eindeutig zu Proxy-IDs der Gegenstelle / Implementierung wie RemoteTaskId
}
