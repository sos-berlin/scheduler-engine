package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.cppproxy.OrderC

/**
  * @author Joacim Zschimmer
  */
@ForCpp
abstract class OrderCallback {
  @ForCpp
  def apply(orderC: OrderC): Unit
}
