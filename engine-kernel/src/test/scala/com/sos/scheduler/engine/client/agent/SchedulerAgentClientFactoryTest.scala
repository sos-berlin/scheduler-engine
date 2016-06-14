package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.client.agent.SchedulerAgentClientFactoryTest._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.RichPath
import java.nio.file.Files.{createTempFile, delete}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class SchedulerAgentClientFactoryTest extends FreeSpec {

  "readPassword from empty file" in {
    intercept[IllegalArgumentException] { testReadPassword("") }
  }

  "readPassword from good file" in {
    assert(testReadPassword("TEST") == SecretString("TEST"))
    assert(testReadPassword("TEST\n") == SecretString("TEST"))
    assert(testReadPassword("TEST\n\n") == SecretString("TEST"))
  }

  "readPassword from too big file" in {
    intercept[IllegalArgumentException] { testReadPassword("one\n" + "two\n") }
  }
}

private object SchedulerAgentClientFactoryTest {
  def testReadPassword(content: String) = {
    val file = createTempFile("SchedulerAgentClientFactoryTest-", ".tmp")
    try {
      file.contentString = content
      SchedulerAgentClientFactory.readPassword(file)
    }
    finally delete(file)
  }
}
