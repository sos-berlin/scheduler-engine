package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.{CLSID, IDispatchable}

/**
 * @author Joacim Zschimmer
 */
private[comrpc] abstract class IDispatchSerializer
extends VariantSerializer {

  protected val proxyRegister: ProxyRegister

  override final def writeIDispatchable(iDispatchableOption: Option[IDispatchable]): Unit = {
    val (proxyId, isNew) = iDispatchableOption map proxyRegister.iDispatchToProxyId getOrElse (ProxyId.Null, false)
    writeInt64(proxyId.value)
    writeBoolean(isNew)
    if (isNew) {
      writeString(iDispatchableOption.getClass.getSimpleName)
      writeUUID(CLSID.Null.uuid)
      writeInt32(0)
//      writeUUID(localProxy.clsid.uuid)
//      writeInt32(localProxy.properties.size)
//      for ((name, v) ← localProxy.properties) {
//        writeString(name)
//        writeVariant(v)
//      }
    }
  }
}
