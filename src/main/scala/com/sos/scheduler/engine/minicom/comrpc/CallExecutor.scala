package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.CallExecutor.ClsidToFactory
import com.sos.scheduler.engine.minicom.comrpc.calls._
import com.sos.scheduler.engine.minicom.types.{CLSID, IDispatch, IID, IUnknown}
import javax.inject.{Inject, Singleton}
import org.scalactic.Requirements._

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class CallExecutor @Inject private(clsidToFactory: ClsidToFactory, proxyRegister: ProxyRegister) {

  def execute(command: Call): Result = command match {
    case CreateInstanceCall(clsid, outer, context, iids) ⇒
      require(outer == None && context == 0 && iids.size == 1)
      val iunknown = clsidToFactory(clsid, iids.head)()
      CreateInstanceResult(iunknown)

    case ReleaseCall(proxyId) ⇒
      proxyRegister.removeProxy(proxyId)
      EmptyResult

    case CallCall(proxyId, methodName, arguments) ⇒
      proxyRegister(proxyId) match {
        case Some(o: IDispatch) ⇒ InvokeResult(IDispatch(o).call(methodName, arguments))
        case o ⇒ throw new IllegalArgumentException("No IDispatch")
      }
  }
}

object CallExecutor {
  type ClsidToFactory = ((CLSID, IID)) ⇒ (() ⇒ IUnknown)
}
