package com.sos.scheduler.engine.taskserver.spoolerapi

import com.sos.scheduler.engine.data.log.SchedulerLogger
import com.sos.scheduler.engine.minicom.idispatch.IDispatchable

/**
 * @author Joacim Zschimmer
 */
trait SpoolerLog extends IDispatchable with SchedulerLogger
