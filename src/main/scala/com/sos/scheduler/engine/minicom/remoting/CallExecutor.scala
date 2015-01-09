package com.sos.scheduler.engine.minicom.remoting

import com.sos.scheduler.engine.common.scalautil.ScalaUtils.cast
import com.sos.scheduler.engine.minicom.idispatch.Dispatcher.implicits._
import com.sos.scheduler.engine.minicom.idispatch.IDispatchable
import com.sos.scheduler.engine.minicom.remoting.CallExecutor.CreateIDispatchableByCLSID
import com.sos.scheduler.engine.minicom.remoting.calls._
import com.sos.scheduler.engine.minicom.remoting.proxy.ProxyRegister
import com.sos.scheduler.engine.minicom.types.{CLSID, IID}
import org.scalactic.Requirements._

/**
 * @author Joacim Zschimmer
 */
private[remoting] final class CallExecutor(createIDispatchable: CreateIDispatchableByCLSID, proxyRegister: ProxyRegister) {

  def execute(call: Call): Result = call match {
    case CreateInstanceCall(clsid, outer, context, iids) ⇒
      require(outer == None && context == 0 && iids.size == 1)
      CreateInstanceResult(iDispatch = createIDispatchable(clsid, iids.head))

    case ReleaseCall(proxyId) ⇒
      proxyRegister.removeProxy(proxyId)
      EmptyResult

    case CallCall(proxyId, methodName, arguments) ⇒
      val iDispatchable = cast[IDispatchable](proxyRegister.iDispatchable(proxyId))
      InvokeResult(result = iDispatchable.call(methodName, arguments))
  }
}

object CallExecutor {
  type CreateIDispatchableByCLSID = (CLSID, IID) ⇒ IDispatchable
}
