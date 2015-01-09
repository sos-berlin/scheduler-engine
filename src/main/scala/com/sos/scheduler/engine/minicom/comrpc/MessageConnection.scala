package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.common.scalautil.Logger
import java.nio.ByteBuffer

/**
 * @author Joacim Zschimmer
 */
trait MessageConnection {
  def receiveMessage(): Option[ByteBuffer]
  def sendMessage(data: Array[Byte], length: Int): Unit
}

private object MessageConnection {
  private val logger = Logger(getClass)
}
