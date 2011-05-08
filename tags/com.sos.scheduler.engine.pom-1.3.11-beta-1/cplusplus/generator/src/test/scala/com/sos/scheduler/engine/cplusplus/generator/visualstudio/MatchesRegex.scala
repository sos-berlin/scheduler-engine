package com.sos.scheduler.engine.cplusplus.generator.visualstudio.visualstudio

import org.hamcrest._
import scala.util.matching.Regex


case class MatchesRegex(regex: Regex) extends BaseMatcher[String] {
    def matches(o: Object) = o match {
        case s: String => regex.findFirstIn(s).isDefined
    }

    def describeTo(d: Description) { 
        d.appendText("matches regex=");
    }
}
