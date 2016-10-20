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

  function get(lastEventId) {
    if (lastEventId !== 1*lastEventId) throw Error("Invalid argument lastEventId=" + lastEventId)
    var requestedAt = new Date()
    jQuery.ajax({
      dataType: 'json',
      url: "/jobscheduler/master/api/event" + path + "?return=OrderStatisticsChanged&timeout=60s&after=" + lastEventId
    })
    .done(function(snapshot) {
      refreshElem.style.visibility = "hidden";
      widgetJq.removeClass('OrderStatistics-error');
      timestampValueDom.innerText = new Date(snapshot.eventId / 1000).toTimeString().substring(0, 8);
      var continueAfterEventId
      if (snapshot.hasOwnProperty("eventSnapshots")) {
        if (snapshot.eventSnapshots.length > 0) {
          var eventSnapshots = snapshot.eventSnapshots;
          var event = eventSnapshots[0];
          var stat = event.orderStatistics;
          for (i in keys) {
            var key = keys[i];
            if (stat[key] !== current[key]) {
              var field = fields[key];
              field.valueDom.innerText = stat[key].toString();
              if (typeof current[key] !== "undefined") {  // Not the first change?
                var style = field.fieldDom.style
                // 'alt' alternates between 1 and 0 to force the animation to restart
                var alt = 1 - 1 * style.animationName.substring(style.animationName.length - 1)
                style.animationName = stat[key] > current[key]? 'OrderStatistics-higher-' + alt : 'OrderStatistics-lower-' + alt;
              }
            }
          }
          current = stat;
          continueAfterEventId = event.eventId;
        } else {
          // No data, timed-out
          continueAfterEventId = lastEventId
        }
      } else {
        // Teared
        continueAfterEventId = snapshot.lastEventId;
      }
      function getNextEvent() { get(continueAfterEventId); }
      if (document.hidden) {
        showRefreshing();
        delayedGetNextEvent = getNextEvent;
      } else {
        setTimeout(getNextEvent, 500);
      }
    })
    .fail(function() {
      showRefreshing();
      var duration = tryAgainTimeoutSeconds - Math.max(0, new Date().getSeconds() - requestedAt.getSeconds());
      setTimeout(function() { get(lastEventId); }, duration * 1000);
    });
  }

  function showRefreshing() {
    refreshElem.style.visibility = "inherit";
    widgetJq.addClass('OrderStatistics-error');
  }

  get(0);
}
