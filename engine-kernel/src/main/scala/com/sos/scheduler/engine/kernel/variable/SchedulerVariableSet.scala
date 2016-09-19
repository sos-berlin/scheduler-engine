package com.sos.scheduler.engine.kernel.variable

import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class SchedulerVariableSet @Inject private[kernel](variableSet: VariableSet)
  (implicit schedulerThreadCallQueue: SchedulerThreadCallQueue) {

  def toMap: Map[String, String] = inSchedulerThread { variableSet.toMap }

  def apply(key: String): String = inSchedulerThread { variableSet(key) }

  def update(key: String, value: String): Unit = inSchedulerThread { variableSet(key) = value }

}
