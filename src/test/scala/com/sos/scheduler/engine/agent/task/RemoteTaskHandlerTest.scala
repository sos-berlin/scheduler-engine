package com.sos.scheduler.engine.agent.task

import com.google.inject.{AbstractModule, Guice}
import com.sos.scheduler.engine.agent.commands.StartRemoteTask
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class RemoteTaskHandlerTest extends FreeSpec {

  "RemoteTaskHandler" in {
    pending
//    val injector = Guice.createInjector(new AbstractModule {
//      def configure = ???
//    })
//    val remoteTaskHandler = injector.apply[RemoteTaskHandler]
    //??? remoteTaskHandler.executeCommand(StartRemoteTask())
  }
}

private object RemoteTaskHandlerTest {
}
