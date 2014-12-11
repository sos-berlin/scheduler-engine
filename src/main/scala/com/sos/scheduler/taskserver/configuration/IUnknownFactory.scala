package com.sos.scheduler.taskserver.configuration

import com.sos.scheduler.taskserver.comrpc.IUnknown
import com.sos.scheduler.taskserver.comrpc.types.{CLSID, IID}

/**
 * @author Joacim Zschimmer
 */
trait IUnknownFactory[A] {
  def clsid: CLSID
  def iid: IID
  def apply(): IUnknown
}
