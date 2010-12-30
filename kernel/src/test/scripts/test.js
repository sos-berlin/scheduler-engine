var cnt;

function spooler_init() {
  cnt = 0;
  print("hello " + name + "!\n");
  print("spooler_init is called.\n");
}

function spooler_process() {
  if (cnt < 5) {
    cnt++;
    print("spooler_process: iteration no " +  cnt + "\n");
    return true;
  }
  return false;
}