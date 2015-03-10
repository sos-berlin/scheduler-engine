package com.sos.scheduler.engine.minicom.remoting.serial

import com.sos.scheduler.engine.minicom.idispatch.Invocable
import com.sos.scheduler.engine.minicom.remoting.serial.variantTypes._
import scala.runtime.BoxedUnit.UNIT

/**
 * @author Joacim Zschimmer
 */
private[remoting] abstract class VariantSerializer extends BaseSerializer {

  final def writeVariant(value: Any): Unit =
    value match {
      case o: Int ⇒ writeInt32(VT_I4); writeInt32(o)
      case o: Long ⇒ writeInt32(VT_I8); writeInt64(o)
      case o: Boolean ⇒ writeInt32(VT_BOOL); writeBoolean(o)
      case o: String ⇒ writeInt32(VT_BSTR); writeString(o)
      case o: Invocable ⇒ writeInt32(VT_DISPATCH); writeInvocable(Some(o))
      case null ⇒ writeInt32(VT_UNKNOWN); writeInvocable(None)
      case Unit | UNIT ⇒ writeInt32(VT_EMPTY)
    }

  def writeInvocable(iDispatch: Option[Invocable]): Unit = throw new UnsupportedOperationException("writeInvocable is not implemented")  // Method is overridden
}
