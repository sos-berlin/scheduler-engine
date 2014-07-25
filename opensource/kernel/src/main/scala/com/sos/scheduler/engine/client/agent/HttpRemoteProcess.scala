package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.client.agent.HttpRemoteProcess._
import com.sos.scheduler.engine.client.command.RemoteSchedulers._
import com.sos.scheduler.engine.client.command.{HttpSchedulerCommandClient, RemoteSchedulers}
import com.sos.scheduler.engine.common.scalautil.xml.StringSource
import scala.concurrent.{ExecutionContext, Future}

/** A remote process started by [[HttpRemoteProcessStarter]].
 * @author Joacim Zschimmer
 */
final class HttpRemoteProcess(client: HttpSchedulerCommandClient, val processDescriptor: ProcessDescriptor)(implicit executionContext: ExecutionContext) {

  def closeRemoteTask(kill: Boolean): Future[Unit] = {
    val command = <remote_scheduler.remote_task.close process_id={processDescriptor.processId} kill={if (kill) true.toString else null}/>
    client.execute(command) map CloseResult.fromXml
  }

  override def toString = s"${getClass.getSimpleName}(processId=${processDescriptor.processId} pid=${processDescriptor.pid})"

  def agentProcessId = processDescriptor.processId
  def pid = processDescriptor.pid
}

object HttpRemoteProcess {
  case object CloseResult {
    def fromXml(o: String) =
      readSchedulerResponse(StringSource(o)) { eventReader ⇒
        import eventReader._
        parseElement("ok") {
          ignoreAttributes()
        }
      }
  }
}
