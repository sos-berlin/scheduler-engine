package com.sos.scheduler.engine.plugins.jetty.configuration

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.jaxrs.json.JacksonJsonProvider
import com.sos.scheduler.engine.data.configuration.EngineJacksonConfiguration.newObjectMapper
import javax.ws.rs.Produces
import javax.ws.rs.core.MediaType.APPLICATION_JSON
import javax.ws.rs.ext.Provider

/** @author Joacim Zschimmer */
//@Provider
//@Produces(Array(APPLICATION_JSON))
//class ObjectMapperJacksonJsonProvider(o: ObjectMapper) extends JacksonJsonProvider(o)

object ObjectMapperJacksonJsonProvider extends JacksonJsonProvider(newObjectMapper())
