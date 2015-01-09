package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.comrpc.CallDeserializer.deserializeCall
import com.sos.scheduler.engine.minicom.comrpc.ErrorSerializer.serializeError
import com.sos.scheduler.engine.minicom.comrpc.ResultSerializer.serializeResult
import com.sos.scheduler.engine.minicom.comrpc.calls._
import java.nio.ByteBuffer
import javax.inject.{Inject, Singleton}
import scala.util.control.NonFatal

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class ByteMessageExecutor @Inject private(executeCall: Call ⇒ Result, proxyRegister: ProxyRegister) {

  private def logger = Logger(getClass)

  /**
   * @return (Array, length)
   */
  def executeMessage(messageConnection: MessageConnection, callBuffer: ByteBuffer): (Array[Byte], Int) =
    try {
      val call = deserializeCall(messageConnection, proxyRegister, callBuffer)
      val result = executeCall(call)
      serializeResult(proxyRegister, result)
    }
    catch { case NonFatal(t) ⇒
      logger.debug(t.toString, t)
      serializeError(t)
    }
}
