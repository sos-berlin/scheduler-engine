package com.sos.scheduler.engine.plugins.jetty

import VerbRestrictionFilter._
import com.google.common.base.Splitter
import com.sos.scheduler.engine.common.scalautil.Logger
import javax.servlet._
import javax.servlet.http.{HttpServletResponse, HttpServletRequest}
import scala.collection.JavaConversions._

/**
 * Filters permitted verbs OPTIONS, GET, HEAD, PUT and POST.
 * Other verbs are rejected with "405 Method Not Allowed".
 * TRACE is not allowed because it can be used for a cross-site scripting attack.
 * @author Joacim Zschimmer
 */
final class VerbRestrictionFilter extends Filter {

  def init(filterConfig: FilterConfig): Unit = {}

  def destroy(): Unit = {}

  def doFilter(request: ServletRequest, response: ServletResponse, chain: FilterChain): Unit = {
    request match {
      case httpRequest: HttpServletRequest ⇒ filterHttp(httpRequest, response.asInstanceOf[HttpServletResponse], chain)
      case _ ⇒ chain.doFilter(request, response)
    }
  }

  private def filterHttp(request: HttpServletRequest, response: HttpServletResponse, chain: FilterChain): Unit = {
    val verb = request.getMethod
    if (!PermittedVerbs(verb)) {
      response.sendError(HttpServletResponse.SC_METHOD_NOT_ALLOWED)
    } else {
        chain.doFilter(request, response)
        for (allow <- Option(response.getHeader(Allow))) {   // Normalerweise nur OPTIONS and 405 "Method not allowed"
          val verbs = AllowSplitter split allow
          val filteredVerbs = verbs filter PermittedVerbs
          if (filteredVerbs.size < verbs.size) {
            val s = filteredVerbs mkString ","
            response.setHeader(Allow, s)
            logger.debug(s"Verb ${verbs.toSet diff filteredVerbs.toSet mkString ", "} removed - $Allow: $allow --> $s")
          }
        }
    }
  }
}

object VerbRestrictionFilter {
  private val PermittedVerbs = Set("OPTIONS", "GET", "HEAD", "PUT", "POST")   // TRACE could be used for cross-site scripting
  private val Allow = "Allow"
  private val AllowSplitter = (Splitter on "[\r\n \t]*,[\r\n \t]*".r.pattern).trimResults.omitEmptyStrings
  private val logger = Logger(getClass)
}
