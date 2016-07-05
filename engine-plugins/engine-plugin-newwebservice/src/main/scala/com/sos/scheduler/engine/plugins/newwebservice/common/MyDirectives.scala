package com.sos.scheduler.engine.plugins.newwebservice.common

import spray.httpx.marshalling.ToResponseMarshallable
import spray.routing.StandardRoute

/**
 * @author Joacim Zschimmer
 */
trait MyDirectives {

  import spray.routing.directives.RouteDirectives._

  /**
   * Like complete, but wraps some Errors NonFatal regards as fatal.
   * But see this discussion:
   * <a href="https://groups.google.com/forum/#!topic/scala-language/eC9dqTTBYHg">https://groups.google.com/forum/#!topic/scala-language/eC9dqTTBYHg</a>
   */
  def toughComplete: (⇒ ToResponseMarshallable) ⇒ StandardRoute =
    marshallable ⇒ complete {
      try marshallable
      catch {
        case e @ (_: OutOfMemoryError | _: NotImplementedError | _: LinkageError) ⇒ throw new RuntimeException(e)
      }
    }
}

object MyDirectives extends MyDirectives
