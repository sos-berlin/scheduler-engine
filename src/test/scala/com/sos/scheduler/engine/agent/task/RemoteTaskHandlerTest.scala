package com.sos.scheduler.engine.agent.task

import com.google.inject.{AbstractModule, Guice, Provides}
import com.sos.scheduler.engine.agent.commands.{CloseRemoteTask, CloseRemoteTaskResponse, StartRemoteTask, StartRemoteTaskResponse}
import com.sos.scheduler.engine.agent.task.RemoteTaskHandlerTest._
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.taskserver.configuration.StartConfiguration
import javax.inject.Singleton
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.concurrent.AsyncAssertions.Waiter
import org.scalatest.concurrent.PatienceConfiguration.Timeout
import org.scalatest.junit.JUnitRunner
import org.scalatest.mock.MockitoSugar.mock
import org.scalatest.time.SpanSugar._
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import scala.util.{Failure, Success, Try}

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class RemoteTaskHandlerTest extends FreeSpec {
  private lazy val remoteTasks = List.fill(2) { mock[RemoteTask] }
  private lazy val remoteTaskHandler = Guice.createInjector(new TestModule(remoteTasks)).apply[RemoteTaskHandler]

  "StartRemoteTask" in {
    val w = new Waiter
    val command = StartRemoteTask(9000, usesApi = false, javaOptions = JavaOptions, javaClassPath = JavaClasspath)
    for (remoteTaskId ← RemoteTaskIds) {
      waiterOnComplete(w, remoteTaskHandler.executeCommand(command)) {
        case Success(StartRemoteTaskResponse(id)) ⇒
          w { id shouldEqual remoteTaskId }
          w.dismiss()
      }
    }
    w.await(Timeout(3000.millis))
    for (remoteTask ← remoteTasks) {
      verify(remoteTask, times(1)).start()
      verify(remoteTask, never).kill()
      verify(remoteTask, never).close()
    }
  }

  "CloseRemoteTask" in {
    val w = new Waiter
    val commands = List(
      CloseRemoteTask(remoteTasks(0).id, kill = false),
      CloseRemoteTask(remoteTasks(1).id, kill = true))
    for (command ← commands) {
      waiterOnComplete(w, remoteTaskHandler.executeCommand(command)) {
        case Success(CloseRemoteTaskResponse) ⇒
          w.dismiss()
      }
    }
    w.await(Timeout(3000.millis))

    verify(remoteTasks(0), times(1)).start()
    verify(remoteTasks(0), never).kill()
    verify(remoteTasks(0), times(1)).close()

    verify(remoteTasks(1), times(1)).start()
    verify(remoteTasks(1), times(1)).kill()
    verify(remoteTasks(1), times(1)).close()
  }
}

private object RemoteTaskHandlerTest {
  private val RemoteTaskIds = List(111111111111111111L, 222222222222222222L) map RemoteTaskId.apply
  private val JavaOptions = "JAVA-OPTIONS"
  private val JavaClasspath = "JAVA-CLASSPATH"

  private class TestModule(remoteTasks: List[RemoteTask]) extends AbstractModule {
    private val remoteTaskIterator = remoteTasks.iterator

    def configure() = {}

    @Provides @Singleton
    private def newRemoteTask: StartConfiguration ⇒ RemoteTask = { conf: StartConfiguration ⇒
      conf.javaOptions shouldEqual JavaOptions
      conf.javaClasspath shouldEqual JavaClasspath
      val remoteTask = remoteTaskIterator.synchronized { remoteTaskIterator.next() }
      verifyNoMoreInteractions(remoteTask)
      when(remoteTask.id) thenReturn conf.remoteTaskId
      remoteTask
    }

    @Provides @Singleton
    private def newRemoteTaskId: () ⇒ RemoteTaskId = RemoteTaskIds.synchronized { RemoteTaskIds.iterator.next }
  }

  def waiterOnComplete[A](w: Waiter, future: Future[A])(pf: PartialFunction[Try[A], Unit]): Unit =
    future.onComplete(pf orElse {
      case Failure(t) ⇒ w { fail(t.toString, t) }
      case o ⇒ w { fail(s"Unexpected: $o") }
    })
}
