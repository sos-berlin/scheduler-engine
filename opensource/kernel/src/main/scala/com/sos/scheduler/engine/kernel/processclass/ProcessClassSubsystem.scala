package com.sos.scheduler.engine.kernel.processclass

import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.cppproxy.{Process_classC, Process_class_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import javax.inject.{Inject, Singleton}

@Singleton
final class ProcessClassSubsystem @Inject private(
  protected[this] val cppProxy: Process_class_subsystemC)
extends FileBasedSubsystem{
  type MySubsystem = ProcessClassSubsystem
  type MyFileBased = ProcessClass
  type MyFile_basedC = Process_classC

  val companion = ProcessClassSubsystem
}

object ProcessClassSubsystem extends FileBasedSubsystem.Companion[ProcessClassSubsystem, ProcessClassPath, ProcessClass](FileBasedType.processClass, ProcessClassPath.apply)
