package com.sos.scheduler.engine.minicom.comrpc

import com.google.common.collect.HashBiMap
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.comrpc.ProxyRegister._
import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.HRESULT.E_POINTER
import com.sos.scheduler.engine.minicom.types.{COMException, IDispatchable}
import javax.inject.{Inject, Singleton}
import scala.collection.JavaConversions._
import scala.util.control.NonFatal

/**
 * @author Joacim Zschimmer
 */
@Singleton
/*TODO private[comrpc]*/ final class ProxyRegister @Inject private {
  private val proxyIdToIDispatch = HashBiMap.create[ProxyId, IDispatchable]()
  private val iunknownToProxyId = proxyIdToIDispatch.inverse
  private val proxyIdGenerator = ProxyId.newGenerator()

  def registerProxy(proxy: ProxyIDispatch): Unit = add(proxy.id, proxy)

  def iDispatchToProxyId(iDispatch: IDispatchable): (ProxyId, Boolean) =
    synchronized {
      iunknownToProxyId.get(iDispatch) match {
        case null ⇒
          val proxyId = proxyIdGenerator.next()
          add(proxyId, iDispatch)
          (proxyId, true)
        case o ⇒ (o, false)
      }
    }

  private def add(proxyId: ProxyId, iDispatch: IDispatchable): Unit =
    synchronized {
      if (proxyIdToIDispatch containsKey proxyId) throw new DuplicateKeyException(s"$proxyId already registered")
      if (iunknownToProxyId containsKey iDispatch) throw new DuplicateKeyException(s"IDispatch '$iDispatch' already registered")
      proxyIdToIDispatch.put(proxyId, iDispatch)
    }

  def removeProxy(proxyId: ProxyId): Unit =
    synchronized {
      proxyIdToIDispatch.remove(proxyId) match {
        case o: AutoCloseable ⇒
          try o.close()
          catch { case NonFatal(t) ⇒ logger.error(s"Suppressed: $t", t) }
        case _ ⇒
      }
    }

  def iDispatchableOption(proxyId: ProxyId): Option[IDispatchable] =
    if (proxyId == ProxyId.Null) None else Some(apply(proxyId))

  def apply(proxyId: ProxyId): IDispatchable =
    if (proxyId == ProxyId.Null) throw new COMException(E_POINTER)
    else synchronized { proxyIdToIDispatch(proxyId) }

  override def toString = s"${getClass.getSimpleName}($size proxies)"

  def size = proxyIdToIDispatch.size
}

object ProxyRegister {
  private val logger = Logger(getClass)

  class DuplicateKeyException(override val getMessage: String) extends RuntimeException
}
