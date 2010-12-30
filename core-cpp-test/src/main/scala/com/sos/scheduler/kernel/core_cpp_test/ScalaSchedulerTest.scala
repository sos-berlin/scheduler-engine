package com.sos.scheduler.engine.kernelcpptest

import org.junit._


abstract class ScalaSchedulerTest extends SchedulerTest
{
    def shortTimeout = SchedulerTest.shortTimeout       // Zur komfortableren Benutzung
}
