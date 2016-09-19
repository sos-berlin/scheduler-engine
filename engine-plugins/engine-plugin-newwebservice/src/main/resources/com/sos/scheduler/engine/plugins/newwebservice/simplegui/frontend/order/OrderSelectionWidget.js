function orderQueryToUrl(query, change) {
  var key, v;
  var q = [];
  for (key in change) if (change.hasOwnProperty(key)) {
    v = change[key]
    if (typeof v == 'undefined') delete query[key]; else query[key] = v;
  }
  var notInTaskLimitPerNode = document.getElementsByName('notInTaskLimitPerNode')[0].value;
  if (typeof notInTaskLimitPerNode == 'undefined' || notInTaskLimitPerNode == '') {
    delete query['notInTaskLimitPerNode'];
  } else {
    query.notInTaskLimitPerNode = notInTaskLimitPerNode;
  }
  for (key in query) if (query.hasOwnProperty(key))
    q.push(key + '=' + query[key].toString());
  var href = window.location.href.replace(/[?].*/g, '');
  if (q.length) href += "?" + q.join('&');
  return href;
}

function selectionToKeyValue(key, names) {
  var name, i;
  var selected = [];
  for (i = 0; i < names.length; i++) {
    name = names[i];
    if (document.getElementsByName(key + "-" + name)[0].checked) {
      selected.push(name);
    }
  }
  var result = {};
  // Revert meaning when nothing is checked: All is accepted
  result[key] = selected.length == 0 ? undefined : selected.join(",");
  return result;
}
