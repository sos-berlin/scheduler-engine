package com.sos.scheduler.taskserver.comrpc

import com.sos.scheduler.taskserver.comrpc.types.CLSID
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
private[comrpc] trait ProxyDeserializer extends COMDeserializer {

  protected val proxyRegister: ProxyRegister

  def readIUnknown() = {
    val proxyId = ProxyId(readInt64())
    val isNew = readBoolean()
    if (isNew) {
      // ???
      val name = readString()
      val proxyClasid = CLSID(readUUID())
      val n = readInt32()
      val proxyProperties = immutable.Seq.fill(n) {
        val name = readString()
        val value = readVariant()
        name → value
      }
    }
    proxyRegister.registerProxyId(proxyId)
  }
}
