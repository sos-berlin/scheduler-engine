package com.sos.scheduler.engine.minicom.remoting.proxy

import com.sos.scheduler.engine.minicom.idispatch.{DISPATCH_METHOD, DISPID, DispatchType, IDispatch}
import com.sos.scheduler.engine.minicom.remoting.calls.{GetIDsOfNamesCall, GetIDsOfNamesResult, InvokeCall, InvokeResult, ProxyId}
import com.sos.scheduler.engine.minicom.types.IID
import org.scalactic.Requirements._
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait ProxyIDispatch extends IDispatch {
  val id: ProxyId
  val name: String
  protected val remoting: ClientRemoting

  final def getIdOfName(name: String) = {
    val call = GetIDsOfNamesCall(id, IID.Null, localeId = 0, names = List(name))
    val GetIDsOfNamesResult(dispIds) = remoting.sendReceive(call)
    require(dispIds.size == 1)
    dispIds.head
  }

  final def invoke(dispId: DISPID, dispatchType: DispatchType, arguments: immutable.Seq[Any]) = {
    val call = InvokeCall(id, dispId, IID.Null, Set(DISPATCH_METHOD), arguments)
    val InvokeResult(value) = remoting.sendReceive(call)
    value
  }
}
