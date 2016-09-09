package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import collection.immutable
import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.data.order.{OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.queries.OrderQuery
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.seqFrag
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.OrderSelectionWidget._
import scalatags.Text.all._
import scalatags.Text.attrs
import spray.json._

/**
  * @author Joacim Zschimmer
  */
private[simplegui] final class OrderSelectionWidget(query: OrderQuery) {

  def html = List(
    raw(s"<script type='text/javascript'>$javascript</script>"),
    form(cls := "ContentBox OrderSelection", onsubmit := "javascript:reloadPage({}); return false")(
      table(
        tbody(
          tr(
            td(colspan := 2, paddingTop := 4.px, paddingBottom := 4.px)(
              "Show only ...")),
          tr(
            td(paddingRight := 6.px, rowspan := 2)(
              booleanCheckBoxes),
            td(paddingLeft := 6.px, paddingRight := 6.px, rowspan := 2, borderLeft := "1px solid #aaa")(
              orderSourceTypes),
            td(paddingLeft := 6.px, paddingRight := 6.px, rowspan := 2, borderLeft := "1px solid #aaa")(
              orderProcessingStates),
            td(verticalAlign := "top", paddingLeft := 6.px, paddingTop := 4.px, borderLeft := "1px solid #aaa")(
              limitPerNodeInput(query.notInTaskLimitPerNode))),
          tr(
            td(verticalAlign := "bottom", textAlign.right)(
              button(`type` := "submit")(
                StringFrag("Show"))))))))

  private def booleanCheckBoxes =
    for ((key, valueOption) ← List(OrderQuery.IsSuspendedName → query.isSuspended,
                                   OrderQuery.IsSetbackName → query.isSetback,
                                   OrderQuery.IsBlacklistedName → query.isBlacklisted,
                                   OrderQuery.IsDistributedName → query.isDistributed))
      yield List(
        labeledCheckbox(key, valueOption, checkedMeans = true),
        StringFrag(" "),
        labeledCheckbox(key, valueOption, checkedMeans = false),
        br)

  private def labeledCheckbox(key: String, value: Option[Boolean], checkedMeans: Boolean) = {
    val name = if (checkedMeans) key else s"not-$key"
    val checked = !checkedMeans ^ (value getOrElse !checkedMeans)
    val onClick = s"javascript:reloadPage({$key: document.getElementsByName('$name')[0].checked ? $checkedMeans : undefined})"
    label(
      input(attrs.name := name, `type` := "checkbox", checked option attrs.checked, attrs.onclick := onClick),
      span(position.relative, top := (-2).px)(
        boldIf(checked)(if (checkedMeans) removePrefixIs(key) else s"not")))
  }

  private def orderSourceTypes =
    enumHtml("isOrderSourceType", OrderSourceType.values map { _.name }, query.isOrderSourceType map { _ map { _.name }})

  private def orderProcessingStates =
    enumHtml("isOrderProcessingState", OrderProcessingState.typedJsonFormat.typeNames.toSeq, query.isOrderProcessingState map { _ map OrderProcessingState.typedJsonFormat.classToTypeName })

  private def enumHtml(key: String, names: Seq[String], selected: Option[Set[String]]) = {
    val onClick = s"javascript:reloadPage(selectionToKeyValue('$key', ${names.mkString("['", "','", "']")}))"
    for (name ← names;
         fieldName = s"$key-$name";
         checked = selected exists { _ contains name }) yield
      seqFrag(
        label(
          input(attrs.name := fieldName, `type` := "checkbox", checked option attrs.checked, attrs.onclick := onClick),
          span(position.relative, top := (-2).px)(
            boldIf(checked)(name))),
        br)
  }

  private def limitPerNodeInput(limitPerNode: Option[Int]) =
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
          attrs.value := limitPerNode map { _.toString } getOrElse "")))

  private def javascript = {
    val orderJson = JsObject(query.withoutPathToMap mapValues JsString.apply).toString
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
