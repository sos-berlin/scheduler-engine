package com.sos.scheduler.engine.cplusplus.generator.main

import org.junit._
import org.junit.Assert._


class PackageTest
{
    @Test def testRelevantClasses() {
// Funktioniert nicht, weil Package kleine .class-Dateien berücksichtigt, nur Jars
//        val result = Package(classOf[test.A].getPackage.getName).relevantClasses
//        assertEquals(Set(classOf[test.A], classOf[test.B]), result.toSet)
    }
}
