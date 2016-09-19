package com.sos.scheduler.engine.kernel.lock

import com.google.inject.Injector
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{LockC, Lock_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import javax.inject.{Inject, Singleton}

@Singleton
private[kernel] final class LockSubsystem @Inject private(
  protected[this] val cppProxy: Lock_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val injector: Injector)
extends FileBasedSubsystem {

  type ThisSubsystemClient = LockSubsystemClient
  type ThisSubsystem = LockSubsystem
  type ThisFile_basedC = LockC
  type ThisFileBased = Lock

  val companion = LockSubsystem
}

object LockSubsystem
extends FileBasedSubsystem.AbstractCompanion[LockSubsystemClient, LockSubsystem, LockPath, Lock] {

  val fileBasedType = FileBasedType.Lock
  val stringToPath = LockPath.apply _
}
