package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.common.scalautil.ScalaUtils.cast
import com.sos.scheduler.engine.minicom.Dispatcher.implicits._
import com.sos.scheduler.engine.minicom.comrpc.CallExecutor.CreateIDispatchableByCLSID
import com.sos.scheduler.engine.minicom.comrpc.calls._
import com.sos.scheduler.engine.minicom.types.{CLSID, IDispatchable, IID}
import javax.inject.{Inject, Singleton}
import org.scalactic.Requirements._

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class CallExecutor @Inject private(createIDispatchable: CreateIDispatchableByCLSID, proxyRegister: ProxyRegister) {

  def execute(call: Call): Result = call match {
    case CreateInstanceCall(clsid, outer, context, iids) ⇒
      require(outer == None && context == 0 && iids.size == 1)
      CreateInstanceResult(iDispatch = createIDispatchable(clsid, iids.head))

    case ReleaseCall(proxyId) ⇒
      proxyRegister.removeProxy(proxyId)
      EmptyResult

    case CallCall(proxyId, methodName, arguments) ⇒
      val iDispatchable = cast[IDispatchable](proxyRegister(proxyId))
      InvokeResult(result = iDispatchable.call(methodName, arguments))
  }
}

object CallExecutor {
  type CreateIDispatchableByCLSID = (CLSID, IID) ⇒ IDispatchable
}
