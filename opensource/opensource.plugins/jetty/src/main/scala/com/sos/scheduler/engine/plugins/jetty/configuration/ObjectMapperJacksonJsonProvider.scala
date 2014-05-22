package com.sos.scheduler.engine.plugins.jetty.configuration

import com.fasterxml.jackson.jaxrs.json.JacksonJsonProvider
import com.sos.scheduler.engine.data.configuration.EngineJacksonConfiguration.newObjectMapper

/** @author Joacim Zschimmer */
object ObjectMapperJacksonJsonProvider extends JacksonJsonProvider(newObjectMapper())
