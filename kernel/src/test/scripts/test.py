def scheduler_init():
  global cnt, name, log
  cnt = 0
  log.debug('START of Test ------------------------------------------------');
  log.debug('spooler_init is called by ' + name )
  return True;

def scheduler_process():
  global cnt
  result = False
  if cnt < 5:
    cnt = cnt + 1
    log.debug('scheduler_process: iteration no ' +  str(cnt) )
    result = True;
  return result;

def scheduler_exit():
  global log
  log.debug('END of Test --------------------------------------------------');
  return True;
