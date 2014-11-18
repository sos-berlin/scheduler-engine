package com.sos.scheduler.engine.data.xmlcommands

import com.sos.scheduler.engine.data.job.JobPath
import org.scalatest.FreeSpec
import org.scalatest.Matchers._

/**
 * @author Joacim Zschimmer
 */
final class ModifyJobCommandTest extends FreeSpec {

  "ModifyJobCommand" in {
    ModifyJobCommand(JobPath("/a")).xmlElem shouldEqual <modify_job job="/a"/>
    ModifyJobCommand(JobPath("/a"), cmd = Some(ModifyJobCommand.Cmd.Stop)).xmlElem shouldEqual <modify_job job="/a" cmd="stop"/>
  }
}
