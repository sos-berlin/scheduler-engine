package com.sos.scheduler.engine.plugins.jetty.rest

import javax.ws.rs.Produces
import javax.ws.rs.core.MediaType.APPLICATION_JSON
import javax.ws.rs.ext.Provider
import org.codehaus.jackson.jaxrs.JacksonJsonProvider
import org.codehaus.jackson.map.ObjectMapper

/** Ein javax.ws.rs.ext.MessageBodyReader und @link javax.ws.rs.ext.MessageBodyWriter zum Anschluss
 * an Jackson, der Java-Objekte und JSON konvertiert.
 * @author Joacim Zschimmer */
@Provider
@Produces(Array(APPLICATION_JSON))
class ObjectMapperJacksonJsonProvider(o: ObjectMapper) extends JacksonJsonProvider(o)
