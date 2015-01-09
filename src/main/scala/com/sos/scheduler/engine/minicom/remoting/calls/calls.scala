package com.sos.scheduler.engine.minicom.remoting.calls

import com.sos.scheduler.engine.minicom.idispatch.{DISPID, DispatchType}
import com.sos.scheduler.engine.minicom.types.{CLSID, IID, IUnknown}
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
private[remoting] trait Call {
  type CallResult <: Result
}


private[remoting] trait SessionCall extends Call


private[remoting] final case class CreateInstanceCall(
  clsid: CLSID,
  outer: Option[IUnknown],
  context: Int,
  iids: immutable.Seq[IID])
extends SessionCall {
  type CallResult = CreateInstanceResult
}


private[remoting] trait ObjectCall extends Call {
  def proxyId: ProxyId
}


private[remoting] final case class ReleaseCall(proxyId: ProxyId)
extends ObjectCall {
  type CallResult = ReleaseResult.type
}


private[remoting] final case class QueryInterfaceCall(proxyId: ProxyId, iid: IID)
extends ObjectCall {
  type CallResult = QueryInterfaceResult
}


private[remoting] final case class GetIDsOfNamesCall(proxyId: ProxyId, iid: IID, localeId: Int, names: immutable.Seq[String])
extends ObjectCall {
  type CallResult = GetIDsOfNamesResult
}


private[remoting] final case class InvokeCall(
  proxyId: ProxyId,
  dispatchId: DISPID,
  iid: IID,
  dispatchTypes: immutable.Set[DispatchType],
  arguments: immutable.Seq[Any],
  namedArguments: immutable.Seq[(DISPID, Any)] = Nil)
extends ObjectCall {
  type CallResult = InvokeResult
}


private[remoting] final case class CallCall(proxyId: ProxyId, methodName: String, arguments: immutable.Seq[Any])
extends ObjectCall

