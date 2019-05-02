package com.sos.scheduler.engine.data.order

import com.sos.scheduler.engine.data.order.OrderPersistentState.BlacklistDatabaseDistributedNextTime
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class OrderPersistentStateTest extends FreeSpec
{
  "BlacklistDatabaseDistributedNextTime" in {
    assert(BlacklistDatabaseDistributedNextTime.toString == "3111-11-11T00:01:00Z")
  }
}
