package com.sos.scheduler.engine.taskserver.dotnet.scriptengine

import com.sos.scheduler.engine.base.generic.IsString
import com.sos.scheduler.engine.taskserver.moduleapi.ModuleLanguage

/**
  * @author Joacim Zschimmer
  */
final case class ScriptLanguage(string: String) extends IsString

object ScriptLanguage {
  private val Prefix = "windowscriptengine:"
  private val KnownLanguages = Set("vbscript", "jscript")

  def unapply(lang: ModuleLanguage) =
    if (lang.string startsWith Prefix)
      Some(ScriptLanguage(lang.string stripPrefix Prefix))
    else if (KnownLanguages(lang.string))
      Some(ScriptLanguage(lang.string))
    else
      None
}
