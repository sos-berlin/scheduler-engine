package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.data.queries.OrderQuery
import scalatags.Text.all._
import scalatags.Text.attrs
import spray.json.{JsObject, JsString}

/**
  * @author Joacim Zschimmer
  */
private[html] final class OrderSelectionHtml(query: OrderQuery) {

  def html =
    form(cls := "OrderSelection", onsubmit := "javascript:reloadPage({}); return false")(
      table(
        tbody(
          tr(
            td(colspan := 2, paddingTop := 4.px, paddingBottom := 4.px)("Show only ...")),
          tr(
            td(paddingRight := 6.px, rowspan := 2)(
              for ((key, valueOption) ← List("suspended" → query.isSuspended,
                                             "setback" → query.isSetback,
                                             "blacklisted" → query.isBlacklisted,
                                             "distributed" → query.isDistributed))
                yield List(
                  labeledCheckbox(key, valueOption, checkedMeans = true),
                  StringFrag(" "),
                  labeledCheckbox(key, valueOption, checkedMeans = false),
                  br)),
            td(
              verticalAlign := "top",
              paddingLeft := 6.px,
              paddingTop := 6.px,
              borderLeft := "1px solid #aaa",
              limitPerNodeInput(query.limitPerNode))),
          tr(
            td(verticalAlign := "bottom", textAlign.right)(
              button(`type` := "submit")(
                StringFrag("Show")))))))

  private def labeledCheckbox(key: String, value: Option[Boolean], checkedMeans: Boolean) = {
    val name = if (checkedMeans) key else s"not-$key"
    val checked = !checkedMeans ^ (value getOrElse !checkedMeans)
    val onClick = s"javascript:reloadPage({$key: document.getElementsByName('$name')[0].checked ? $checkedMeans : undefined})"
    label(
      input(attrs.name := name, `type` := "checkbox", checked option attrs.checked, attrs.onclick := onClick),
      if (checkedMeans) key else s"not")
  }

  def limitPerNodeInput(limitPerNode: Option[Int]) =
    div(
      div(marginBottom := 2.px,
        u("L"), StringFrag("imit per node")),  // "Limit per node"
      div(
        input(
          attrs.name := "limitPerNode",
          accesskey := "L",
          width := 7.ch,
          `type` := "number",
          attrs.min := 1,
          attrs.value := limitPerNode map { _.toString } getOrElse "")))

  def javascript = s"""
    function reloadPage(change) {
      var query = ${toJson(query)};
      var key, v;
      var q = [];
      for (key in change) if (change.hasOwnProperty(key)) {
        v = change[key]
        if (typeof v == 'undefined') delete query[key]; else query[key] = v;
      }
      var limitPerNode = document.getElementsByName('limitPerNode')[0].value;
      if (typeof limitPerNode == 'undefined' || limitPerNode == '') delete query['limitPerNode']; else query.limitPerNode = limitPerNode;
      for (key in query) if (query.hasOwnProperty(key)) q.push(key + '=' + query[key].toString());
      var href = window.location.href.replace(/\\?.*/g, '');
      if (q.length) href += "?" + q.join('&');
      window.location.href = href;
    }"""

  private def toJson(query: OrderQuery) = JsObject(query.withoutPathToMap mapValues JsString.apply)
}
