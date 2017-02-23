package com.sos.scheduler.engine.plugins.newwebservice.routes.log

import akka.actor.{ActorRefFactory, Props}
import com.sos.jobscheduler.common.sprayutils.SprayUtils.accept
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration.SchedulerHttpCharset
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute._
import scala.concurrent.ExecutionContext
import spray.http.ContentType
import spray.http.MediaTypes.`text/plain`
import spray.httpx.marshalling.Marshaller
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait LogRoute {

  implicit protected def schedulerThreadCallQueue: SchedulerThreadCallQueue
  implicit protected def actorRefFactory: ActorRefFactory
  implicit protected def executionContext: ExecutionContext

  def logRoute(prefixLog: PrefixLog): Route = {
    accept(`text/plain`) {
      dynamic {
        implicit val marshaller = logMarshaller(`text/plain` withCharset SchedulerHttpCharset)
        complete(prefixLog)
      }
    }
  }
}

object LogRoute {
  private def logMarshaller(contentTypes: ContentType*)
    (implicit stcq: SchedulerThreadCallQueue, actorRefFactory: ActorRefFactory, ec: ExecutionContext): Marshaller[PrefixLog]
  =
    Marshaller.of[PrefixLog](contentTypes: _*) {
      case (prefixLog, contentType, ctx) ⇒ actorRefFactory.actorOf(Props { new LogActor(prefixLog, contentType, ctx) })
    }
}
