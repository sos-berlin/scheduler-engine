package com.sos.scheduler.engine.minicom

import com.sos.scheduler.engine.minicom.types.{CLSID, IDispatchable, IID}

/**
 * @author Joacim Zschimmer
 */
trait IDispatchFactory {
  def clsid: CLSID
  def iid: IID
  def apply(): IDispatchable
}
