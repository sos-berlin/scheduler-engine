package com.sos.scheduler.engine.cplusplus.generator.util

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy
import org.junit._
import org.junit.Assert._
import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.Matchers._

class PackageOpsTest {
  import PackageOps._

  @Test def testClassNamesOfPackage() {
    // Funktioniert nur mit Klassen aus einem Jar, nicht mit Klassen im Dateisystem. Deshalb nehmen wir scala-library.jar zum Test.
    // Beim Aufruf mit eingenem Paket muss das mit mvn install zu einem Jar gemacht werden.
    val clas = classOf[CppProxy]
    val classNames = classNamesOfPackage("scala")
    assert(classNames.nonEmpty)
    assert(classNames contains classOf[scala.Option[_]].getName)
  }
}
