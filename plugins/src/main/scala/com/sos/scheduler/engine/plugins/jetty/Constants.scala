package com.sos.scheduler.engine.plugins.jetty

import javax.ws.rs.core.{MediaType, Variant}
import com.google.common.base.Charsets.UTF_8
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants._

object Constants {
  val schedulerTextPlainVariant = new Variant(MediaType.TEXT_PLAIN_TYPE, null, schedulerEncoding.name())
  val textPlainVariant = new Variant(MediaType.TEXT_PLAIN_TYPE, null, UTF_8.name)
  val textXmlVariant = new Variant(MediaType.TEXT_PLAIN_TYPE, null, null)
}