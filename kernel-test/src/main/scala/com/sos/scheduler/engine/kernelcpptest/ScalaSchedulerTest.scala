package com.sos.scheduler.engine.kernelcpptest

import org.junit._
import com.sos.scheduler.engine.kernel.test.SchedulerTest


abstract class ScalaSchedulerTest extends SchedulerTest
{
    def shortTimeout = SchedulerTest.shortTimeout       // Zur komfortableren Benutzung
}
