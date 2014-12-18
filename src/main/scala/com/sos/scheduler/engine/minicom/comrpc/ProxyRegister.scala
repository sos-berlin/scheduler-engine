package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.comrpc.ProxyRegister._
import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.IUnknown
import javax.inject.{Inject, Singleton}
import scala.collection.mutable
import scala.util.Random

/**
 * @author Joacim Zschimmer
 */
@Singleton
private[comrpc] final class ProxyRegister @Inject private {
  private val proxyIdToProxy = mutable.Map[ProxyId, Entry]()
  private val iunknownToProxyId = mutable.Map[IUnknown, ProxyId]()
  private val proxyIdIterator = Iterator from 1 map { i: Int ⇒ ProxyId((Random.nextInt().toLong << 32) + i) }  // FIXME Eindeutig zu Proxy-IDs der Gegenstelle

  def registerIUnknown(iunknown: IUnknown): (ProxyId, Boolean) = {
    iunknownToProxyId.get(iunknown) match {
      case Some(o) ⇒ (o, false)
      case None ⇒
        val proxy = Entry(proxyIdIterator.next(), iunknown, registerIsOwner = true)
        proxyIdToProxy += proxy.proxyId → proxy
        iunknownToProxyId += proxy.iunknown → proxy.proxyId
        (proxy.proxyId, true)
    }
  }

  def registerProxyId(proxyId: ProxyId): IUnknown =
    if (proxyId == ProxyId.Null)
      null
    else
      proxyIdToProxy.get(proxyId) match {
        case Some(o) ⇒ o.iunknown
        case None ⇒
          val proxyIUnknown = ProxyIUnknown()
          val e = Entry(proxyId, proxyIUnknown, registerIsOwner = false)
          proxyIdToProxy += proxyId → e
          iunknownToProxyId += proxyIUnknown → proxyId
          e.iunknown
      }

  def removeProxy(proxyId: ProxyId) = {
    proxyIdToProxy.get(proxyId) match {
      case None ⇒
      case Some(proxy) ⇒
        iunknownToProxyId -= proxy.iunknown
        proxyIdToProxy -= proxyId
    }
  }

  def apply(proxyId: ProxyId): IUnknown = proxyIdToProxy(proxyId).iunknown
}

object ProxyRegister {
  private val logger = Logger(getClass)

  final case class Entry(
    proxyId: ProxyId,
    iunknown: IUnknown,
    registerIsOwner: Boolean)

  final case class ProxyIUnknown() extends IUnknown
}
