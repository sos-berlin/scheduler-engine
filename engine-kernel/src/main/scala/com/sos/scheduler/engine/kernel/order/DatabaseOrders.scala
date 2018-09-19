package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.base.utils.ScalaUtils.RichAny
import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.scalautil.AutoClosing._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaStax.RichStartElement
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.xml.XmlUtils.xmlStringToBoolean
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId, NodeKey}
import com.sos.scheduler.engine.data.order.OrderPersistentState.{BlacklistDatabaseDistributedNextTime, NeverDatabaseDistributedNextTime, NowDatabaseDistributedNextTime, ReplacementDatabaseDistributedNextTime}
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, OrderKey, OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, QueryableOrder}
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem._
import com.sos.scheduler.engine.kernel.database.{DatabaseSubsystem, JdbcConnectionPool}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import java.io.Reader
import java.sql
import java.sql.ResultSet
import java.time.Instant
import javax.inject.{Inject, Singleton}
import javax.xml.stream.events.StartElement
import javax.xml.transform.stream.StreamSource
import scala.concurrent.blocking
import scala.util.control.ControlThrowable

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
      column = "JOB_CHAIN",
      jobChainPaths map { o ⇒ quoteSqlString(o.withoutStartingSlash) })

  def nodeKeysToSql(nodeKeys: TraversableOnce[NodeKey]): String =
    (for ((jobChainPath, nodeKeys) ← nodeKeys.toSeq groupBy { _.jobChainPath }) yield {
      nodeKeys.nonEmpty option
        """("JOB_CHAIN" = """ + quoteSqlString(jobChainPath.withoutStartingSlash) +
            " and " +
            databaseSubsystem.toInClauseSql(column = "STATE", nodeKeys map { o ⇒ quoteSqlString(o.nodeId.string) }) +
          ")"
    }).flatten mkString " and "

  def queryToSql(query: JobChainNodeQuery, conditionSql: String, ordered: Boolean = false): String = {
    val select = new StringBuilder(1000)
    select ++= "select "
    val limit = Int.MaxValue
    if (limit < Int.MaxValue) select ++= s"%limit($limit) "
    select ++= """"ID", "JOB_CHAIN", "STATE", "DISTRIBUTED_NEXT_TIME", "OCCUPYING_CLUSTER_MEMBER_ID", "ORDER_XML""""
    select ++= " from "
    select ++= schedulerConfiguration.ordersTableName
    select ++= """  where "SPOOLER_ID"="""
    select ++= DatabaseSubsystem.quoteSqlString(schedulerId.string.substitute("", "-"))
    select ++= """ and "DISTRIBUTED_NEXT_TIME" is not null"""
    if (conditionSql.nonEmpty) {
      select ++= s" and ($conditionSql)"
    }
    if (ordered) {
        select ++= """ order by "JOB_CHAIN", "STATE", "DISTRIBUTED_NEXT_TIME", "PRIORITY", "ORDERING""""
    }
    select.toString
  }
}

private[order] object DatabaseOrders {
  private[order] def fetchDistributedOrderStatistics(connection: sql.Connection, sqlStmt: String): JocOrderStatistics =
    autoClosing(connection.prepareStatement(sqlStmt)) { stmt ⇒
      val resultSet = stmt.executeQuery()
      fetchDistributedOrderStatistics(resultSet)
    }

  private def fetchDistributedOrderStatistics(resultSet: ResultSet): JocOrderStatistics = {
    blocking {
      var result = new JocOrderStatistics.Mutable
      while (resultSet.next()) {
        result.count(toQueryableOrder(
          OrderRow(resultSet),
          autoClosing(resultSet.getClob("ORDER_XML").getCharacterStream)(OrderXmlResolved.apply)))
      }
      result.toImmutable
    }
  }

  private def toQueryableOrder(row: OrderRow, xmlResolved: OrderXmlResolved): QueryableOrder =
    QueryableOrder.Standard(
      orderKey = row.orderKey,
      nodeId = row.nodeId,
      orderSourceType = xmlResolved.sourceType,
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
    def apply(reader: Reader): OrderXmlResolved = {
      var orderSourceType: OrderSourceType = null
      var isSuspended = false
      var isBlacklisted = false
      var isTouched = false
      var isSetback = false
      object Completed extends ControlThrowable
      val config = ScalaXMLEventReader.Config(ignoreUnknown = true)
      try ScalaXMLEventReader.parseDocument(new StreamSource(reader), config = config) { eventReader ⇒
        import eventReader._
        parseElement("order", withAttributeMap = false) {
          peek.asStartElement.attributes map { o ⇒ o.getName.getLocalPart → o.getValue } foreach {
            case ("suspended"        , value) ⇒ isSuspended   = xmlStringToBoolean(value)
            case ("on_blacklist"     , value) ⇒ isBlacklisted = xmlStringToBoolean(value)
            case ("touched"          , value) ⇒ isTouched     = xmlStringToBoolean(value)
            case ("setback"          , _    ) ⇒ isSetback = true
            case ("order_source_type", value) ⇒ orderSourceType = OrderSourceType.valueOf(value)
            case _ ⇒
          }
          if (orderSourceType != null) throw Completed
          eat[StartElement]
          while (peek.isStartElement) {
            peek.asStartElement.getName.toString match {
              case "file_based" ⇒ parseElement() {
                if (orderSourceType == null && attributeMap.contains("last_write_time")) {
                  orderSourceType = OrderSourceType.Permanent
                }
              }
              case "params" ⇒ parseElement() {
                forEachStartElement {
                  case "param" ⇒ parseElement() {
                    if (attributeMap.get("name") contains Order.FilePathParameterName) {
                      orderSourceType = OrderSourceType.FileOrder
                    }
                  }
                }
              }
              case _ ⇒ ignoreElement()
            }
            if (orderSourceType != null) throw Completed
          }
        }
      }
      catch { case Completed ⇒ }
      if (orderSourceType == null) {
        orderSourceType = OrderSourceType.AdHoc
      }
      new OrderXmlResolved(
        isSuspended = isSuspended,
        isBlacklisted = isBlacklisted,
        isSetback = isSetback,
        isTouched = isTouched,
        sourceType = orderSourceType)
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
}
