package com.sos.scheduler.engine.test.util

import com.sos.jobscheduler.common.scalautil.Collections.implicits.RichArray
import java.net.URL
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
private[test] object JavaResourceResolver {

  private[test] def resourcePatternToUrls(pattern: String): immutable.Seq[URL] = {
    val r = new PathMatchingResourcePatternResolver
    r.getResources(s"classpath*:$pattern").toImmutableSeq map { _.getURL }
  }
}
