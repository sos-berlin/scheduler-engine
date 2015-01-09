package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.CLSID
import com.sos.scheduler.engine.taskserver.spoolerapi.ProxySpoolerLog
import java.util.UUID
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
private[comrpc] trait IUnknownDeserializer extends VariantDeserializer {

  protected val connection: MessageConnection
  protected val proxyRegister: ProxyRegister

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
      val proxy = if (proxyClsid == CLSID(UUID.fromString("feee47a6-6c1b-11d8-8103-000476ee8afb"))) {
        new ProxySpoolerLog(connection, proxyRegister, proxyId, name)
      } else 
        new ProxyIDispatch.Simple(connection, proxyRegister, proxyId, name)
      proxyRegister.registerProxy(proxy)
      Some(proxy)
    } else
      proxyRegister.iDispatchableOption(proxyId)
  }
}
