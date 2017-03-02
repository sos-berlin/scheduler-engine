package com.sos.scheduler.engine.data.jobchain

import com.sos.scheduler.engine.data.filebased.FileBasedState
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JobChainOverviewTest extends FreeSpec {

  "JSON" in {
    val o = JobChainOverview(
      JobChainPath("/TEST"),
      FileBasedState.active,
      orderLimit = Some(1000),
      orderIdSpaceName = Some("ORDER-ID-SPACE"))
    val json = """{
        "path": "/TEST",
        "fileBasedState": "active",
        "isDistributed": false,
        "orderLimit": 1000,
        "orderIdSpaceName": "ORDER-ID-SPACE",
        "obstacles": []
      }""".parseJson
    assert(o.toJson == json)
    assert(json.convertTo[JobChainOverview] == o)
  }
}
