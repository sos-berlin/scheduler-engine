package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.base.utils.ScalaUtils.{RichAny, SwitchStatement}
import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.AutoClosing._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.RichFutures
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId, NodeKey}
import com.sos.scheduler.engine.data.order.OrderPersistentState.{BlacklistDatabaseDistributedNextTime, NeverDatabaseDistributedNextTime, NowDatabaseDistributedNextTime, ReplacementDatabaseDistributedNextTime}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderProcessingState, OrderSourceType, OrderStatistics}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, QueryableOrder}
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem._
import com.sos.scheduler.engine.kernel.database.{DatabaseSubsystem, JdbcConnectionPool}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import java.io.{Reader, StringReader}
import java.sql
import java.sql.ResultSet
import java.time.Instant
import javax.inject.{Inject, Singleton}
import javax.xml.transform.stream.StreamSource
import scala.collection.mutable
import scala.concurrent.{ExecutionContext, Future}

/**
  * @author Joacim Zschimmer
  */
@Singleton
private[order] final class DatabaseOrders @Inject private(
  schedulerId: SchedulerId,
  databaseSubsystem: DatabaseSubsystem,
  schedulerConfiguration: SchedulerConfiguration,
  jdbcConnectionPool: JdbcConnectionPool) {

  def jobChainPathsToSql(jobChainPaths: TraversableOnce[JobChainPath]): String =
    databaseSubsystem.toInClauseSql(
      column = "job_chain",
      jobChainPaths map { o ⇒ quoteSqlString(o.withoutStartingSlash) })

  def nodeKeysToSql(nodeKeys: TraversableOnce[NodeKey]): String =
    (for ((jobChainPath, nodeKeys) ← nodeKeys.toSeq groupBy { _.jobChainPath }) yield {
      nodeKeys.nonEmpty option
        "(" +
          quoteSqlName("job_chain") +
          "=" +
          quoteSqlString(jobChainPath.withoutStartingSlash) +
          " and " +
          databaseSubsystem.toInClauseSql(column = "state", nodeKeys map { o ⇒ quoteSqlString(o.nodeId.string) }) +
        ")"
    }).flatten mkString " and "

  def queryToSql(query: JobChainNodeQuery, conditionSql: String, ordered: Boolean = false): String = {
    val select = new StringBuilder(1000)
    select ++= "select "
    val limit = Int.MaxValue
    if (limit < Int.MaxValue) select ++= s"%limit($limit) "
    select ++= "`ID`, `JOB_CHAIN`, `STATE`, `DISTRIBUTED_NEXT_TIME`, `OCCUPYING_CLUSTER_MEMBER_ID`, `ORDER_XML`"
    select ++= " from "
    select ++= schedulerConfiguration.ordersTableName
    select ++= "  where `SPOOLER_ID`="
    select ++= DatabaseSubsystem.quoteSqlString(schedulerId.string.substitute("", "-"))
    select ++= " and `DISTRIBUTED_NEXT_TIME` is not null"
    if (conditionSql.nonEmpty) {
      select ++= s" and ($conditionSql)"
    }
    if (ordered) {
        select ++= " order by `JOB_CHAIN`, `STATE`, `DISTRIBUTED_NEXT_TIME`, `PRIORITY`, `ORDERING`"
    }
    select.toString
  }
}

private[order] object DatabaseOrders {
  private val ParallelizationTimeout = 10 * 60.s

  private[order] def fetchDistributedOrderStatistics(connection: sql.Connection, sqlStmt: String, parallelizeBelowOrderXmlSize: Int)(implicit ec: ExecutionContext): OrderStatistics = {
    val parallelization = sys.runtime.availableProcessors
    autoClosing(connection.prepareStatement(sqlStmt)) { stmt ⇒
      val resultSet = stmt.executeQuery()
      fetchDistributedOrderStatistics(resultSet, parallelization, parallelizeBelowOrderXmlSize = parallelizeBelowOrderXmlSize)
    }
  }

  private def fetchDistributedOrderStatistics(resultSet: ResultSet, parallelization: Int, parallelizeBelowOrderXmlSize: Int)(implicit ec: ExecutionContext): OrderStatistics = {
    require(parallelization >= 1)
    var result = new OrderStatistics.Mutable
    var hasNext = resultSet.next()
    while (hasNext) {
      val futures = new mutable.ArrayBuffer[Future[OrderStatistics]](parallelization)
      while (futures.size < parallelization && hasNext) {
        val row = OrderRow(resultSet)
        val clob = resultSet.getClob("ORDER_XML")
        hasNext = resultSet.next()
        if (clob.length * 2/*UTF-16*/ < parallelizeBelowOrderXmlSize) {
          val orderXml = clob.getSubString(1, clob.length.toInt)
          futures += Future {
            val xmlResolved = OrderXmlResolved(new StringReader(orderXml))  // May take longer
            toOrderStatistics(toQueryableOrder(row, xmlResolved))
          }
        } else {
          val xmlResolved = autoClosing(clob.getCharacterStream)(OrderXmlResolved.apply)
          result += toOrderStatistics(toQueryableOrder(row, xmlResolved))
        }
      }
      for (o ← futures await ParallelizationTimeout) {
        result += o
      }
    }
    result.toImmutable
  }

  private def toQueryableOrder(row: OrderRow, xmlResolved: OrderXmlResolved): QueryableOrder =
    QueryableOrder.Standard(
      orderKey = row.orderKey,
      nodeId = row.nodeId,
      sourceType = xmlResolved.sourceType,
      isSuspended = xmlResolved.isSetback,
      isSetback = xmlResolved.isSetback,
      isBlacklisted = xmlResolved.isBlacklisted,
      orderProcessingStateClass =
        if (xmlResolved.isSetback)
          classOf[OrderProcessingState.Setback]
        else
          row.resolvedDistributedNextTime match {
            case None if row.occupiedByClusterMember.isDefined ⇒ classOf[OrderProcessingState.OccupiedByClusterMember]
            case None ⇒ OrderProcessingState.NotPlanned.getClass
            case Some(_) if xmlResolved.isTouched ⇒ OrderProcessingState.WaitingForResource.getClass
            case Some(t) if Instant.now < t ⇒ classOf[OrderProcessingState.Planned]
            case Some(_) ⇒ classOf[OrderProcessingState.Due]
          })

  private case class OrderRow(
    orderKey: OrderKey,
    nodeId: NodeId,
    occupiedByClusterMember: Option[ClusterMemberId],
    resolvedDistributedNextTime: Option[Instant])

  private object OrderRow {
    def apply(resultSet: ResultSet) = {
      val orderKey = JobChainPath("/" + resultSet.getString("JOB_CHAIN")) orderKey "" + resultSet.getString("ID")
      val distributedNextTime = Option(resultSet.getDate("DISTRIBUTED_NEXT_TIME")) map dateToInstant
      val resolvedDistributedNextTime = resolveDatabaseDistributedNextTime(distributedNextTime)
      val occupiedByClusterMember = Option(resultSet.getString("occupying_cluster_member_id")) map ClusterMemberId.apply
      new OrderRow(
        orderKey = orderKey,
        nodeId = NodeId("" + resultSet.getString("STATE")),
        occupiedByClusterMember = occupiedByClusterMember,
        resolvedDistributedNextTime = resolvedDistributedNextTime)
        //isBlacklisted = distributedNextTime == Some(BlacklistDatabaseDistributedNextTime)
        //hasReplacements = distributedNextTime == Some(ReplacementDatabaseDistributedNextTime)
    }
  }

  private[order] final case class OrderXmlResolved(
    isSuspended: Boolean,
    isBlacklisted: Boolean,
    isSetback: Boolean,
    isTouched: Boolean,
    sourceType: OrderSourceType)

  private[order] object OrderXmlResolved {
    def apply(reader: Reader): OrderXmlResolved =
      ScalaXMLEventReader.parseDocument(new StreamSource(reader), ignoreUnknown = true) { eventReader ⇒
        import eventReader._
        parseElement("order") {
          var orderSourceType = OrderSourceType.AdHoc
          val isSuspended = attributeMap.as[Boolean]("suspended", false)
          val isBlacklisted = attributeMap.as[Boolean]("on_blacklist", false)
          val isSetback = attributeMap contains "setback"
          val isTouched = attributeMap.as[Boolean]("touched", false)
          forEachStartElement {
            case "file_based" ⇒ parseElement() {
              if (attributeMap contains "last_write_time") {
                orderSourceType = OrderSourceType.Permanent
              }
            }
            case "params" ⇒ parseElement() {
              forEachStartElement {
                case "param" ⇒ parseElement() {
                if (attributeMap.get("name") == Some(Order.FilePathParameterName)) {
                    orderSourceType = OrderSourceType.FileOrder
                  }
                }
              }
            }
          }
          new OrderXmlResolved(
            isSuspended = isSuspended,
            isBlacklisted = isBlacklisted,
            isSetback = isSetback,
            isTouched = isTouched,
            sourceType = orderSourceType)
        }
      }
  }

  private def resolveDatabaseDistributedNextTime(distributedNextTime: Option[Instant]) =
    distributedNextTime match {
      case Some(NowDatabaseDistributedNextTime) ⇒ Some(Instant.EPOCH)
      case Some(NeverDatabaseDistributedNextTime) ⇒ None
      case Some(BlacklistDatabaseDistributedNextTime) ⇒ None
      case Some(ReplacementDatabaseDistributedNextTime) ⇒ None
      case o ⇒ o
    }

  private[order] def toOrderStatistics(order: QueryableOrder): OrderStatistics = {
    import OrderProcessingState._
    def toInt(b: Boolean): Int = if (b) 1 else 0
    OrderStatistics(
      total       = 1,
      notPlanned  = toInt(order.orderProcessingStateClass == NotPlanned.getClass),
      planned     = toInt(order.orderProcessingStateClass == classOf[Planned]),
      due         = toInt(order.orderProcessingStateClass == classOf[Due]),
      inProcess   = toInt(order.orderProcessingStateClass == InTaskProcess.getClass),
      setback     = toInt(order.orderProcessingStateClass == classOf[Setback]),
      running     = toInt(classOf[Started] isAssignableFrom order.orderProcessingStateClass),
      inTask      = toInt(classOf[InTask] isAssignableFrom order.orderProcessingStateClass),
      suspended   = toInt(order.isSuspended),
      blacklisted = toInt(order.isBlacklisted),
      permanent   = toInt(order.sourceType == OrderSourceType.Permanent),
      fileOrder   = toInt(order.sourceType == OrderSourceType.FileOrder))
  }
}
