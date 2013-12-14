package com.sos.scheduler.engine.plugins.webservice.services

import EventsService._
import com.google.common.collect.AbstractIterator
import com.sos.scheduler.engine.data.configuration.EngineJacksonConfiguration
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.eventbus.{EventHandlerAnnotated, EventBus, EventHandler}
import com.sos.scheduler.engine.plugins.webservice.utils.WebServices.noCache
import java.io.OutputStream
import java.util.concurrent.{TimeUnit, ArrayBlockingQueue}
import javax.inject.{Inject, Singleton}
import javax.ws.rs._
import javax.ws.rs.core.{StreamingOutput, MediaType, Response}
import org.slf4j.LoggerFactory

@Path("TESTONLY/events")
@Singleton
final class EventsService @Inject private(eventBus: EventBus) {

  private val objectMapper = EngineJacksonConfiguration.newObjectMapper()

  @GET
  @Produces(Array(MediaType.APPLICATION_JSON))
  def get = {
    val result = new MyStreamingOutput(new EventCollector)
    Response.ok(result).cacheControl(noCache).build()
  }

  private class MyStreamingOutput(eventCollector: EventCollector) extends StreamingOutput {
    eventBus.registerAnnotated(eventCollector)  // TODO Wenn write() nicht aufgerufen wird, bleibt der EventHandler registriert

    def write(out: OutputStream) {
      try writeEvents(out)
      catch {
        case x: InterruptedException =>
        case x: Throwable => logger.error(x.toString); throw x
      } finally {
        eventBus.unregisterAnnotated(eventCollector)
      }
    }

    private def writeEvents(out: OutputStream) {
      while (eventCollector.hasNext) {
        val event = eventCollector.next()
        objectMapper.writeValue(out, event)
        out.write('\n')
        out.flush()
      }
    }
  }
}

object EventsService {
  private val maxQueueSize = 100 // Klein halten, weil EventBus.unregisterAnnotated() nicht aufgerufen wird => Speicherleck
  private val logger = LoggerFactory.getLogger(classOf[EventsService])

  private class EventCollector extends AbstractIterator[Event] with EventHandlerAnnotated {
    val queue = new ArrayBlockingQueue[Event](maxQueueSize)//new mutable.SynchronizedQueue[Event]

    @EventHandler def handle(e: Event) {
      if (queue.size > maxQueueSize) queue.poll()
      queue.add(e)
    }

    def computeNext() = queue.poll(Integer.MAX_VALUE, TimeUnit.DAYS)
  }
}
