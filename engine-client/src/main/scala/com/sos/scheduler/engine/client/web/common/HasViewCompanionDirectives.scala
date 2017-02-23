package com.sos.scheduler.engine.client.web.common

import com.sos.jobscheduler.common.sprayutils.SprayUtils._
import com.sos.scheduler.engine.data.common.HasViewCompanion
import shapeless.{::, HNil}
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object HasViewCompanionDirectives {

  def viewReturnParameter[Super](hasViewCompanion: HasViewCompanion.WithKnownSubtypes[Super], default: HasViewCompanion.Companion[_ <: Super])
  : Directive1[hasViewCompanion.AnyCompanion[Super]]
  =
    new Directive1[hasViewCompanion.AnyCompanion[Super]] {
      def happly(inner: (hasViewCompanion.AnyCompanion[Super] :: HNil) ⇒ Route) =
        parameter("return" ? default.name) { returnType ⇒
          hasViewCompanion.companion(returnType) match {
            case Right(companion) ⇒ inner(companion :: HNil)
            case _ ⇒ reject  // Ignore error message and continue with other routes
          }
        }
    }
}
