package com.sos.scheduler.engine.kernel.processclass

import com.google.inject.Injector
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{Process_classC, Process_class_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import javax.inject.{Inject, Singleton}

@Singleton
private[kernel] final class ProcessClassSubsystem @Inject private(
  protected[this] val cppProxy: Process_class_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val injector: Injector)
extends FileBasedSubsystem{

  type ThisSubsystemClient = ProcessClassSubsystemClient
  type ThisSubsystem = ProcessClassSubsystem
  type ThisFileBased = ProcessClass
  type ThisFile_basedC = Process_classC
  type Path = ProcessClassPath

  val companion = ProcessClassSubsystem

  private[kernel] def processClass(path: ProcessClassPath) = fileBased(path)
}

object ProcessClassSubsystem
extends FileBasedSubsystem.AbstractCompanion[ProcessClassSubsystemClient, ProcessClassSubsystem, ProcessClassPath, ProcessClass] {

  val fileBasedType = FileBasedType.ProcessClass
  val stringToPath = ProcessClassPath.apply _
}
