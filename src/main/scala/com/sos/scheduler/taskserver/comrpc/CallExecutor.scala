package com.sos.scheduler.taskserver.comrpc

import com.sos.scheduler.taskserver.comrpc.calls._
import com.sos.scheduler.taskserver.comtypes.ClsidToFactory
import javax.inject.{Inject, Singleton}
import org.scalactic.Requirements._

/**
 * @author Joacim Zschimmer
 */
@Singleton
private[comrpc] final class CallExecutor @Inject private(clsidToFactory: ClsidToFactory, proxyRegister: ProxyRegister) {

  def apply(command: Call): Result = command match {
    case CreateInstanceCall(clsid, outer, context, iids) ⇒
      require(outer == null && context == 0 && iids.size == 1)
      val iunknown = clsidToFactory(clsid, iids.head)()
      CreateInstanceResult(iunknown)

    case ReleaseCall(proxyId) ⇒
      proxyRegister.removeProxy(proxyId)
      EmptyResult

    case CallCall(proxyId, methodName, arguments) ⇒
      //val args = arguments map { case SimpleVariantSerializable(o) ⇒ o }
      proxyRegister(proxyId) match {
        case iDispatch: IDispatch ⇒
          val result = iDispatch.call(methodName, arguments)
          CallResult(result)
        case o ⇒ throw new IllegalArgumentException("No IDispatch")
      }
  }
}
