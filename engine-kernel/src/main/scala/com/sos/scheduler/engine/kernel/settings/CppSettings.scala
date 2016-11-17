package com.sos.scheduler.engine.kernel.settings

import com.sos.scheduler.engine.kernel.cppproxy.SettingsC
import com.sos.scheduler.engine.kernel.settings.CppSettingName._

/**
 * C++ settings (settings.cxx), for write-access only.
 * For reading, see [[com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration]].
 */
final class CppSettings private(val valueMap: Map[CppSettingName, String]) {

  def setSettingsInCpp(cppProxy: SettingsC): Unit = {
    for (e ← valueMap)
      cppProxy.set(e._1.number, e._2)
  }
}

object CppSettings {
  lazy val Empty = apply(Nil)
  lazy val TestMap = Map(alwaysCreateDatabaseTables -> true.toString)

  def apply(values: Iterable[(CppSettingName, String)]): CppSettings =
    new CppSettings(values.toMap withDefault { o ⇒ throw new NoSuchElementException(s"Unknown CppSettingName $o") })
}
