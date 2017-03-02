var jocOrderStatisticsWidget = (function() {
  var widgetJq;
  var liveElem;
  var refreshElem;
  var tryAgainTimeoutSeconds = 1;
  var keys = [
    "total",
    "notPlanned",
    "planned",
    "due",
    "started",
    "inTask",
    "inTaskProcess",
    "waitingForResource",
    "setback",
    "suspended",
    "blacklisted",
    "permanent",
    "fileOrder"];
  var fields = {};
  var timestampValueDom;
  var current = {};
  var paused = false;
  var urlPrefix;
  var lastEventId = 0;
  var ajax = null;

  function start(path) {
    urlPrefix = "/jobscheduler/master/api/order" + path + "?return=JocOrderStatisticsChanged&timeout=60s&after=";
    widgetJq = $('#OrderStatistics');
    liveElem = document.getElementById('OrderStatistics-live');
    refreshElem = document.getElementById('OrderStatistics-refresh');
    var i, key;
    for (i in keys) if (keys.hasOwnProperty(i)) {
      key = keys[i];
      fields[key] = {
        fieldDom: document.getElementById('order-' + key + "-field"),
        valueDom: document.getElementById('order-' + key + "-value"),
        barDom: document.getElementById('order-' + key + "-bar")
      }
    }
    timestampValueDom = document.getElementById('order-timestamp-value');
    document.addEventListener("visibilitychange", documentVisibilityChanged, false);

    return {
      getNext: getNext,
      togglePause: togglePause
    }
  }

  function getNext() {
    var requestedAt = new Date();
    ajax = jQuery.ajax({
      dataType: 'json',
      url: urlPrefix + lastEventId
    });
    ajax.done(function(eventSeq) {
      ajax = null;
      if (!paused) {
        lastEventId = processEventSeq(eventSeq);
        if (!document.hidden) {
          setTimeout(getNext, 500);
        }
      }
      tryAgainTimeoutSeconds = 1;
    })
    .fail(function() {
      ajax = null;
      if (!paused) {
        showRefreshing();
        var duration = tryAgainTimeoutSeconds - Math.max(0, new Date().getSeconds() - requestedAt.getSeconds());
        setTimeout(getNext, duration * 1000);
        tryAgainTimeoutSeconds = Math.min(60, tryAgainTimeoutSeconds + 2);
      }
    });
  }

  function togglePause() {
    paused = !paused;
    if (paused) {
      hideRefreshing();
      widgetJq.addClass('OrderStatistics-paused');
    } else {
      widgetJq.removeClass('OrderStatistics-paused');
      if (!ajax) {
        showRefreshing();
        getNext();
      }
    }
  }

  function documentVisibilityChanged() {
    if (!paused && !document.hidden && !ajax) {
      getNext();
    }
  }

  function processEventSeq(eventSeq) {
    showNewEvent(eventSeq.eventId);
    switch (eventSeq.TYPE) {
      case "NonEmpty":
        var event = eventSeq.eventSnapshots[0];
        var orderStatistics = event.orderStatistics;
        showOrderStatistics(orderStatistics);
        current = orderStatistics;
        return event.eventId;
      case "Empty":  // Timed-out
        return eventSeq.lastEventId;
      case "Torn":
        return 0;
    }
  }

  function showNewEvent(eventId) {
    hideRefreshing();
    timestampValueDom.innerText = new Date(eventId / 1000).toTimeString().substring(0, 8);
  }

  function showOrderStatistics(orderStatistics) {
    for (var i in keys) if (keys.hasOwnProperty(i)) {
      var key = keys[i];
      if (orderStatistics[key] !== current[key]) {
        var field = fields[key];
        var total = orderStatistics.total;
        var value = orderStatistics[key];
        field.valueDom.innerText = value.toString();
        field.barDom.style.width = total === 0 ? "0%" : Math.floor(value / total * 100 + 0.5) + "%";
        if (typeof current[key] !== "undefined") {  // Not the first change?
          var style = field.fieldDom.style;
          // 'alt' alternates between 1 and 0 to force the animation to restart
          var alt = 1 - 1 * style.animationName.substring(style.animationName.length - 1);
          style.animationName = orderStatistics[key] > current[key]? 'OrderStatistics-higher-' + alt : 'OrderStatistics-lower-' + alt;
        }
      }
    }
  }

  function showRefreshing() {
    refreshElem.style.visibility = "inherit";
    liveElem.style.visibility = "hidden";
    refreshElem.style.animationIterationCount = "infinite";
    widgetJq.addClass('OrderStatistics-error');
  }

  function hideRefreshing() {
    refreshElem.style.visibility = "hidden";
    refreshElem.style.animationIterationCount = 0;  // Switch off CPU usage
    liveElem.style.visibility = "inherit";
    widgetJq.removeClass('OrderStatistics-error');
  }

  return (function() {
    var state;
    return {
      start: function(path) {
        state = start(path);
        state.getNext();
      },
      togglePause: function() {
        state.togglePause();
      }
    }
  })();
})();
