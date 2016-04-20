package com.sos.scheduler.engine.taskserver.module

import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import com.sos.scheduler.engine.jobapi.dotnet.api.DotnetModuleReference
import java.nio.file.Path
import org.jetbrains.annotations.TestOnly
import scala.reflect.ClassTag

/**
  * @author Joacim Zschimmer
  */
sealed trait ModuleArguments {
  def language: ModuleLanguage
}

object ModuleArguments {
  def apply(
    language: ModuleLanguage,
    javaClassNameOption: Option[String],
    dllOption: Option[Path],
    dotnetClassNameOption: Option[String],
    script: Script,
    dllDirectory: Option[Path]): ModuleArguments =
  {
    def requireUnused[A](name: String, option: Option[A]) =
      for (o ← option) throw new IllegalArgumentException(s"language='$language' conflicts with parameter $name='$o'")

    language match {
      case ShellModuleLanguage ⇒
        requireUnused("java_class", javaClassNameOption)
        requireUnused("dll", dllOption)
        requireUnused("dotnet_class", dotnetClassNameOption)
        ShellModuleArguments(script)

      case JavaModuleLanguage ⇒
        requireUnused("dll", dllOption)
        requireUnused("dotnet_class", dotnetClassNameOption)
        JavaModuleArguments(
          className = javaClassNameOption getOrElse { throw new IllegalArgumentException(s"language='$language' requires a class name") } )

      case JavaScriptModuleLanguage(scriptLanguage) ⇒
        requireUnused("java_class", javaClassNameOption)
        requireUnused("dll", dllOption)
        requireUnused("dotnet_class", dotnetClassNameOption)
        JavaScriptModuleArguments(scriptLanguage, script)

      case lang @ PowershellModuleLanguage ⇒
        requireUnused("java_class", javaClassNameOption)
        requireUnused("dll", dllOption)
        requireUnused("dotnet_class", dotnetClassNameOption)
        DotnetModuleArguments(lang, DotnetModuleReference.Powershell(script = script.string))

      case lang @ DotnetClassModuleLanguage ⇒
        requireUnused("java_class", javaClassNameOption)
        val dll = dllOption getOrElse { throw new IllegalArgumentException(s"language='$language' requires a Java class name") }
        val className = dotnetClassNameOption getOrElse { throw new IllegalArgumentException(s"language='$language' requires a .Net class name") }
        DotnetModuleArguments(lang, DotnetModuleReference.DotnetClass(
          dll = dllDirectory map { _ resolve dll } getOrElse dll,
          className = className))
    }
  }

  final case class ShellModuleArguments(script: Script)
  extends ModuleArguments {
    def language = ShellModuleLanguage
  }

  case class JavaModuleArguments(className: String)
  extends ModuleArguments {
    def language = JavaModuleLanguage
  }

  @TestOnly
  final class TestJavaModuleArguments[A: ClassTag](val newModule: () ⇒ A)
  extends JavaModuleArguments(implicitClass[A].getName)

  final case class JavaScriptModuleArguments(scriptLanguage: String, script: Script)
  extends ModuleArguments {
    def language = JavaModuleLanguage
  }

  final case class DotnetModuleArguments(val language: ModuleLanguage, dotnetModuleReference: DotnetModuleReference)
  extends ModuleArguments
}
