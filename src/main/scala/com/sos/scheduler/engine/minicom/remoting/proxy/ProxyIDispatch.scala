package com.sos.scheduler.engine.minicom.remoting.proxy

import com.sos.scheduler.engine.minicom.idispatch.{DISPID, DispatchType, IDispatch}
import com.sos.scheduler.engine.minicom.remoting.calls.ProxyId
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait ProxyIDispatch extends IDispatch {
  val id: ProxyId
  val name: String
  protected val remoting: ClientRemoting

  final def getIdOfName(name: String) = remoting.getIdOfName(id, name)

  final def invoke(dispId: DISPID, dispatchType: DispatchType, arguments: immutable.Seq[Any]) =
    remoting.invoke(id, dispId, dispatchType, arguments)
}
