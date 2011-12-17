package com.sos.scheduler.engine.test.scala

import com.sos.scheduler.engine.test.SchedulerTest
import org.scalatest.{FunSuite, BeforeAndAfterAll}
import org.scalatest.junit.JUnitRunner
import org.junit.runner.RunWith

@RunWith(classOf[JUnitRunner])
abstract class ScalaSchedulerTest extends SchedulerTest with FunSuite with BeforeAndAfterAll {
    def shortTimeout = SchedulerTest.shortTimeout       // Zur komfortableren Benutzung

    override def afterAll(configMap: Map[String, Any]) {
        try controller.close()
        finally super.afterAll(configMap)
    }
}
