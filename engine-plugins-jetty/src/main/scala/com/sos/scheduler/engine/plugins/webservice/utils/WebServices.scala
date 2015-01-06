package com.sos.scheduler.engine.plugins.webservice.utils

import com.google.common.base.Charsets._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants._
import javax.ws.rs.core.{MediaType, Variant, CacheControl}

object WebServices {
  private val nullLocale: java.util.Locale = null
  val schedulerTextPlainVariant = new Variant(MediaType.TEXT_PLAIN_TYPE, nullLocale, schedulerEncoding.name)
  val textPlainVariant = new Variant(MediaType.TEXT_PLAIN_TYPE, nullLocale, UTF_8.name)

  val noCache = {
    val result = new CacheControl
    result.setNoCache(true)
    result
  }

  def wrapXmlResponse(e: Seq[xml.Elem]) =
    <scheduler>{e}</scheduler>

  def stripFromEnd(s: String, end: String) =
    s ensuring { _ endsWith end } substring (0, s.length - end.length)
}
