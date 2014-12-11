package com.sos.scheduler.taskserver.comrpc.calls

import com.sos.scheduler.taskserver.comrpc.types.{CLSID, DISPID, IID}
import com.sos.scheduler.taskserver.comrpc.{IUnknown, ProxyId}
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait Call

trait SessionCall extends Call

final case class CreateInstanceCall(
  clsid: CLSID,
  outer: IUnknown,
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
  flags: Int,
  unnamedArguments: immutable.Seq[Any],
  namedArguments: immutable.Seq[(DISPID, Any)])
  extends ObjectCall

final case class CallCall(proxyId: ProxyId, methodName: String, arguments: immutable.Seq[Any])
  extends ObjectCall

