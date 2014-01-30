package com.sos.scheduler.engine.test.util

import java.lang.Thread.currentThread

object TestUtils {
  private val testClassNameEndings = Set("Test", "IT")

  def currentTestClass: Class[_] = {
    def currentTestClassName: String =
      currentThread.getStackTrace.reverseIterator map { _.getClassName } map normalizeClassName find isTestClassName getOrElse
          sys.error(s"No current test class found (no class in stack which name ends $testClassNameEndings)")

    def normalizeClassName(name: String) =
      name.replaceAll("""\$\$.*$""", "")   // $$ ist der Trenner für von Scala generierte Klassen

    def isTestClassName(o: String) =
      testClassNameEndings exists o.endsWith

    Class forName currentTestClassName
  }
}
