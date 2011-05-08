var cnt;

function scheduler_init() {
  cnt = 0;
  print("hello " + name + "!\n");
  print("spooler_init is called.\n");
}

function scheduler_process() {
  if (cnt < 5) {
    cnt++;
    print("scheduler_process: iteration no " +  cnt + "\n");
    return true;
  }
  return false;
}