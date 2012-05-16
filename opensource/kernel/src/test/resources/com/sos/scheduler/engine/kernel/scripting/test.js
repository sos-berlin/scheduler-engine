var cnt;

function spooler_init() {
  cnt = 0;
  print("spooler_init is called by " + name + "\n");
}

function spooler_process() {
  if (cnt < 5) {
    cnt++;
    print("spooler_process is called by " + name + "\n");
    return true;
  }
  return false;
}

function spooler_exit() {
   print("spooler_exit is called by " + name + "\n");
  return true;
}
