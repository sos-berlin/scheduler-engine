package com.sos.scheduler.engine.taskserver.module

import com.sos.scheduler.engine.taskserver.module.ModuleArguments._
import com.sos.scheduler.engine.taskserver.module.ModuleFactory._
import com.sos.scheduler.engine.taskserver.module.dotnet.DotnetModuleFactory
import com.sos.scheduler.engine.taskserver.module.javamodule.{JavaClassModule, JavaScriptModule, StandardJavaModule}
import com.sos.scheduler.engine.taskserver.module.shell.ShellModule
import javax.inject.{Inject, Singleton}
import org.jetbrains.annotations.TestOnly

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class ModuleFactory @Inject private(newDotnetModule: DotnetModuleFactory) {

  def apply(moduleArguments: ModuleArguments): Module = {
    moduleArguments match {
      case a: ShellModuleArguments ⇒ new ShellModule(a)
      case a: TestJavaModuleArguments[_] ⇒ new TestJavaModule(a)
      case a: JavaModuleArguments ⇒ new StandardJavaModule(a)
      case a: JavaScriptModuleArguments ⇒ new JavaScriptModule(a)
      case a: DotnetModuleArguments ⇒ newDotnetModule(a)
      case _ ⇒ throw new IllegalArgumentException(s"Unsupported language $language")
    }
  }
}

object ModuleFactory {
  @TestOnly
  val PureJavaOnly = new ModuleFactory(DotnetModuleFactory.Unsupported)

  private class TestJavaModule[A](val arguments: TestJavaModuleArguments[A]) extends JavaClassModule {
    def newInstance() = arguments.newModule()
  }
}
