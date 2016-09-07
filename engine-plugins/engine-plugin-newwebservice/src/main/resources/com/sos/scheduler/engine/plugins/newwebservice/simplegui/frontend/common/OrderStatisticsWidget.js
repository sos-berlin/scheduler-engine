jQuery(function() {
  var keys = [
    "total",
    "notPlanned",
    "planned",
    "pending",
    "running",
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
    jQuery.ajax({
      dataType: 'json',
      url: "/jobscheduler/master/api/event/?return=OrderStatisticsChanged&after=" + lastEventId
    })
    .done(function(snapshot) {
      refreshElem.style.visibility = "hidden";
      widgetJq.removeClass('OrderStatistics-error');
      var events = snapshot.elements;
      timestampValueDom.innerText = new Date(snapshot.eventId / 1000).toTimeString().substring(0, 8);
      var event = events[0];
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
      function getNextEvent() { get(event.eventId); }
      if (document.hidden) {
        showRefreshing();
        delayedGetNextEvent = getNextEvent;
      } else {
        setTimeout(getNextEvent, 500);
      }
    })
    .fail(function() {
      showRefreshing();
      setTimeout(function() { get(lastEventId); }, 5000);
    });
  }

  function showRefreshing() {
    refreshElem.style.visibility = "inherit";
    widgetJq.addClass('OrderStatistics-error');
  }

  get(0);
});
