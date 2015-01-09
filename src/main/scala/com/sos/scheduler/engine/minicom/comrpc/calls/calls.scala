package com.sos.scheduler.engine.minicom.comrpc.calls

import com.sos.scheduler.engine.minicom.types.{CLSID, DISPID, DispatchType, IID, IUnknown}
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait Call

trait SessionCall extends Call

final case class CreateInstanceCall(
  clsid: CLSID,
  outer: Option[IUnknown],
  context: Int,
  iids: immutable.Seq[IID])
extends SessionCall

trait ObjectCall extends Call {
  def proxyId: ProxyId
}

final case class ReleaseCall(proxyId: ProxyId)
extends ObjectCall

final case class QueryInterfaceCall(proxyId: ProxyId, iid: IID)
extends ObjectCall

final case class GetIDsOfNamesCall(proxyId: ProxyId, iid: IID, localeId: Int, names: immutable.Seq[String])
extends ObjectCall

final case class InvokeCall(
  proxyId: ProxyId,
  dispatchId: DISPID,
  iid: IID,
  dispatchTypes: immutable.Set[DispatchType],
  arguments: immutable.Seq[Any],
  namedArguments: immutable.Seq[(DISPID, Any)] = Nil)
extends ObjectCall

final case class CallCall(proxyId: ProxyId, methodName: String, arguments: immutable.Seq[Any])
extends ObjectCall

