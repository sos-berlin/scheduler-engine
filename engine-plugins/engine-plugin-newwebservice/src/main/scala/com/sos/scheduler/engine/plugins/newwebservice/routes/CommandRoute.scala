package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.api.CommandClient
import com.sos.jobscheduler.common.scalautil.xmls.SafeXML
import com.sos.jobscheduler.common.sprayutils.SprayUtils.{completeWithError, emptyParameterMap}
import com.sos.jobscheduler.common.sprayutils.XmlString
import com.sos.scheduler.engine.kernel.DirectCommandClient._
import scala.concurrent.ExecutionContext
import spray.http.HttpEntity
import spray.http.StatusCodes._
import spray.routing.Directives._

/**
  * @author Joacim Zschimmer
  */
trait CommandRoute {
  protected def client: CommandClient
  protected implicit def executionContext: ExecutionContext

  def commandRoute =
    pathEnd {
      post {
        entity(as[XmlString]) { case XmlString(xmlString) ⇒
          complete(client.uncheckedExecuteXml(xmlString) map XmlString.apply)
        }
      } ~
      get {
        parameterMap { parameterMap ⇒
          val command = parameterMap.get("command") match {
            case Some(cmd) if cmd startsWith "<" ⇒ cmd
            case Some(cmd) ⇒ s"<$cmd/>"
            case None ⇒ <s/>.toString
          }
          emptyParameterMap(parameterMap - "command") {
            if (commandIsReadOnly(command)) {
              val elem = SafeXML.loadString(command)  // Verify valid XML
              complete(client.uncheckedExecute(elem) map XmlString.apply)
            } else
              completeWithError(Forbidden, "Only a read-only command is allowed")
          }
        }
      }
    }

  def untypedPostCommandRoute =
    pathEnd {
      post {
        entity(as[HttpEntity]) { entity ⇒
          complete(client.uncheckedExecuteXml(entity.asString) map XmlString.apply)
        }
      }
    }
}
