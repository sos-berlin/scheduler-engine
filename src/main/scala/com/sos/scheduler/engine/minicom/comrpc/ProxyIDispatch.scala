package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.{GetIDsOfNamesCall, GetIDsOfNamesResult, InvokeCall, InvokeResult, ProxyId}
import com.sos.scheduler.engine.minicom.types.{DISPATCH_METHOD, DISPID, DispatchType, IDispatch, IID}
import org.scalactic.Requirements._
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait ProxyIDispatch extends IDispatch {
  val id: ProxyId
  val name: String
  protected val serialContext: SerialContext

  final def getIdOfName(name: String) = {
    val call = GetIDsOfNamesCall(id, IID.Null, localeId = 0, names = List(name))
    val GetIDsOfNamesResult(dispIds) = serialContext.sendReceive(call).readGetIDsOfNamesResult()
    require(dispIds.size == 1)
    dispIds.head
  }

  final def invoke(dispId: DISPID, dispatchType: DispatchType, arguments: immutable.Seq[Any]) = {
    val call = InvokeCall(id, dispId, IID.Null, Set(DISPATCH_METHOD), arguments)
    val InvokeResult(value) = serialContext.sendReceive(call).readInvokeResult()
    value
  }
}

object ProxyIDispatch {
  final class Simple(
    protected val serialContext: SerialContext,
    val id: ProxyId,
    val name: String)
  extends ProxyIDispatch
}
