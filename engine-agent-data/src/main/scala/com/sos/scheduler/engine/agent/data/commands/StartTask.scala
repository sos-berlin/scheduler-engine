package com.sos.scheduler.engine.agent.data.commands

import com.sos.scheduler.engine.agent.data.responses.StartTaskResponse

/**
 * @author Joacim Zschimmer
 */
trait StartTask extends TaskCommand {
  type Response = StartTaskResponse
}
