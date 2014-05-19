package com.sos.scheduler.engine.kernel.lock

import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{LockC, Lock_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import javax.inject.{Inject, Singleton}

@Singleton
final class LockSubsystem @Inject private(
  protected[this] val cppProxy: Lock_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystem {

  type ThisSubsystem = LockSubsystem
  type ThisFile_basedC = LockC
  type ThisFileBased = Lock

  val description = LockSubsystem
}

object LockSubsystem extends FileBasedSubsystem.AbstractDesription[LockSubsystem, LockPath, Lock] {
  val fileBasedType = FileBasedType.lock
  val stringToPath = LockPath.apply _
}
