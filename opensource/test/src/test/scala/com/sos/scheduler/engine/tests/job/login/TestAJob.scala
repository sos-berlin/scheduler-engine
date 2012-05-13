package com.sos.scheduler.engine.tests.job.login

import com.sos.scheduler.engine.tests.job.login.JobLoginTest.propertyNames
import sos.spooler.Job_impl

class TestAJob extends Job_impl {
  override def spooler_process() = {
    val properties = propertyNames map { name => name -> System.getProperty(name) }
    val v = spooler.variables
    val jobName = spooler_job.name
    val schedulerVariables = properties map { case (name, value) => jobName +"."+ name -> value }
    schedulerVariables foreach { case (name, value) => v.set_value(name, value) }
    false
  }
}
