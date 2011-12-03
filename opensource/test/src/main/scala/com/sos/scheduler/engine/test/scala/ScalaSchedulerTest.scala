package com.sos.scheduler.engine.test.scala

import com.sos.scheduler.engine.test.SchedulerTest

abstract class ScalaSchedulerTest extends SchedulerTest {
    def shortTimeout = SchedulerTest.shortTimeout       // Zur komfortableren Benutzung
}
