package com.sos.scheduler.engine.common.process.windows

import com.sos.scheduler.engine.common.scalautil.Futures.implicits.RichFutures
import com.sos.scheduler.engine.common.time.ScalaTime._
import org.scalatest.FreeSpec
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
final class ResourceGuardTest extends FreeSpec {

  private final class G extends ResourceGuard("RESOURCE") {
    var released = 0
    def release(string: String) = released += 1
  }

  "ResourceGuard immediately" in {
    val g = new G
    assert(g { o ⇒ o } == Some("RESOURCE"))
    assert(g.released == 0)

    g.releaseAfterUse()
    assert(g.released == 1)

    assert(g { o ⇒ o } == None)
    assert(g.released == 1)

    g.releaseAfterUse()
    assert(g.released == 1)
  }

  "ResourceGuard delayed" in {
    val g = new G
    g {
      case Some("RESOURCE") ⇒
        g.releaseAfterUse()
        assert(g.released == 0)
      case _ ⇒
        fail()
    }
    assert(g.released == 1)
  }

  "ResourceGuard parallel" in {
    val g = new G
    var notReleased = 0
    val futures = for (_ ← 1 to Runtime.getRuntime.availableProcessors) yield Future {
      var stop = false
      while (!stop) {
        g {
          case Some("RESOURCE") ⇒
            assert(g.released == 0)
            notReleased += 1
          case Some(_) ⇒
            fail()
          case None ⇒
            stop = true
        }
      }
    }
    sleep(100.ms)
    g.releaseAfterUse()
    futures await 99.s
    assert(g.released == 1)
    assert(notReleased > 1)
  }
}
