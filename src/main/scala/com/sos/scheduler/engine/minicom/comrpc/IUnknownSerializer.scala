package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.{CLSID, IUnknown}

/**
 * @author Joacim Zschimmer
 */
private[comrpc] abstract class IUnknownSerializer
extends VariantSerializer {

  protected val iunknownProxyRegister: ProxyRegister

  override final def writeIUnknown(iUnknownOption: Option[IUnknown]): Unit = {
    val (proxyId, isNew) = iUnknownOption map iunknownProxyRegister.iUnknownToProxyId getOrElse (ProxyId.Null, false)
    writeInt64(proxyId.value)
    writeBoolean(isNew)
    if (isNew) {
      writeString(iUnknownOption.getClass.getSimpleName)
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
