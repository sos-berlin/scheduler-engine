package com.sos.scheduler.engine.taskserver.data.module

/**
  * @author Joacim Zschimmer
  */
import com.sos.scheduler.engine.minicom.idispatch.IDispatch
import com.sos.scheduler.engine.taskserver.moduleapi.Module
import com.sos.scheduler.engine.taskserver.spoolerapi.TypedNamedIDispatches
import sos.spooler.Job_impl

/**
  * @author Joacim Zschimmer
  */
trait IDispatchModule extends Module {
  def newInstance(namedIDispatches: TypedNamedIDispatches): IDispatch
}
