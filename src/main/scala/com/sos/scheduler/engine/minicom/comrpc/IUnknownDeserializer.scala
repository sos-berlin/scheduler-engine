package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.CLSID
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
private[comrpc] trait IUnknownDeserializer extends VariantDeserializer {

  protected val remoting: Remoting

  override final def readIDispatchableOption() = {
    val proxyId = ProxyId(readInt64())
    val isNew = readBoolean()
    if (isNew) {
      val name = readString()
      val proxyClsid = CLSID(readUUID())
      val proxyProperties = immutable.Seq.fill(readInt32()) {
        val name = readString()
        val value = readVariant()
        name → value
      }
      Some(remoting.newProxy(proxyId, name, proxyClsid, proxyProperties))
    }
    else
      proxyId match {
        case ProxyId.Null ⇒ None
        case _ ⇒ Some(remoting.iDispatchable(proxyId))
      }
  }
}
