package com.sos.scheduler.engine.tests.jira.js802

import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.job.{TaskEnded, TaskKey}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderStarted}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js802.JS802IT._
import org.joda.time.DateTime
import org.joda.time.format.DateTimeFormat
import org.junit.Assert._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

/** JS-802 "http://www.sos-berlin.com/jira/browse/JS-802": Testet einen Auftrag und einen Job.
  * @see <a href="http://www.sos-berlin.com/jira/browse/JS-802">JS-802</a> */
@RunWith(classOf[JUnitRunner])
final class JS802IT extends FreeSpec with ScalaSchedulerTest {

  @volatile private var startTime = new DateTime(0)

  private object collector {
    val set = mutable.HashSet[Any]()

    def add(o: Any): Unit = {
      assertFalse("Duplicate event " + o, set contains o)
      set.add(o)
      if (set == Set(orderKey, jobName))
        controller.terminateScheduler()
    }
  }

  "test" in {
    startTime = secondNow() plusSeconds orderDelay
    scheduler.executeXml(orderElem(orderKey, startTime))
    scheduler.executeXml(jobElem(jobName, startTime))
    controller.waitForTermination()
  }

  eventBus.on[OrderStarted.type] {
    case KeyedEvent(key, OrderStarted) ⇒
      assertTrue(s"Order $key has been started before expected time $startTime", new DateTime() isAfter startTime)
      collector.add(key)
  }

  eventBus.on[TaskEnded] {
    case KeyedEvent(TaskKey(jobPath, _), _: TaskEnded) if jobPath.name == jobName ⇒
      assertTrue(s"$jobPath has been started before expected time $startTime", new DateTime() isAfter startTime)
      collector.add(jobPath.name)
  }
}

object JS802IT {
  private val orderDelay = 3 + 1
  private val orderKey = OrderKey("/a", "atOrder")
  private val jobName = "job"
  private val yyyymmddhhmmssFormatter = DateTimeFormat.forPattern("yyyy-MM-dd HH:mm:ss")

  private def secondNow() = {
    val now = new DateTime()
    now minusMillis now.millisOfSecond.get
  }

  private def orderElem(orderKey: OrderKey, startTime: DateTime) =
    <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}>
      <run_time>
        <at at={yyyymmddhhmmssFormatter.print(startTime)}/>
      </run_time>
    </order>

  private def jobElem(jobName: String, startTime: DateTime) =
    <job name={jobName}>
      <script java_class={classOf[com.sos.scheduler.engine.test.jobs.SingleStepJob].getName}/>
      <run_time>
        <at at={yyyymmddhhmmssFormatter.print(startTime)}/>
      </run_time>
    </job>
}
