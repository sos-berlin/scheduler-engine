jQuery(function() {
  var totalElem = document.getElementById('order-total')
  var unplannedElem = document.getElementById('order-unplanned');
  var plannedOrPendingElem = document.getElementById('order-plannedOrPending');
  var runningElem = document.getElementById('order-running');
  var inTaskElem = document.getElementById('order-inTask');
  var inProcessElem = document.getElementById('order-inProcess');
  var setbackElem = document.getElementById('order-setback');
  var suspendedElem = document.getElementById('order-suspended');
  var blacklistedElem = document.getElementById('order-blacklisted');
  var permanentElem = document.getElementById('order-permanent');
  var fileOrderElem = document.getElementById('order-fileOrder');
  var current = {}

  function get(lastEventId) {
    jQuery.ajax({
      dataType: 'json',
      url: "/jobscheduler/master/api/event/?return=OrderStatisticsChanged&after=" + lastEventId,
      cache: false
    })
    .done(function(snapshot) {
      var events = snapshot.schedulerResponseContent
      var event = events[0]
      var stat = event.orderStatistics;
      if (stat.total != current.total) {
        totalElem.innerText = stat.total;
      }
      if (stat.unplanned != current.unplanned) {
        unplannedElem.innerText = stat.unplanned;
      }
      if (stat.planned + stat.pending != current.planned + current.pending) {
        plannedOrPendingElem.innerText = stat.planned + stat.pending;  // OrderStatisticsChanged does not trigger on difference between Planned and Pending
      }
      if (stat.running != current.running) {
        runningElem.innerText =stat.running;
      }
      if (stat.inTask != current.inTask) {
        inTaskElem.innerText = stat.inTask;
      }
      if (stat.inProcess != current.inProcess) {
        inProcessElem.innerText = stat.inProcess;
      }
      if (stat.setback != current.setback) {
        setbackElem.innerText = stat.setback;
      }
      if (stat.suspended != current.suspended) {
        suspendedElem.innerText = stat.suspended;
      }
      if (stat.blacklisted != current.blacklisted) {
        blacklistedElem.innerText = stat.blacklisted;
      }
      if (stat.permanent != current.permanent) {
        permanentElem.innerText = stat.permanent;
      }
      if (stat.fileOrder != current.fileOrder) {
        fileOrderElem.innerText = stat.fileOrder;
      }
      current = stat
      setTimeout(function() { get(event.eventId); }, 250)
    })
    .fail(function() {
      setTimeout(function() { get(lastEventId); }, 5000);
    });
  }

  get(0);
});
