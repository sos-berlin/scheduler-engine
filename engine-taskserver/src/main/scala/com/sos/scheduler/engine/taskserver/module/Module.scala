package com.sos.scheduler.engine.taskserver.module

import com.sos.scheduler.engine.jobapi.dotnet.DotnetModuleReference
import com.sos.scheduler.engine.taskserver.module.dotnet.DotnetModule
import com.sos.scheduler.engine.taskserver.module.javamodule.{JavaScriptModule, StandardJavaModule}
import com.sos.scheduler.engine.taskserver.module.shell.ShellModule

/**
 * @author Joacim Zschimmer
 */
trait Module {
  def moduleLanguage: ModuleLanguage
}

object Module {
  def apply(moduleLanguage: ModuleLanguage, script: Script, javaClassNameOption: Option[String]): Module =
    moduleLanguage match {
      case ShellModuleLanguage ⇒
        for (name ← javaClassNameOption) throw new IllegalArgumentException(s"Language '$moduleLanguage' conflicts with parameter javaClass='$name'")
        new ShellModule(script)

      case JavaModuleLanguage ⇒
        val className = javaClassNameOption getOrElse { throw new NoSuchElementException(s"Language '$moduleLanguage' requires a class name") }
        StandardJavaModule(className)

      case JavaScriptModuleLanguage(language) ⇒
        new JavaScriptModule(language, script)

      case PowershellModuleLanguage ⇒
        new DotnetModule(moduleLanguage, DotnetModuleReference.Powershell(script = script.string))

      case DotnetClassModuleLanguage ⇒
        new DotnetModule(moduleLanguage, DotnetModuleReference.DotnetClass(dll = ???, className = ???))

      case _ ⇒ throw new IllegalArgumentException(s"Unsupported language $moduleLanguage")
    }
}
