package com.sos.scheduler.engine.cplusplus.generator.visualstudio

import org.hamcrest._


case class ContainsSingleString(singleString: String) extends BaseMatcher[String] {
    def matches(o: Object) = o match {
        case s: String =>
            val first = s.indexOf(singleString)
            first > 0  &&  s.indexOf(singleString, first + singleString.length) == -1
    }

    def describeTo(d: Description) {
        d.appendText("single occurrence of " + singleString);
    }
}
