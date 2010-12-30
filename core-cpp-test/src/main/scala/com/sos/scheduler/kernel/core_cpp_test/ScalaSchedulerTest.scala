package com.sos.scheduler.kernel.core_cpp_test

import org.junit._


abstract class ScalaSchedulerTest extends SchedulerTest
{
    def shortTimeout = SchedulerTest.shortTimeout       // Zur komfortableren Benutzung
}
