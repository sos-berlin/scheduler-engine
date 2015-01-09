package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.types.IDispatchable
import com.sos.scheduler.engine.minicom.types.Variant._
import scala.runtime.BoxedUnit.UNIT

/**
 * @author Joacim Zschimmer
 */
private[comrpc] abstract class VariantSerializer extends BaseSerializer {

  final def writeVariant(value: Any): Unit =
    value match {
      case o: Int ⇒ writeInt32(VT_I4); writeInt32(o)
      case o: Long ⇒ writeInt32(VT_I8); writeInt64(o)
      case o: Boolean ⇒ writeInt32(VT_BOOL); writeBoolean(o)
      case o: String ⇒ writeInt32(VT_BSTR); writeString(o)
      case o: IDispatchable ⇒ writeInt32(VT_DISPATCH); writeIDispatchable(Some(o))
      case null ⇒ writeInt32(VT_UNKNOWN); writeIDispatchable(None)
      case Unit | UNIT ⇒ writeInt32(VT_EMPTY)
    }

  def writeIDispatchable(iDispatch: Option[IDispatchable]): Unit = throw new UnsupportedOperationException("writeIDispatchable is not implemented")  // Method is overridden
}
