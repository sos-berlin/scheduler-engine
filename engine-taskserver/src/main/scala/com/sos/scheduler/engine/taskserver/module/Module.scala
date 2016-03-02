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
  def apply(moduleArguments: ModuleArguments): Module = {
    import moduleArguments._
    moduleArguments.language match {
      case ShellModuleLanguage ⇒
        for (name ← javaClassNameOption) throw new IllegalArgumentException(s"Language '$language' conflicts with parameter javaClass='$name'")
        new ShellModule(script)

      case JavaModuleLanguage ⇒
        val className = javaClassNameOption getOrElse { throw new NoSuchElementException(s"Language '$language' requires a class name") }
        StandardJavaModule(className)

      case JavaScriptModuleLanguage(lang) ⇒
        new JavaScriptModule(lang, script)

      case PowershellModuleLanguage ⇒
        new DotnetModule(language, DotnetModuleReference.Powershell(script = script.string))

      case DotnetClassModuleLanguage ⇒
        new DotnetModule(language, DotnetModuleReference.DotnetClass(
          dll = dllOption getOrElse { throw new IllegalArgumentException("Missing key for DLL") },
          className = dotnetClassNameOption getOrElse { throw new IllegalArgumentException("Missing key dotnet_class_name") }))

      case _ ⇒ throw new IllegalArgumentException(s"Unsupported language $language")
    }
  }
}
