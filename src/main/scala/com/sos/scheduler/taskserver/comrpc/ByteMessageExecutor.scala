package com.sos.scheduler.taskserver.comrpc

import com.sos.scheduler.taskserver.comrpc.calls._
import java.nio.ByteBuffer
import javax.inject.{Inject, Singleton}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class ByteMessageExecutor @Inject private(callExecutor: CallExecutor, proxyRegister: ProxyRegister) {

  def executeMessage(commandBuffer: ByteBuffer): (Array[Byte], Int) = {
    val deserializer = new CallDeserializer(proxyRegister, commandBuffer)
    val call = deserializer.readCall()
    val result = callExecutor.apply(call)
    serializeResult(result)
  }

  def serializeResult(result: Result): (Array[Byte], Int) = {
    val serializer = new CallSerializer(proxyRegister)
    serializer.writeResult(result)
    (serializer.byteBuffer.array, serializer.byteBuffer.position)
  }
}
