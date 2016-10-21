function startOrderStatisticsChangedListener(path) {
  var tryAgainTimeoutSeconds = 10
  var keys = [
    "total",
    "notPlanned",
    "planned",
    "due",
    "started",
    "inTask",
    "inProcess",
    "setback",
    "suspended",
    "blacklisted",
    "permanent",
    "fileOrder"];
  var fields = {};
  var i, key;
  for (i in keys) {
    key = keys[i];
    fields[key] = {
      fieldDom: document.getElementById('order-' + key + "-field"),
      valueDom: document.getElementById('order-' + key + "-value")
    }
  }
  var timestampValueDom = document.getElementById('order-timestamp-value');
  var refreshElem = document.getElementById('OrderStatistics-refresh');
  var widgetJq = $('#OrderStatistics');
  var current = {}
  var delayedGetNextEvent = null;

  function documentVisibilityChanged() {
    if (!document.hidden && delayedGetNextEvent) {
      var f = delayedGetNextEvent;
      delayedGetNextEvent = null;
      f();
    }
  }
  document.addEventListener("visibilitychange", documentVisibilityChanged, false);

  function getNext(lastEventId) {
    if (lastEventId !== 1*lastEventId) throw Error("Invalid argument lastEventId=" + lastEventId)
    var requestedAt = new Date()
    jQuery.ajax({
      dataType: 'json',
      url: "/jobscheduler/master/api/event" + path + "?return=OrderStatisticsChanged&timeout=60s&after=" + lastEventId
    })
    .done(function(eventSeq) {
      var continueAfterEventId = processEventSeq(eventSeq)
      function f() {
        getNext(continueAfterEventId);
      }
      if (document.hidden) {
        showRefreshing();
        delayedGetNextEvent = f;
      } else {
        setTimeout(f, 500);
      }
    })
    .fail(function() {
      showRefreshing();
      var duration = tryAgainTimeoutSeconds - Math.max(0, new Date().getSeconds() - requestedAt.getSeconds());
      setTimeout(function() { getNext(lastEventId); }, duration * 1000);
    });
  }

  function processEventSeq(eventSeq) {
    showNewEvent(eventSeq.eventId)
    switch (eventSeq.TYPE) {
      case "NonEmpty":
        var eventSnapshot = eventSeq.eventSnapshots[0]
        var orderStatistics = eventSnapshot.orderStatistics;
        showOrderStatistics(orderStatistics)
        current = orderStatistics;
        return eventSnapshot.eventId;
      case "Empty":  // Timed-out
        return lastEventId;
      case "Torn":
        return eventSeq.lastEventId;
    }
  }

  function showNewEvent(eventId) {
    refreshElem.style.visibility = "hidden";
    widgetJq.removeClass('OrderStatistics-error');
    timestampValueDom.innerText = new Date(eventId / 1000).toTimeString().substring(0, 8);
  }

  function showOrderStatistics(orderStatistics) {
    for (i in keys) {
      var key = keys[i];
      if (orderStatistics[key] !== current[key]) {
        var field = fields[key];
        field.valueDom.innerText = orderStatistics[key].toString();
        if (typeof current[key] !== "undefined") {  // Not the first change?
          var style = field.fieldDom.style
          // 'alt' alternates between 1 and 0 to force the animation to restart
          var alt = 1 - 1 * style.animationName.substring(style.animationName.length - 1)
          style.animationName = orderStatistics[key] > current[key]? 'OrderStatistics-higher-' + alt : 'OrderStatistics-lower-' + alt;
        }
      }
    }
  }

  function showRefreshing() {
    refreshElem.style.visibility = "inherit";
    widgetJq.addClass('OrderStatistics-error');
  }

  getNext(0);
}
