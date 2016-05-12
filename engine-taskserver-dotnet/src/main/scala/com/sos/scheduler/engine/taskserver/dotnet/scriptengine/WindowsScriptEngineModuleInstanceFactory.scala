package com.sos.scheduler.engine.taskserver.dotnet.scriptengine

import com.sos.scheduler.engine.minicom.idispatch.IDispatch.implicits.RichIDispatch
import com.sos.scheduler.engine.minicom.idispatch.{IDispatch, InvocableIDispatch, PublicMethodsInvocable}
import com.sos.scheduler.engine.taskserver.dotnet.scriptengine.WindowsScriptEngineModuleInstanceFactory._
import com.sos.scheduler.engine.taskserver.moduleapi.NamedIDispatches

/**
  * @author Joacim Zschimmer
  */
trait WindowsScriptEngineModuleInstanceFactory extends  {

  /**
    * @tparam A [[sos.spooler.Job_impl]] or a [[sos.spooler.Monitor_impl]]
    */
  @throws[Exception]
  def newInstance[A](clazz: Class[A], namedIDispatches: NamedIDispatches, script: EngineScript): A =
}

object WindowsScriptEngineModuleInstanceFactory {

}
