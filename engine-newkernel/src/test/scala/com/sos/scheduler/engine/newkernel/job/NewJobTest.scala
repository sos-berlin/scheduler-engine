package com.sos.scheduler.engine.newkernel.job

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.data.event.AnyKeyedEvent
import com.sos.scheduler.engine.common.async.{CallRunner, StandardCallQueue}
import com.sos.scheduler.engine.data.job.{JobPath, TaskEnded, TaskStarted}
import com.sos.scheduler.engine.eventbus.{EventHandler, EventHandlerAnnotated, SchedulerEventBus}
import com.sos.scheduler.engine.newkernel.job.NewJobTest._
import com.sos.scheduler.engine.newkernel.utils.Service.withService
import com.sos.scheduler.engine.newkernel.utils.ThreadService
import org.joda.time.DateTimeZone
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class NewJobTest extends FunSuite with EventHandlerAnnotated {
  val callQueue = new StandardCallQueue
  val callRunner = new CallRunner(callQueue)

  ignore("xx") {
    val xml =
      <new_job>
        <run_time>
          <period begin="12:42" end="24:00" repeat="5"/>
        </run_time>
        <script language="shell">
          ping -n 3 127.0.0.1
          exit 0
        </script>
      </new_job>
      .toString()
    val eventBus = new SchedulerEventBus
    eventBus.registerAnnotated(this)
    withService(new ThreadService(eventBus)) {
      val jobConfiguration = JobConfigurationXMLParser.parseString(xml, DateTimeZone.getDefault)
      val job = new NewJob(jobPath, jobConfiguration, eventBus, callQueue)
      job.activate()
      callRunner.run()
    }
  }

  @EventHandler def handle(keyedEvent: AnyKeyedEvent): Unit = {
    keyedEvent.event match {
      case TaskStarted ⇒
        logger info keyedEvent.toString
      case event: TaskEnded ⇒
        logger info keyedEvent.toString
        callRunner.end()
    }
  }
}

private object NewJobTest {
  private val logger = Logger(getClass)
  private val jobPath = JobPath("/a")
}
