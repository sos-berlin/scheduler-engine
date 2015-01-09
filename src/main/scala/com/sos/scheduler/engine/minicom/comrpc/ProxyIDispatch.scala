package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.CallSerializer.serializeCall
import com.sos.scheduler.engine.minicom.comrpc.calls.{Call, GetIDsOfNamesCall, GetIDsOfNamesResult, InvokeCall, InvokeResult, ProxyId}
import com.sos.scheduler.engine.minicom.types.{DISPATCH_METHOD, DISPID, DispatchType, IDispatch, IID}
import java.nio.ByteBuffer
import org.scalactic.Requirements._
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
trait ProxyIDispatch extends IDispatch {
  val id: ProxyId
  val name: String
  protected val proxyRegister: ProxyRegister
  protected val connection: MessageConnection

  final def getIdOfName(name: String) = {
    val byteBuffer = sendReceive(GetIDsOfNamesCall(id, IID.Null, localeId = 0, names = List(name)))
    val GetIDsOfNamesResult(dispIds) = new ResultDeserializer(connection, proxyRegister, byteBuffer).readGetIDsOfNamesResult()
    require(dispIds.size == 1)
    dispIds.head
  }

  final def invoke(dispId: DISPID, dispatchType: DispatchType, arguments: immutable.Seq[Any]) = {
    val byteBuffer = sendReceive(InvokeCall(id, dispId, IID.Null, Set(DISPATCH_METHOD), arguments))
    val InvokeResult(value) = new ResultDeserializer(connection, proxyRegister, byteBuffer).readInvokeResult()
    value
  }

  private def sendReceive(call: Call): ByteBuffer = {
    val (byteArray, length) = serializeCall(proxyRegister, call)
    connection.sendMessage(byteArray, length)
    connection.receiveMessage().get
  }
}

object ProxyIDispatch {
  final class Simple(
    protected val connection: MessageConnection,
    protected val proxyRegister: ProxyRegister,
    val id: ProxyId,
    val name: String)
  extends ProxyIDispatch
}
