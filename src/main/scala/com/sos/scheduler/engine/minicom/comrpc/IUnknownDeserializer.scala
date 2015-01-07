package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.CLSID
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
private[comrpc] trait IUnknownDeserializer extends VariantDeserializer {

  protected val proxyRegister: ProxyRegister

  override final def readIUnknown() = {
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
      proxyRegister.registerProxyId(proxyId, name)
    } else
      proxyRegister(proxyId)
  }
}
