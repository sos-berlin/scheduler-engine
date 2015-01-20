package com.sos.scheduler.engine.cplusplus.generator.util

import org.junit._
import com.sos.scheduler.engine.cplusplus.generator.util.PackageOps._

class PackageOpsTest {

  @Test def testClassNamesOfPackage(): Unit = {
    // Funktioniert nur mit Klassen aus einem Jar, nicht mit Klassen im Dateisystem. Deshalb nehmen wir scala-library.jar zum Test.
    // Beim Aufruf mit eingenem Paket muss das mit mvn install zu einem Jar gemacht werden.
    val classNames = classNamesOfPackage("scala")
    assert(classNames.nonEmpty)
    assert(classNames contains classOf[scala.Option[_]].getName)
  }
}
