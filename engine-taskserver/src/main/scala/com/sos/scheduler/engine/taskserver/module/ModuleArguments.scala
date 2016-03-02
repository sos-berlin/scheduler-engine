package com.sos.scheduler.engine.taskserver.module

import java.nio.file.Path

/**
  * @author Joacim Zschimmer
  */
final case class ModuleArguments(
  language: ModuleLanguage,
  javaClassNameOption: Option[String],
  dllOption: Option[Path],
  dotnetClassNameOption: Option[String],
  script: Script)
