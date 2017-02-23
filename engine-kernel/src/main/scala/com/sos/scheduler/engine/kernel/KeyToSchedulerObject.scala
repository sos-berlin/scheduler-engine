package com.sos.scheduler.engine.kernel

import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.data.monitor.MonitorPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.lock.LockSubsystem
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, StandingOrderSubsystem}
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystem
import com.sos.scheduler.engine.kernel.schedule.ScheduleSubsystem
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
private[kernel] final class KeyToSchedulerObject @Inject private(
  folderSubsystem: FolderSubsystem,
  jobSubsystem: JobSubsystem,
  lockSubsystem: LockSubsystem,
//monitorSubsystem: MonitorSubsystem,
  orderSubsystem: OrderSubsystem,
  processClassSubsystem: ProcessClassSubsystem,
  scheduleSubsystem: ScheduleSubsystem,
  standingOrderSubsystem: StandingOrderSubsystem) {

  def pathToFileBased(path: TypedPath): FileBased =
    path match {
      case path: FolderPath ⇒ folderSubsystem.fileBased(path)
      case path: JobChainPath ⇒ orderSubsystem.fileBased(path)
      case path: JobPath ⇒ jobSubsystem.fileBased(path)
      case path: LockPath ⇒ lockSubsystem.fileBased(path)
      case path: MonitorPath ⇒ throw new UnsupportedOperationException("No MonitorSubsystem")  // monitorSubsystem.fileBased(path)
      case path: OrderKey ⇒ standingOrderSubsystem.fileBased(path)
      case path: ProcessClassPath ⇒ processClassSubsystem.fileBased(path)
      case path: SchedulePath ⇒ scheduleSubsystem.fileBased(path)
    }
}
