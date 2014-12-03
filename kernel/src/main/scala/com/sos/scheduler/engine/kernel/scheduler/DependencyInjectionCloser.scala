package com.sos.scheduler.engine.kernel.scheduler

import com.google.common.io.Closer

/**
 * @author Joacim Zschimmer
 */
final case class DependencyInjectionCloser(closer: Closer)
