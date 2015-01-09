package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.CLSID
import com.sos.scheduler.engine.taskserver.spoolerapi.ProxySpoolerLog
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
private[comrpc] trait IUnknownDeserializer extends VariantDeserializer {

  protected val serialContext: SerialContext

  override final def readIDispatchableOption() = {
    val proxyId = ProxyId(readInt64())
    val isNew = readBoolean()
    if (isNew) {
      // TODO Proxy properties
      val name = readString()
      val proxyClsid = CLSID(readUUID())
      val proxyProperties = immutable.Seq.fill(readInt32()) {
        val name = readString()
        val value = readVariant()
        name → value
      }
      val proxy = proxyClsid match {
        case ProxySpoolerLog.clsid ⇒ new ProxySpoolerLog(serialContext, proxyId, name)  // TODO Proxy CLSID register, CreateIDispatchableByCLSID?
        case _/*TODO CLSID.Null*/ ⇒ new ProxyIDispatch.Simple(serialContext, proxyId, name)
        //case o ⇒ throw new IllegalArgumentException(s"Unknown proxy $o")
      }
      serialContext.registerProxy(proxy)
      Some(proxy)
    } else
      serialContext.iDispatchableOption(proxyId)
  }
}
