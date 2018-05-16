package com.sos.scheduler.engine.tests.extrascheduler

import akka.actor.ActorRefFactory
import com.google.inject.{AbstractModule, Guice, Provides}
import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.configuration.{AgentConfiguration, Akkas}
import com.sos.scheduler.engine.client.command.SchedulerClientFactory
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.Closers.withCloser
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.xmlcommands.StartJobCommand
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.binary.{CppBinariesDebugMode, TestCppBinaries}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.{ImplicitTimeout, ProvidesTestDirectory, TestEnvironment}
import javax.inject.Singleton
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class ExtraSchedulerIT extends FreeSpec with ProvidesTestDirectory {

  protected def testClass = getClass

  "ExtraScheduler" in {
    // Incomplete test. An example for a future SimpleExtraScheduler.
    withCloser { implicit closer ⇒
      val testEnvironment = TestEnvironment(TestConfiguration(testClass), testDirectory).closeWithCloser
      testEnvironment.prepare()
      val actorSystem = Akkas.newActorSystem(getClass.getSimpleName)
      closer.onClose { actorSystem.shutdown() }
      import actorSystem.dispatcher
      val injector = Guice.createInjector(new AbstractModule {
        @Provides @Singleton
        def actorRefFactory(): ActorRefFactory = actorSystem
      })
      //implicit val timeout = Timeout(10.s)
      implicit val implicitTimeout = ImplicitTimeout(10.s)
      val List(masterHttpPort, agentHttpPort) = findRandomFreeTcpPorts(2)
      val args = List(
        TestCppBinaries.cppBinaries(CppBinariesDebugMode.Debug).file(CppBinary.exeFilename).getPath,
        s"-sos.ini=${testEnvironment.sosIniFile}",
        s"-ini=${testEnvironment.iniFile}",
        s"-id=${TestEnvironment.TestSchedulerId}",
        s"-log-dir=${testEnvironment.logDirectory}",
        s"-log-level=debug9",
        s"-log=${testEnvironment.schedulerLog}",
        s"-java-classpath=${System.getProperty("java.class.path")}",
        s"-job-java-classpath=${System.getProperty("java.class.path")}",
        s"-roles=scheduler",
        s"-db=jdbc -class=org.h2.Driver jdbc:h2:mem:ExtraScheduler",
        s"-configuration-directory=${testEnvironment.liveDirectory}",
        (testEnvironment.configDirectory / "scheduler.xml").getPath)
      val scheduler = new ExtraScheduler(args, httpPort = Some(masterHttpPort)).closeWithCloser
      val agent = new Agent(AgentConfiguration.forTest(httpPort = agentHttpPort))
      awaitResults(List(scheduler.start(), agent.start()))

      val client = injector.instance[SchedulerClientFactory].apply(scheduler.uri)
      awaitSuccess(client execute <process_class name="agent" remote_scheduler={agent.localUri}/>)
      awaitResults(for (jobPath ← List(JobPath("/test-shell"), JobPath("/test-api"))) yield client.execute(StartJobCommand(jobPath)))

      sleep(3.s)
    }
  }
}
