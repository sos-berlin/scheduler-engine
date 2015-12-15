package com.sos.scheduler.engine.kernel.processclass.common.selection

/**
  * @author Joacim Zschimmer
  */
trait Selector[A] {
  def iterator: BufferedIterator[A]
}
