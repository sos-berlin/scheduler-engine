package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.base.serial.PathAndParameterSerializable.toPathAndParameters
import com.sos.jobscheduler.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.jobscheduler.common.sprayutils.html.HtmlPage.{EmptyFrag, seqFrag}
import com.sos.scheduler.engine.data.order.{OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.OrderSelectionWidget._
import scalatags.Text.all._
import scalatags.Text.attrs
import spray.json._

/**
  * @author Joacim Zschimmer
  */
private[simplegui] final class OrderSelectionWidget(queryToUri: OrderQuery ⇒ String, query: OrderQuery) {

  def html: Frag = seqFrag(
    raw(s"<script type='text/javascript'>$javascript</script>"),
    form(cls := "ContentBox OrderSelection", onsubmit := "javascript:reloadPage({}); return false")(
      table(
        tbody(
          tr(
            th(cls := "BoxHeader OrderSelection-Header", colspan := 4)(
              "Show only ...")),
          tr(
            td(cls := "OrderSelection-Boolean")(
              booleanCheckBoxes),
            td(cls := "OrderSelection-Enum")(
              orderSourceTypesHtml),
            td(cls := "OrderSelection-Enum")(
              orderProcessingStatesHtml,
              orIsSuspendedHtml),
            td(cls := "OrderSelection-LimitPerNode")(
              limitPerNodeInputHtml(query.notInTaskLimitPerNode),
              deepFoldersHtml))))))

  private def booleanCheckBoxes =
    for ((key, valueOption) ← List(OrderQuery.IsSuspendedName → query.isSuspended,
                                   OrderQuery.IsSetbackName → query.isSetback,
                                   OrderQuery.IsBlacklistedName → query.isBlacklisted,
                                   JobChainQuery.IsDistributedName → query.jobChainQuery.isDistributed))
      yield List(
        labeledDoubleCheckbox(key, valueOption, checkedMeans = true),
        StringFrag(" "),
        labeledDoubleCheckbox(key, valueOption, checkedMeans = false),
        br)

  private def labeledDoubleCheckbox(key: String, value: Option[Boolean], checkedMeans: Boolean) = {
    val name = "OrderSelection-" + (if (checkedMeans) key else s"not-$key")
    val checked = !checkedMeans ^ (value getOrElse !checkedMeans)
    val onClick = s"javascript:reloadPage({$key: document.getElementsByName('$name')[0].checked ? $checkedMeans : undefined})"
    label(
      input(attrs.name := name, `type` := "checkbox", checked option attrs.checked, attrs.onclick := onClick),
      span(position.relative, top := (-2).px)(
        boldIf(checked)(if (checkedMeans) removePrefixIs(key) else s"not")))
  }

  private def orderSourceTypesHtml =
    enumHtml("isOrderSourceType", OrderSourceType.values map { _.name }, query.isOrderSourceType map { _ map { _.name }})

  private def orderProcessingStatesHtml = {
    import OrderProcessingState.typedJsonFormat
    enumHtml("isOrderProcessingState", typedJsonFormat.subtypeNames, query.isOrderProcessingState map { _ map typedJsonFormat.classToTypeName })
  }

  private def enumHtml(key: String, names: Seq[String], selected: Option[Set[String]]) = {
    val onClick = s"javascript:reloadPage(selectionToKeyValue('$key', ${names.mkString("['", "','", "']")}))"
    for (name ← names;
         fieldName = s"$key-$name";
         checked = selected exists { _ contains name }) yield
      seqFrag(
        div(
          label(
            input(attrs.name := fieldName, `type` := "checkbox", checked option attrs.checked, attrs.onclick := onClick),
            span(position.relative, top := (-2).px)(
              boldIf(checked)(name)))))
  }

  private def orIsSuspendedHtml: Frag = {
    val name = "OrderSelection-orIsSuspended"
    val onClick = s"javascript:reloadPage({ orIsSuspended: document.getElementsByName('$name')[0].checked ? true : undefined })"
    label(
      input(attrs.name := name, `type` := "checkbox", query.orIsSuspended option attrs.checked, attrs.onclick := onClick),
      span(position.relative, top := (-2).px)(
        boldIf(query.orIsSuspended)("orIsSuspended")))
  }

  private def limitPerNodeInputHtml(limitPerNode: Option[Int]) =
    div(
      div(marginBottom := 2.px,
        span(title := "Per node limit of orders currently not being executed by a task")(
          boldIf(limitPerNode.nonEmpty)(u("L"), StringFrag("imit orders not in task")))),  // "Limit orders not in task"
      div(
        input(
          attrs.name := "notInTaskLimitPerNode",
          accesskey := "L",
          width := 9.ch,
          `type` := "number",
          attrs.min := 0,
          attrs.value := limitPerNode map { _.toString } getOrElse ""),
        span(cls := "OrderSelection-LimitPerNode-Submit", colspan := 4)(
          button(`type` := "submit")(
            StringFrag("Show")))))

  private def deepFoldersHtml =
    query.jobChainQuery.pathQuery match {
      case pathQuery: PathQuery.Folder ⇒
        val name = "isRecursive"
        val checked = pathQuery.isRecursive
        val q = for (b ← Array(false, true)) yield
          queryToUri(query.copy(nodeQuery = query.nodeQuery.copy(jobChainQuery = query.jobChainQuery.copy(pathQuery = query.jobChainQuery.pathQuery.withRecursive(b)))))
        val onClick = s"javascript:window.location.href = document.getElementsByName('$name')[0].checked ? '${q(1)}' : '${q(0)}'"
        div(marginTop := 1.em)(
          label(
            input(attrs.name := name, `type` := "checkbox", checked option attrs.checked,
              attrs.onclick := onClick),
            span(position.relative, top := (-2).px)(
              boldIf(checked)("nested folders"))))
      case o ⇒ EmptyFrag
    }

  private def javascript = {
    val orderJson = JsObject(toPathAndParameters(query)._2 mapValues JsString.apply).toString
    s"""function reloadPage(change) {
      window.location.href = orderQueryToUrl($orderJson, change);
    }"""
  }
}

object OrderSelectionWidget {

  private def removePrefixIs(string: String) = {
    val (head, tail) = string stripPrefix "is" splitAt 2
    head.toLowerCase + tail
  }

  private def boldIf(flag: Boolean)(frags: Frag*): Frag = if (flag) b(frags) else frags
}
