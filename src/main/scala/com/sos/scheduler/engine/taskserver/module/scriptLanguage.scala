package com.sos.scheduler.engine.taskserver.module

import com.sos.scheduler.engine.data.base.IsString

/**
 * @author Joacim Zschimmer
 */
trait ScriptLanguage extends IsString

object ScriptLanguage {
  def apply(language: String): ScriptLanguage =
    language.toLowerCase match {
      case ShellScriptLanguage.string ⇒ ShellScriptLanguage
      case _ ⇒ OtherScriptLanguage(language)
    }
}

case object ShellScriptLanguage extends ScriptLanguage {
  val string = "shell"
}

//final case class JavaScriptLanguage(javaName: String) extends ScriptLanguage {
//  def string = s"java:$javaName"
//}

final case class OtherScriptLanguage(string: String) extends ScriptLanguage
