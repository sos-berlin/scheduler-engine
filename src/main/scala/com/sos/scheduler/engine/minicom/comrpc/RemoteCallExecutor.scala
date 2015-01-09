package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.comrpc.calls.{Call, Result}

/**
 * @author Joacim Zschimmer
 */
final class RemoteCallExecutor {
  def execute(call: Call): Result = {
    val serializer: CallSerializer = ???
    serializer.writeCall(call)
    val deserializer: ResultDeserializer = ???
    deserializer.readInvokeResult()
  } 
}

private object RemoteCallExecutor {
  private val logger = Logger(getClass)
}
