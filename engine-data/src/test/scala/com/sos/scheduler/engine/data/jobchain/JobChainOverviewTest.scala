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
      JobChainState.running,
      orderLimit = Some(1000),
      jobOrJobChainNodeCount = 3,
      nonBlacklistedOrderCount = 7,
      blacklistedOrderCount = 9,
      title = "TITLE",
      orderIdSpaceName = Some("ORDER-ID-SPACE"))
    val json = """{
        "path": "/TEST",
        "fileBasedState": "active",
        "state": "running",
        "title": "TITLE",
        "hasJobChainNodes": false,
        "isDistributed": false,
        "orderLimit": 1000,
        "jobOrJobChainNodeCount": 3,
        "nonBlacklistedOrderCount": 7,
        "blacklistedOrderCount": 9,
        "orderIdSpaceName": "ORDER-ID-SPACE",
        "obstacles": []
      }""".parseJson
    assert(o.toJson == json)
    assert(json.convertTo[JobChainOverview] == o)
  }
}
