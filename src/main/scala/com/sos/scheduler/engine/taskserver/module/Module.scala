package com.sos.scheduler.engine.taskserver.module

import com.sos.scheduler.engine.taskserver.module.java.JavaModule
import com.sos.scheduler.engine.taskserver.module.shell.ShellModule

/**
 * @author Joacim Zschimmer
 */
trait Module {
  def moduleLanguage: ModuleLanguage
}

object Module {
  def apply(moduleLanguage: ModuleLanguage, script: Script, javaClassOption: Option[String]) =
    moduleLanguage match {
      case ShellModuleLanguage ⇒
        javaClassOption map { o ⇒ throw new IllegalArgumentException(s"language '$moduleLanguage' forbids parameter javaClass=$o") }
        new ShellModule(script)
      case JavaModuleLanguage ⇒ JavaModule(className = javaClassOption getOrElse { throw new NoSuchElementException(s"Language '$moduleLanguage' requires a class name")})
      case _ ⇒ throw new IllegalArgumentException(s"Unsupported language $moduleLanguage")
    }
}
