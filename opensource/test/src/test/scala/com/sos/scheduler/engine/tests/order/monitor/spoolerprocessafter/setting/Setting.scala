package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting

import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.job.JobPath

case class Setting(details: SettingDetail*) {

  import Setting._

  private val name = details flatMap {_.jobNameParts} mkString "_"

  val orderKey = OrderKey("/test", name)

  private val orderState = initialOrderStatePrefix + name

  val orderElem =
    <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string} state={orderState}>
      <params>{
        details flatMap { _.orderParams } map { case (n, v) => <param name={n} value={v}/> }
      }</params>
    </order>

  val jobPath = JobPath("/test_" + name)

  override def toString = details mkString ","
}

object Setting {
  val initialOrderStatePrefix = "initial_"
}