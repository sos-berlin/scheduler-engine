package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.IDispatchFactory
import com.sos.scheduler.engine.minicom.comrpc.CallDeserializer._
import com.sos.scheduler.engine.minicom.comrpc.CallExecutor._
import com.sos.scheduler.engine.minicom.comrpc.CallSerializer._
import com.sos.scheduler.engine.minicom.comrpc.ErrorSerializer._
import com.sos.scheduler.engine.minicom.comrpc.ResultSerializer._
import com.sos.scheduler.engine.minicom.comrpc.StandardRemoting._
import com.sos.scheduler.engine.minicom.comrpc.calls.{Call, ProxyId}
import com.sos.scheduler.engine.minicom.types.{CLSID, IDispatchable, IID}
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
extends Remoting {

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

  private[comrpc] def newProxy(proxyId: ProxyId, name: String, proxyClsid: CLSID, properties: Iterable[(String, Any)]) = {
    val f = proxyClsidMap.getOrElse(proxyClsid, proxyClsidMap(CLSID.Null) /* Solange nicht alle Proxys implementiert sind */)
    val result = f(this, proxyId, name, properties)
    proxyRegister.registerProxy(result)
    result
  }

  private[comrpc] def iDispatchable(proxyId: ProxyId) = proxyRegister.iDispatchable(proxyId)

  def sendReceive(call: Call): ResultDeserializer = {
    val (byteArray, length) = serializeCall(proxyRegister, call)
    connection.sendMessage(byteArray, length)
    val byteBuffer = connection.receiveMessage().get
    new ResultDeserializer(this, byteBuffer)
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
