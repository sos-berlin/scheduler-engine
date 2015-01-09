package com.sos.scheduler.engine.minicom.remoting.proxy

import com.sos.scheduler.engine.minicom.idispatch.{DISPID, DispatchType}
import com.sos.scheduler.engine.minicom.remoting.calls.ProxyId
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait ClientRemoting {

  private[remoting] def getIdOfName(proxyId: ProxyId, name: String): DISPID

  private[remoting] def invoke(proxyId: ProxyId, dispId: DISPID, dispatchType: DispatchType, arguments: immutable.Seq[Any]): Any
}
