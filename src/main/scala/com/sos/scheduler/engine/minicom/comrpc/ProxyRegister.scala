package com.sos.scheduler.engine.minicom.comrpc

import com.google.common.collect.HashBiMap
import com.sos.scheduler.engine.minicom.comrpc.ProxyRegister._
import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.IUnknown
import javax.annotation.Nullable
import javax.inject.{Inject, Singleton}
import scala.collection.JavaConversions._

/**
 * @author Joacim Zschimmer
 */
@Singleton
private[comrpc] final class ProxyRegister @Inject private {
  private val proxyIdToIUnknown = HashBiMap.create[ProxyId, IUnknown]()
  private val iunknownToProxyId = proxyIdToIUnknown.inverse
  private val proxyIdGenerator = ProxyId.newGenerator()

  def registerProxyId(proxyId: ProxyId, name: String): Option[IUnknown] =
    if (proxyId == ProxyId.Null)
      None
    else {
      val iUnknown = ProxyIUnknown(proxyId, name)
      add(proxyId, iUnknown)
      Some(iUnknown)
    }

  def iUnknownToProxyId(iunknown: IUnknown): (ProxyId, Boolean) =
    synchronized {
      iunknownToProxyId.get(iunknown) match {
        case null ⇒
          val proxyId = proxyIdGenerator.next()
          add(proxyId, iunknown)
          (proxyId, true)
        case o ⇒ (o, false)
      }
    }

  private def add(proxyId: ProxyId, iUnknown: IUnknown): Unit =
    synchronized {
      if (proxyIdToIUnknown containsKey proxyId) throw new DuplicateKeyException(s"$proxyId already registered")
      if (iunknownToProxyId containsKey iUnknown) throw new DuplicateKeyException(s"IUnknown '$iUnknown' already registered")
      proxyIdToIUnknown.put(proxyId, iUnknown)
    }

  def removeProxy(proxyId: ProxyId): Unit =
    synchronized {
      proxyIdToIUnknown.remove(proxyId)
    }

  @Nullable
  def apply(proxyId: ProxyId): Option[IUnknown] =
    if (proxyId == ProxyId.Null)
      None
    else
      Some(synchronized { proxyIdToIUnknown(proxyId) })

  override def toString = s"${getClass.getSimpleName}($size proxies)"

  def size = proxyIdToIUnknown.size
}

object ProxyRegister {
  final case class ProxyIUnknown(id: ProxyId, name: String) extends IUnknown
  class DuplicateKeyException(override val getMessage: String) extends RuntimeException
}
