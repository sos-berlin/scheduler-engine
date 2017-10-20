package com.sos.scheduler.engine.kernel.command

import com.sos.scheduler.engine.common.xml.XmlUtils.newDocument

/**
  * @author Joacim Zschimmer
  */
trait Result

object Result {
  object OK extends Result

  final object Xmlizer extends ResultXmlizer {
    def getResultClass = OK.getClass
    def toElement(r: Result) = newDocument().createElement("ok")
  }
}
