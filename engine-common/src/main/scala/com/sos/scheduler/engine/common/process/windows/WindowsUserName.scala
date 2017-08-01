package com.sos.scheduler.engine.common.process.windows

import com.sos.scheduler.engine.base.generic.IsString
import java.util.Locale

/**
  * @author Joacim Zschimmer
  */
final case class WindowsUserName(string: String) extends IsString {
  override def equals(o: Any) = o match {
    case WindowsUserName(x) ⇒ string.toLowerCase(Locale.ROOT) == x.toLowerCase(Locale.ROOT)
    case _ ⇒ false
  }
}
