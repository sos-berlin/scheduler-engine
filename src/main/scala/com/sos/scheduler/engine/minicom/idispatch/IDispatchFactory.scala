package com.sos.scheduler.engine.minicom.idispatch

import com.sos.scheduler.engine.minicom.types.{CLSID, IID}

/**
 * @author Joacim Zschimmer
 */
trait IDispatchFactory {
  def clsid: CLSID
  def iid: IID
  def apply(): IDispatchable
}
