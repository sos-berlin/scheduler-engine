package com.sos.scheduler.engine.minicom.types

import com.sos.scheduler.engine.minicom.annotations.invocable

/**
 * Methods annotated with @[[invocable]] are callable via [[com.sos.scheduler.engine.minicom.Dispatcher]]
 * @author Joacim Zschimmer
 */
trait IDispatchable extends IUnknown
