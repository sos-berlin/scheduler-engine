package com.sos.scheduler.engine.tests.scheduler.comapi.job.start_task

import sos.spooler.Job_impl

class TestBJob extends Job_impl {

   override def spooler_process() = {
     val test = spooler_task.params().value("TEST")
     spooler.set_var("test-b", test)
     false
   }
 }
