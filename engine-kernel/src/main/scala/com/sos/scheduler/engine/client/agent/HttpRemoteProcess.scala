package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.client.agent.HttpRemoteProcess._
import com.sos.scheduler.engine.client.command.HttpSchedulerCommandClient
import com.sos.scheduler.engine.client.command.RemoteSchedulers._
import com.sos.scheduler.engine.common.scalautil.xmls.StringSource
import scala.concurrent.{ExecutionContext, Future}

/**
 * A remote process started by [[HttpRemoteProcessStarter]].
 * @author Joacim Zschimmer
 */
final class HttpRemoteProcess(client: HttpSchedulerCommandClient, uri: String, processDescriptor: ProcessDescriptor)
  (implicit executionContext: ExecutionContext) {

  def killRemoteTask(unixSignal: Int): Future[Unit] = {
    require(unixSignal == 15, "SIGTERM (15) required")
    val command = <remote_scheduler.remote_task.kill process_id={processDescriptor.agentProcessId.string} signal="SIGTERM"/>
    client.uncheckedExecute(uri, command) map OkResult.fromXml
  }

  def closeRemoteTask(kill: Boolean): Future[Unit] = {
    val command = <remote_scheduler.remote_task.close process_id={processDescriptor.agentProcessId.string} kill={if (kill) true.toString else null}/>
    client.uncheckedExecute(uri, command) map OkResult.fromXml
  }

  override def toString = s"${getClass.getSimpleName}(processId=${processDescriptor.agentProcessId.string} pid=${processDescriptor.pid})"
}

object HttpRemoteProcess {
  case object OkResult {
    def fromXml(o: String) =
      readSchedulerResponse(StringSource(o)) { eventReader ⇒
        import eventReader._
        parseElement("ok") {}
      }
  }
}
