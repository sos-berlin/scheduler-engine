package com.sos.scheduler.engine.tests.jira.js994

import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.junit.Ignore

@RunWith(classOf[JUnitRunner])
@Ignore("Ein schedule mit substitute seiner selbst lässt C++-Code mit Stapelüberlauf abbrechen")
final class JS994 extends FunSuite with ScalaSchedulerTest {
  test("Ein schedule mit substitute seiner selbst lässt C++-Code mit Stapelüberlauf abbrechen") {}
}
