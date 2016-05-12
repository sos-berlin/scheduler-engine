package com.sos.scheduler.engine.taskserver.modules.windowsscriptengine

import com.sos.scheduler.engine.minicom.idispatch.IDispatch.implicits.RichIDispatch
import com.sos.scheduler.engine.minicom.idispatch.{IDispatch, InvocableIDispatch, PublicMethodsInvocable}
import com.sos.scheduler.engine.taskserver.data.module.IDispatchModule
import com.sos.scheduler.engine.taskserver.dotnet.scriptengine.{EngineScript, ScriptLanguage, WindowsScriptEngineModuleInstanceFactory}
import com.sos.scheduler.engine.taskserver.moduleapi.{ModuleArguments, ModuleFactory, NamedIDispatches, RawModuleArguments}
import com.sos.scheduler.engine.taskserver.modules.javamodule.ApiModule
import com.sos.scheduler.engine.taskserver.modules.windowsscriptengine.WindowsScriptEngineModule.MockScriptEngine
import com.sos.scheduler.engine.taskserver.spoolerapi.TypedNamedIDispatches
import java.nio.file.Path
import WindowsScriptEngineModule._

/**
  * @author Joacim Zschimmer
  */
final class WindowsScriptEngineModule private[windowsscriptengine](val arguments: WindowsScriptEngineModule.Arguments)
extends IDispatchModule {

  def newInstance(namedIDispatches: TypedNamedIDispatches, arguments: Arguments) =
    new MockScriptEngine(namedIDispatches, arguments.script)
}

object WindowsScriptEngineModule {

  final class Factory(factory: WindowsScriptEngineModuleInstanceFactory, classDllDirectory: Option[Path]) extends ModuleFactory {
    def toModuleArguments: PartialFunction[RawModuleArguments, ModuleArguments] = {
      case RawModuleArguments(ScriptLanguage(lang), None, script, None, None) ⇒
        Arguments(this, EngineScript(lang, script = script.string))
    }

    def newModule(arguments: ModuleArguments) =
      new WindowsScriptEngineModule(arguments.asInstanceOf[Arguments], factory)

    override def toString = s"WindowsScriptEngineModule.Factory($factory)"
  }

  final case class Arguments(moduleFactory: ModuleFactory, script: EngineScript) extends ModuleArguments

  private class MockScriptEngine(namedIDispatches: NamedIDispatches, script: EngineScript) {
    import namedIDispatches.spoolerTask

    val iDispatch: IDispatch = new InvocableIDispatch with PublicMethodsInvocable {
      def spooler_process() = {
        val orderId = spoolerTask.invokeGet("order").asInstanceOf[IDispatch].invokeGet("id").asInstanceOf[String]
        assert(orderId == "TEST-ORDER-ID")
        true
      }

      def spooler_close() = ()
    }
  }
}
