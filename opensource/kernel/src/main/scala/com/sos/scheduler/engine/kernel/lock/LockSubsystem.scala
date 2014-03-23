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
  implicit protected[this] val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystem {

  type MySubsystem = LockSubsystem
  type MyFile_basedC = LockC
  type MyFileBased = Lock

  val companion = LockSubsystem
}

object LockSubsystem extends FileBasedSubsystem.Companion[LockSubsystem, LockPath, Lock](FileBasedType.lock, LockPath.apply)
