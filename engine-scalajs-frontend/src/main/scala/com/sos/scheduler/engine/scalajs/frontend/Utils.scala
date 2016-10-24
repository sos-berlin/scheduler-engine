package com.sos.scheduler.engine.scalajs.frontend

import org.scalajs.dom._
import org.scalajs.dom.ext.Ajax
import scala.scalajs.concurrent.JSExecutionContext.Implicits.queue
import scala.concurrent.Future
import scala.scalajs.js
import scala.scalajs.js.JSON

/**
  * @author Joacim Zschimmer
  */
object Utils {
  type EventId = Long

  def getDynamic(url: String): Future[js.Dynamic] =
    getJson(url) map { o ⇒ JSON.parse(o.responseText) }

  def getJson(url: String): Future[XMLHttpRequest] =
    Ajax.get(url, headers = Map("Accept" → "application/json"))
}
