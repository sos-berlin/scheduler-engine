package com.sos.scheduler.engine.kernel.database

import com.sos.jobscheduler.base.generic.SecretString
import com.sos.jobscheduler.common.scalautil.Collections.emptyToNone
import scala.collection.mutable

/**
  * @author Joacim Zschimmer
  */
private[kernel] final case class CppDatabaseProperties(
  url: String,
  driverClassName: Option[String] = None,
  user: Option[String] = None,
  password: Option[SecretString] = None,
  properties: Map[String, String] = Map())
{
  def toEntityManagerProperties: Map[String, String] = {
    val b = mutable.Buffer[(String, String)]()
    // "hibernate.show_sql" → "true"
    b += "javax.persistence.jdbc.url" → url
    for (o ← driverClassName) b += "javax.persistence.jdbc.driver" → o
    for (o ← user           ) b += "javax.persistence.jdbc.user" → o
    for (o ← password       ) b += "javax.persistence.jdbc.password" → o.string
    b.toMap
  }

  def toJdbcProperties: java.util.Properties = {
    val result = new java.util.Properties
    for (o ← user    ) result.setProperty("user", o)
    for (o ← password) result.setProperty("password", o.string)
    result
  }
}

private[database] object CppDatabaseProperties {
  def apply(map: Map[String, String]) = new CppDatabaseProperties(
    url = map("url"),
    driverClassName = map.get("jdbc.driverClass") flatMap emptyToNone,
    user = map.get("user") flatMap emptyToNone,
    password = map.get("password") flatMap emptyToNone map SecretString.apply,
    map - "password")
}
