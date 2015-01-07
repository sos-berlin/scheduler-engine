package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.types.Variant._
import com.sos.scheduler.engine.minicom.types.{IDispatchable, IUnknown}
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
      case o: IDispatchable ⇒ writeInt32(VT_DISPATCH); writeIUnknown(Some(o))
      case o: IUnknown ⇒ writeInt32(VT_UNKNOWN); writeIUnknown(Some(o))
      case null ⇒ writeInt32(VT_UNKNOWN); writeIUnknown(None)
      case Unit | UNIT ⇒ writeInt32(VT_EMPTY)
    }

  def writeIUnknown(iUnknown: Option[IUnknown]): Unit = throw new UnsupportedOperationException("IUnknown is not supported")  // Method is overridden
}
