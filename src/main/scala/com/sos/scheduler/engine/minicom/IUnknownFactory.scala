package com.sos.scheduler.engine.minicom

import com.sos.scheduler.engine.minicom.types.{CLSID, IID, IUnknown}

/**
 * @author Joacim Zschimmer
 */
trait IUnknownFactory {
  def clsid: CLSID
  def iid: IID
  def apply(): IUnknown
}
