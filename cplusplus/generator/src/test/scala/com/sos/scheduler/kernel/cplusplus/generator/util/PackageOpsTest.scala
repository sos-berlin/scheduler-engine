package com.sos.scheduler.kernel.cplusplus.generator.util

import com.sos.scheduler.kernel.cplusplus.runtime.CppProxy
import org.junit._
import org.junit.Assert._
import PackageOps._


class PackageOpsTest
{
    @Test def testClassNamesOfPackage {
        // Funktioniert nur mit Klassen aus einem Jar, nicht mit Klassen im Dateisystem. Deshalb nehmen wir CppProxy zum Test.
        val clas = classOf[CppProxy]
        val classNames = classNamesOfPackage(clas.getPackage.getName)
        assert(classNames.nonEmpty)
        assert(classNames contains clas.getName)
    }
}
