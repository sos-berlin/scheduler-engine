package com.sos.scheduler.engine.kernel.scheduler

import com.sos.scheduler.engine.kernel.http.{SchedulerHttpResponse, SchedulerHttpRequest}
import com.sos.scheduler.engine.kernel.cppproxy.{SpoolerC, HttpResponseC}
import javax.inject.{Inject, Singleton}

@Singleton
class SchedulerHttpService @Inject private(cppProxy: SpoolerC) {

  /** @return { @link HttpResponseC#close()} MUSS aufgerufen werden! */
  def executeHttpRequest (request: SchedulerHttpRequest, response: SchedulerHttpResponse): HttpResponseC =
    cppProxy.java_execute_http (request, response)
}
