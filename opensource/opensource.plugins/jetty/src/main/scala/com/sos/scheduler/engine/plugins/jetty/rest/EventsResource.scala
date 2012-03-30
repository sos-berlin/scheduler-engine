package com.sos.scheduler.engine.plugins.jetty.rest

import com.google.common.collect.AbstractIterator
import com.sos.scheduler.engine.eventbus.{EventHandlerAnnotated, EventBus, Event, EventHandler}
import com.sos.scheduler.engine.plugins.jetty.rest.WebServices._
import java.io.OutputStream
import java.util.concurrent.{TimeUnit, ArrayBlockingQueue}
import javax.inject.{Inject, Singleton}
import javax.ws.rs._
import javax.ws.rs.core.{StreamingOutput, MediaType, Response}
import org.codehaus.jackson.JsonGenerator
import org.codehaus.jackson.map.ObjectMapper
import org.slf4j.LoggerFactory

@Path("TESTONLY/events")
@Singleton
class EventsResource @Inject()(eventBus: EventBus){
  import EventsResource._

  private val objectMapper = {
    val result = new ObjectMapper()
    result.configure(JsonGenerator.Feature.AUTO_CLOSE_TARGET, false)
    result
  }

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
        case x => logger.error(x.toString); throw x
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

object EventsResource {
  private val maxQueueSize = 100 // Klein halten, weil EventBus.unregisterAnnotated() nicht aufgerufen wird => Speicherleck
  private val logger = LoggerFactory.getLogger(classOf[EventsResource])

  private class EventCollector extends AbstractIterator[Event] with EventHandlerAnnotated {
    val queue = new ArrayBlockingQueue[Event](maxQueueSize)//new mutable.SynchronizedQueue[Event]

    @EventHandler def handle(e: Event) {
      if (queue.size > maxQueueSize) queue.poll()
      queue.add(e)
    }

    def computeNext() = queue.poll(Integer.MAX_VALUE, TimeUnit.DAYS)
  }
}
