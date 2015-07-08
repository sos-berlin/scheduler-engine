package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.client.agent.HttpRemoteProcess._
import com.sos.scheduler.engine.client.command.RemoteSchedulers._
import com.sos.scheduler.engine.client.command.SchedulerCommandClient
import com.sos.scheduler.engine.common.scalautil.xmls.StringSource
import scala.concurrent.{ExecutionContext, Future}

/**
 * A remote process started by [[HttpRemoteProcessStarter]].
 *
 * @author Joacim Zschimmer
 */
trait HttpRemoteProcess {

  protected def classicClient: SchedulerCommandClient
  protected def processDescriptor: ProcessDescriptor
  protected implicit def executionContext: ExecutionContext

  def start(): Unit
  def close(): Unit

  final def killRemoteTask(unixSignal: Int): Future[Unit] = {
    require(unixSignal == 15, "SIGTERM (15) required")
    val command = <remote_scheduler.remote_task.kill process_id={processDescriptor.agentProcessId.value.toString} signal="SIGTERM"/>
    classicClient.uncheckedExecute(command) map OkResult.fromXml
  }

  final def closeRemoteTask(kill: Boolean): Future[Unit] = {
    val command = <remote_scheduler.remote_task.close process_id={processDescriptor.agentProcessId.value.toString} kill={if (kill) true.toString else null}/>
    classicClient.uncheckedExecute(command) map OkResult.fromXml
  }

  override def toString = s"${getClass.getSimpleName}(${processDescriptor.agentProcessId} pid=${processDescriptor.pid})"
}

object HttpRemoteProcess {
  private case object OkResult {
    def fromXml(o: String) =
      readSchedulerResponse(StringSource(o)) { eventReader ⇒
        import eventReader._
        parseElement("ok") {}
      }
  }

  final class Standard(
    protected val classicClient: SchedulerCommandClient,
    protected val processDescriptor: ProcessDescriptor,
    protected val executionContext: ExecutionContext)
  extends HttpRemoteProcess {

    def start() = {}
    def close() = {}
  }
}
