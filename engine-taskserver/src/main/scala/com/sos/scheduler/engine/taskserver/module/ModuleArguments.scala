package com.sos.scheduler.engine.taskserver.module

/**
  * @author Joacim Zschimmer
  */
final case class ModuleArguments(
  language: ModuleLanguage,
  javaClassNameOption: Option[String],
  script: Script)
