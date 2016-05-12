package com.sos.scheduler.engine.taskserver.dotnet.scriptengine

import com.sos.scheduler.engine.taskserver.moduleapi.ModuleLanguage
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class ScriptLanguageTest extends FreeSpec {

  "vbscript" in {
    val ScriptLanguage(lang) = ModuleLanguage("vbscript")
    assert(lang == ScriptLanguage("vbscript"))
  }

  "jscript" in {
    val ScriptLanguage(lang) = ModuleLanguage("jscript")
    assert(lang == ScriptLanguage("jscript"))
  }

  "other" in {
    intercept[MatchError] {
      val ScriptLanguage(_) = ModuleLanguage("other")
    }
  }
}
