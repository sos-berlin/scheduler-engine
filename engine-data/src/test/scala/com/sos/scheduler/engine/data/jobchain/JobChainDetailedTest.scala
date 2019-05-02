package com.sos.scheduler.engine.data.jobchain

import com.sos.scheduler.engine.data.filebased.FileBasedState
import java.nio.file.{Files, Paths}
import java.time.{Duration, Instant}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._
import com.sos.scheduler.engine.base.sprayjson.SprayJson._

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JobChainDetailedTest extends FreeSpec {
  "JSON" in {
    val o = JobChainDetailed(
      JobChainOverview(
        JobChainPath("/TEST"),
        FileBasedState.active,
        JobChainState.running,
        jobOrJobChainNodeCount = 3,
        nonBlacklistedOrderCount = 7,
        title = "TITLE"),
      List(
        EndNodeOverview(
          JobChainPath("/TEST"),
          NodeId("100"))),
      fileOrderSources =
        Vector(
          JobChainDetailed.FileOrderSource(
            directory = Paths.get("/DIRECTORY"),
            regex = "REGEX",
            repeat = Duration.ofSeconds(30),
            delayAfterError = Duration.ofSeconds(60),
            alertWhenDirectoryMissing = true,
            files = Vector(
              JobChainDetailed.FileOrderSourceFile(Paths.get("/DIRECTORY/FILE"), Instant.parse("2019-04-29T12:00:00Z"))))))
    val json = json"""
      {
        "overview": {
          "path": "/TEST",
          "fileBasedState": "active",
          "state": "running",
          "hasJobChainNodes":false,
          "jobOrJobChainNodeCount": 3,
          "nonBlacklistedOrderCount": 7,
          "blacklistedOrderCount": 0,
          "title": "TITLE",
          "isDistributed": false,
          "obstacles": []
        },
        "nodes": [
          {
            "TYPE": "End",
            "jobChainPath": "/TEST",
            "nodeId": "100"
          }
        ],
        "fileOrderSources": [
          {
            "directory": "/DIRECTORY",
            "regex": "REGEX",
            "repeat": 30,
            "delayAfterError": 60,
            "files": [
              {
                "file": "/DIRECTORY/FILE",
                "lastModified": "2019-04-29T12:00:00Z"
              }
            ],
            "alertWhenDirectoryMissing": true
          }
        ],
        "blacklistedOrders": []
      }"""
    assert(o.toJson == json)
    assert(json.convertTo[JobChainDetailed] == o)
  }
}
