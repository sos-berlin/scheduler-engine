package com.sos.scheduler.engine.plugins.newwebservice.common

import spray.routing.Directives._
import spray.routing._

/**
 * @author Joacim Zschimmer
 */
object SprayUtils {
  object implicits {
    implicit class RichOption[A](val delegate: Option[A]) extends AnyVal {
      def applyRoute(f: A ⇒ Route): Route =
        delegate match {
          case Some(a) ⇒ f(a)
          case None ⇒ reject
        }
    }
  }
}
