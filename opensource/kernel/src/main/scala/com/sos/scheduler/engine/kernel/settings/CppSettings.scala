package com.sos.scheduler.engine.kernel.settings

import CppSettingName._
import com.sos.scheduler.engine.kernel.cppproxy.SettingsC

final class CppSettings private(val valueMap: Map[CppSettingName, String]) {

  def setSettingsInCpp(cppProxy: SettingsC) {
    for (e <- valueMap)
      cppProxy.set(e._1.getNumber, e._2)
  }
}

object CppSettings {
  lazy val empty = apply(Nil)
  lazy val testMap = Map(alwaysCreateDatabaseTables -> true.toString)

  def apply(values: Iterable[(CppSettingName, String)]) =
    new CppSettings(values.toMap)
}
