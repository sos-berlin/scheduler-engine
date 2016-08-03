package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.data.queries.OrderQuery
import scalatags.Text.all._
import scalatags.Text.attrs

/**
  * @author Joacim Zschimmer
  */
private[html] final class OrderSelectionHtml(query: OrderQuery) {

  def html =
    div(cls := "OrderSelection")(
      div(paddingTop := 4.px, paddingBottom := 4.px)(i("Show only ...")),
      for ((key, valueOption) ← List("suspended" → query.isSuspended,
                                     "setback" → query.isSetback,
                                     "blacklisted" → query.isBlacklisted,
                                     "distributed" → query.isDistributed))
        yield List(
          inputElement(key, valueOption, checkedMeans = true),
          StringFrag(" "),
          inputElement(key, valueOption, checkedMeans = false),
          br))

  private def inputElement(key: String, value: Option[Boolean], checkedMeans: Boolean) = {
    val name = if (checkedMeans) key else s"not-$key"
    val checked = !checkedMeans ^ (value getOrElse !checkedMeans)
    val onClick = s"javascript:reloadPage({$key: document.getElementsByName('$name')[0].checked ? $checkedMeans : undefined})"
    label(
      input(attrs.name := name, `type` := "checkbox", checked option attrs.checked, attrs.onclick := onClick),
      if (checkedMeans) key else s"not")
  }

  def javascript = s"""
    function reloadPage(change) {
      var query = ${toJavascript(query)};
      var key, v;
      var q = [];
      for (key in change) if (change.hasOwnProperty(key)) {
        v = change[key]
        if (typeof v == 'undefined') delete query[key]; else query[key] = v;
      }
      for (key in query) if (query.hasOwnProperty(key)) q.push(key + '=' + query[key].toString());
      var href = window.location.href.replace(/\\?.*/g, '');
      if (q.length) href += "?" + q.join('&');
      window.location.href = href;
    }"""

  private def toJavascript(query: OrderQuery) =
    ((for (o ← query.isSuspended) yield s"suspended:$o") ++
    (for (o ← query.isSetback) yield s"setback:$o") ++
    (for (o ← query.isBlacklisted) yield s"blacklisted:$o") ++
    (for (o ← query.isDistributed) yield s"distributed:$o"))
      .mkString("{", ",", "}")
}
