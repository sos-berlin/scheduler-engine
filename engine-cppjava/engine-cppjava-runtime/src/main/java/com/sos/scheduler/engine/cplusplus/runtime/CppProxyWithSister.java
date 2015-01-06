package com.sos.scheduler.engine.cplusplus.runtime;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.JavaOnlyInterface;

/**
 *
 * @author Zschimmer.sos
 */
@JavaOnlyInterface
public interface CppProxyWithSister<SISTER extends Sister> extends CppProxy, HasSister<SISTER> {}
