package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting.Setting._

final case class Setting(details: SettingDetail*) {

  private val name = details flatMap {_.jobNameParts} mkString "_"
  val jobPath = JobPath(s"/test_$name")
  val orderKey = OrderKey("/test", name)
  private val orderState = s"$InitialOrderStatePrefix$name"

  val orderElem =
    <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string} state={orderState}>
      <params>{
        details flatMap { _.orderParams } map { case (n, v) => <param name={n} value={v}/> }
      }</params>
    </order>

  override def toString = details mkString ","
}

object Setting {
  val InitialOrderStatePrefix = "initial_"
}
