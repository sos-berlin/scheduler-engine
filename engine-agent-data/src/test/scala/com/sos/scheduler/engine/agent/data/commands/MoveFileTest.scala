package com.sos.scheduler.engine.agent.data.commands

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class MoveFileTest extends FreeSpec {

   "JSON" in {
     val obj = MoveFile("OLD", "NEW")
     val json = """{
       "$TYPE": "MoveFile",
       "path": "OLD",
       "newPath": "NEW"
     }""".parseJson
     assert((obj: Command).toJson == json)   // Command serializer includes $TYPE
     assert(obj == json.convertTo[Command])
   }
 }
