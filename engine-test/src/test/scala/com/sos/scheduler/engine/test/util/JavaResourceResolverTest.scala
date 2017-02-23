package com.sos.scheduler.engine.test.util

import com.sos.jobscheduler.common.utils.JavaResource
import com.sos.scheduler.engine.test.util.JavaResourceResolver.resourcePatternToUrls
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JavaResourceResolverTest extends FreeSpec {

  private val dirResource = JavaResource("com/sos/scheduler/engine/test/util/test-resources/")
  private val txtResource = JavaResource("com/sos/scheduler/engine/test/util/test-resources/test.txt")
  private val xmlResource = JavaResource("com/sos/scheduler/engine/test/util/test-resources/test.xml")

  "resourcePatternToUrls with directory pattern" in {
    val urls = resourcePatternToUrls("com/sos/scheduler/engine/test/util/test-resources/")
    assert(urls.toSet == Set(dirResource.url))
  }

  "resourcePatternToUrls with star pattern" in {
    val urls = resourcePatternToUrls("com/sos/scheduler/engine/test/util/test-resources/*")
    assert(urls.toSet == Set(txtResource.url, xmlResource.url))
  }

  "resourcePatternToUrls with name pattern" in {
    val urls = resourcePatternToUrls("com/sos/scheduler/engine/test/util/test-resources/*.txt")
    assert(urls.toSet == Set(txtResource.url))
  }

  "resourcePatternToUrls with recusive directory pattern" in {
    val urls = resourcePatternToUrls("com/sos/scheduler/engine/test/util/**/*.txt")
    assert(urls contains txtResource.url)
    assert(!(urls contains dirResource.url))
    assert(urls.size > 3)
  }
}
