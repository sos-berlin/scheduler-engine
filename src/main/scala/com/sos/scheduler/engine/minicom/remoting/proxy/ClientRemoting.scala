package com.sos.scheduler.engine.minicom.remoting.proxy

import com.sos.scheduler.engine.minicom.remoting.calls.Call

/**
 * @author Joacim Zschimmer
 */
trait ClientRemoting {
  def sendReceive(call: Call): call.CallResult
}
