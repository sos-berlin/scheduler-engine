package com.sos.scheduler.engine.newkernel.job

import com.google.inject.Injector
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.job.{JobPath, TaskPersistentState}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{Job_nodeC, SpoolerC, Variable_setC}
import com.sos.scheduler.engine.kernel.job.{Job, JobStateCommand}
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.newkernel.job.commands.StopJobCommand
import java.io.ByteArrayInputStream
import javax.xml.transform.stream.StreamSource
import org.joda.time.DateTimeZone
import org.w3c.dom.{Document, Element}
import scala.sys.error

@ForCpp
final class CppNewJob(/*cppProxy: JavaJobC,*/ injector: Injector, jobSister: Job, timeZone: DateTimeZone)
extends Sister {

  @ForCpp def this(spoolerC: SpoolerC, jobSister: Job) =
    this(spoolerC.getSister.injector, jobSister, CppNewJob.timeZone(spoolerC.time_zone_name))

  private var jobOption: Option[NewJob] = None
  private var configuration: JobConfiguration = null
  private var stateText = ""

  private def jobPath = JobPath("/CppNewJob-OUT-OF-ORDER")//jobSister.path
  //var stateText = ""

  def onCppProxyInvalidated(): Unit = {}

  @ForCpp def close(): Unit = {
    for (j <- jobOption) j.close()
  }

  @ForCpp def setXmlBytes(bytes: Array[Byte]): Unit = {
    configuration = ScalaXMLEventReader.parseDocument(new StreamSource(new ByteArrayInputStream(bytes))) { reader ⇒
      JobConfigurationXMLParser.parseWithReader(timeZone)(reader)
    }
  }

  @ForCpp def onInitialize(): Boolean = {
    jobOption = Some(new NewJob(
      jobPath,
      configuration,
      injector.getInstance(classOf[SchedulerEventBus]),
      injector.getInstance(classOf[SchedulerThreadCallQueue])))    //TODO Haben wir eine eigene CallQueue mit eigenem CallRunner?
    true
  }

  @ForCpp def onLoad(): Boolean = {
    true
  }

  @ForCpp def onActivate(): Boolean = {
    job.activate()
    true
  }

  @ForCpp def onPrepareToRemove(): Unit = {
  }

  @ForCpp def canBeRemovedNow: Boolean =
    jobOption exists { _.canBeRemovedNow }

  @ForCpp def onRemoveNow(): Unit = {}

  @ForCpp def removalError: String =
    "removelError not implemented"


  // STATE

  @ForCpp def title =
    configuration.title

  @ForCpp def description: String =
    ???  //job.configuration.description

  @ForCpp def setStateText(o: String): Unit = {
    stateText = o
  }

  @ForCpp def executeStateCommand(command: String) = JobStateCommand.valueOf(command) match {
    //case JobStateCommand.start => job executeCommand StartJobCommand
    case JobStateCommand.stop => for (j <- jobOption) j executeCommand StopJobCommand
    case JobStateCommand.unstop => ???
    case JobStateCommand.start => ???
    case JobStateCommand.wake => ???
    case JobStateCommand.endTasks => ???
    case JobStateCommand.suspendTasks => ???
    case JobStateCommand.continueTasks => ???
    case JobStateCommand.remove => ???
    case JobStateCommand.enable => ???
    case JobStateCommand.disable => ???
    //case JobStateCommand.wakeWhenInPeriod => ???
    case _ => ???
  }

  @ForCpp def stop(): Unit = {
    for (j <- jobOption) j executeCommand StopJobCommand
  }

  @ForCpp def stopSimply(): Unit = {
    for (j <- jobOption) j executeCommand StopJobCommand
  }

  @ForCpp def isPermanentlyStopped: Boolean =
    ???

  @ForCpp def stateString: String = ???
  //job.state map ...

  @ForCpp def domElement(d: Document, /*w: ShowWhat,*/ jobChain: JobChain): Element =
    d.createElement("job")

  @ForCpp def readHistory(d: Document, taskId: Int, n: Int/*, w: ShowWhat*/): Element =
    ???


  // PROCESS CLASS

  @ForCpp def processClassPath: String =
    configuration.processClassPath.string

  @ForCpp def isWaitingForProcess: Boolean = {
    ???
  }

  @ForCpp def onProcessIsIdle(): Unit = {
    ???
  }


  // SCHEDULE

  @ForCpp def setScheduleDOM(o: Element): Unit = {
    ???
  }

  @ForCpp def onReferencedScheduleLoaded(): Unit = {
    ???
  }

  @ForCpp def onReferencedScheduleModified(): Unit = {
    ???
  }

  @ForCpp def onReferencedScheduleToBeRemoved(): Boolean = {
    ???
  }

  @ForCpp def appendCalendarDOMElements(d: Document/*, o: ShowCalendarOptions*/): Element =
    ???


  // TASKS

  @ForCpp def startTask(parameters: Variable_setC, environment: Variable_setC, atMillis: Long, force: Boolean, taskName: String, webServiceName: String): TaskId = {
    job.startTask()
  }

  @ForCpp def enqueueTask(o: TaskPersistentState): Unit = {
    ???
  }

  @ForCpp def removeRunningTask(taskId: Int): Unit = {
    ???
  }

  @ForCpp def tryToEndATask(): Boolean =
    jobOption exists { _.tryToEndATask() }

  @ForCpp def killTask(id: Int, immediately: Boolean) = {
    val task = job.task(TaskId(id))
    if (immediately)
      task.kill()
    else
      task.end()
  }

  @ForCpp def hasTask: Boolean = jobOption exists { _.hasTask }


  // ORDER

  @ForCpp def connectJobNode(o: Job_nodeC): Boolean = {
    ???
  }

  @ForCpp def disconnectJobNode(o: Job_nodeC): Unit = {
    ???
  }

  @ForCpp def isInJobChain: Boolean = {
    false
  }

  @ForCpp def setOrderControlled(): Unit = {
    ???
  }

  @ForCpp def setIdleTimeout(epochMillis: Long): Unit = {
    ???
  }

  @ForCpp def onOrderPossiblyAvailable(): Unit = {
    ???
  }

  @ForCpp def signalEarlierOrder(epochMillis: Long, orderName: String, function: String): Unit = {
    ???
  }

  @ForCpp def orderSetbackMaximum: Int =
    ???

  private def job =
    jobOption.getOrElse { error("Job is not initialized") }
}

object CppNewJob {
  private def timeZone(name: String) = name match {
    case "" => DateTimeZone.getDefault
    case _ => DateTimeZone forID name
  }
}
