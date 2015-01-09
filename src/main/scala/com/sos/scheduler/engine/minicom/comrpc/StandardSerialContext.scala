package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.comrpc.CallDeserializer._
import com.sos.scheduler.engine.minicom.comrpc.CallExecutor._
import com.sos.scheduler.engine.minicom.comrpc.CallSerializer._
import com.sos.scheduler.engine.minicom.comrpc.ErrorSerializer._
import com.sos.scheduler.engine.minicom.comrpc.ResultSerializer._
import com.sos.scheduler.engine.minicom.comrpc.StandardSerialContext._
import com.sos.scheduler.engine.minicom.comrpc.calls.{ProxyId, Call}
import com.sos.scheduler.engine.minicom.types.IDispatchable
import java.nio.ByteBuffer
import scala.util.control.NonFatal

/**
 * @author Joacim Zschimmer
 */
final class StandardSerialContext(
  connection: MessageConnection,
  createIDispatchableByCLSID: CreateIDispatchableByCLSID)
extends SerialContext {

  private val proxyRegister = new ProxyRegister
  private val callExecutor = new CallExecutor(createIDispatchableByCLSID, proxyRegister)

  private[comrpc] def registerProxy(proxy: ProxyIDispatch): Unit = proxyRegister.registerProxy(proxy)

  private[comrpc] def iDispatchableOption(proxyId: ProxyId): Option[IDispatchable] = proxyRegister.iDispatchableOption(proxyId)

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

  def sendReceive(call: Call): ResultDeserializer = {
    val (byteArray, length) = serializeCall(proxyRegister, call)
    connection.sendMessage(byteArray, length)
    val byteBuffer = connection.receiveMessage().get
    new ResultDeserializer(this, byteBuffer)
  }
}

object StandardSerialContext {
  private val logger = Logger(getClass)
}
