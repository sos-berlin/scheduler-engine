package com.sos.scheduler.engine.minicom.remoting

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.idispatch.{IDispatchFactory, IDispatchable}
import com.sos.scheduler.engine.minicom.remoting.CallExecutor.CreateIDispatchableByCLSID
import com.sos.scheduler.engine.minicom.remoting.StandardRemoting._
import com.sos.scheduler.engine.minicom.remoting.calls.{Call, CreateInstanceCall, GetIDsOfNamesCall, InvokeCall, ProxyId}
import com.sos.scheduler.engine.minicom.remoting.proxy.{ClientRemoting, ProxyIDispatchFactory, ProxyRegister, SimpleProxyIDispatch}
import com.sos.scheduler.engine.minicom.remoting.serial.CallDeserializer._
import com.sos.scheduler.engine.minicom.remoting.serial.CallSerializer._
import com.sos.scheduler.engine.minicom.remoting.serial.ErrorSerializer._
import com.sos.scheduler.engine.minicom.remoting.serial.ResultSerializer._
import com.sos.scheduler.engine.minicom.remoting.serial.{ResultDeserializer, ServerRemoting}
import com.sos.scheduler.engine.minicom.types.{CLSID, IID}
import java.nio.ByteBuffer
import scala.collection.breakOut
import scala.util.control.NonFatal

/**
 * @author Joacim Zschimmer
 */
final class StandardRemoting(
  connection: MessageConnection,
  iDispatchFactories: Iterable[IDispatchFactory],
  proxyIDispatchFactories: Iterable[ProxyIDispatchFactory])
extends ServerRemoting with ClientRemoting {

  private val proxyRegister = new ProxyRegister
  private val callExecutor = new CallExecutor(toCreateIDispatchableByCLSID(iDispatchFactories), proxyRegister)
  private val proxyClsidMap: Map[CLSID, ProxyIDispatchFactory.Fun] =
    (List(SimpleProxyIDispatch) ++ proxyIDispatchFactories).map { o ⇒ o.clsid → o.apply _ } (breakOut)

  def run() = while (processNextMessage()) {}

  private def processNextMessage(): Boolean =
    connection.receiveMessage() match {
      case Some(callBytes) ⇒
        val (resultBytes, n) = executeMessage(callBytes)
        connection.sendMessage(resultBytes, n)
        true
      case None ⇒
        false
    }

  private def executeMessage(callBuffer: ByteBuffer): (Array[Byte], Int) =
    try {
      val call = deserializeCall(this, callBuffer)
      val result = callExecutor.execute(call)
      serializeResult(proxyRegister, result)
    }
    catch { case NonFatal(t) ⇒
      logger.debug(t.toString, t)
      serializeError(t)
    }

  private[minicom] def newProxy(proxyId: ProxyId, name: String, proxyClsid: CLSID, properties: Iterable[(String, Any)]) = {
    val newProxy = proxyClsidMap.getOrElse(proxyClsid, proxyClsidMap(CLSID.Null) /* Solange nicht alle Proxys implementiert sind */)
    val result = newProxy(this, proxyId, name, properties)
    proxyRegister.registerProxy(result)
    result
  }

  private[minicom] def iDispatchable(proxyId: ProxyId) = proxyRegister.iDispatchable(proxyId)

  def sendReceive(call: Call): call.CallResult = {
    val (byteArray, length) = serializeCall(proxyRegister, call)
    connection.sendMessage(byteArray, length)
    val byteBuffer = connection.receiveMessage().get
    val resultDeserializer = new ResultDeserializer(this, byteBuffer)
    (call match {
      case _: CreateInstanceCall ⇒ resultDeserializer.readCreateInstanceResult()
      case _: GetIDsOfNamesCall ⇒ resultDeserializer.readGetIDsOfNamesResult()
      case _: InvokeCall ⇒ resultDeserializer.readInvokeResult()
    }).asInstanceOf[call.CallResult]
  }
}

object StandardRemoting {
  private val logger = Logger(getClass)

  private def toCreateIDispatchableByCLSID(iDispatchFactories: Iterable[IDispatchFactory]): CreateIDispatchableByCLSID = {
    val clsidToFactoryMap = (iDispatchFactories map { o ⇒ o.clsid → o }).toMap
    def createIDispatchable(clsId: CLSID, iid: IID): IDispatchable = {
      val factory = clsidToFactoryMap(clsId)
      require(factory.iid == iid, s"IID $iid is not supported by $factory")
      factory()
    }
    createIDispatchable  // Return the function
  }
}
