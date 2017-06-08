package com.sos.scheduler.engine.data.job

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JobStateTest extends FreeSpec {

  private val stateToName = List(
    JobState.not_initialized → "not_initialized",
    JobState.initialized → "initialized",
    JobState.loaded → "loaded",
    JobState.stopping → "stopping",
    JobState.stopped → "stopped",
    JobState.error → "error",
    JobState.pending → "pending",
    JobState.running  → "running")

  for ((jobState, name) ← stateToName) name in {
    implicit def jsonFormat = JobState.MyJsonFormat
    assert(jobState.toJson == JsString(name))
    assert(('"' + name + '"').parseJson.convertTo[JobState] == jobState)
  }
}


