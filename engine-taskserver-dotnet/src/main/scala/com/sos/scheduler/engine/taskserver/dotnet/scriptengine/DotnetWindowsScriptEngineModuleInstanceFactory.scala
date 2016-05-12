//package com.sos.scheduler.engine.taskserver.dotnet.scriptengine
//import com.sos.scheduler.engine.taskserver.moduleapi.NamedIDispatches
//
///**
//  * @author Joacim Zschimmer
//  */
//final class DotnetWindowsScriptEngineModuleInstanceFactory extends WindowsScriptEngineModuleInstanceFactory {
//
//  /**
//    * @tparam A [[sos.spooler.Job_impl]] or a [[sos.spooler.Monitor_impl]]
//    */
//  def newInstance[A](clazz: Class[A], namedIDispatches: NamedIDispatches, script: EngineScript): A = {
//    /*
//      IActiveScriptParse ..
//      C#-Adapter für die sos.spooler-Objekte:
//        Implementiert IDispatch mit GetIDsForNames und Invoke
//        Für jede Verwendung erzeugen wir einen neuen Adapter. Das heißt, zwei aufs gleiche Objekte zeigende Adapter müssen nicht gleich sein.
//
//
//     */
//    ???
//    // clazz ist egal?
////    if (clazz == classOf[sos.spooler.Job_impl]) {
////    }
////    else
////    if (clazz == classOf[sos.spooler.Monitor_impl]) {
////      ???
////    }
////    else
////      throw new IllegalArgumentException
//  }
//
//  def close(): Unit = ???
//
//}
