package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class StartNonApiTaskTest extends FreeSpec {

  "JSON minimum" in {
    val obj = StartNonApiTask(meta = None, logon = None)
    val json = """{ "$TYPE": "StartNonApiTask" }""".parseJson
    assert((obj: Command).toJson == json)   // Command serializer includes $TYPE
    assert(obj == json.convertTo[Command])
  }

  "JSON maximum" in {
    val obj = StartNonApiTask(
      Some(StartTask.Meta(JobPath("/folder/test"), TaskId(123))),
      Some(StartTask.KeyLogon("CREDENTIALS", false)))
    val json =
      """{
        "$TYPE": "StartNonApiTask",
        "meta": {
          "job": "/folder/test",
          "taskId": "123"
        },
        "logon": {
          "credentialsKey": "CREDENTIALS",
          "withUserProfile": false
        }
      }""".parseJson
    assert((obj: Command).toJson == json)   // Command serializer includes $TYPE
    assert(obj == json.convertTo[Command])
  }
}
