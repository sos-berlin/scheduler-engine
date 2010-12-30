public cnt;

public boolean spooler_init() {
  cnt = 0;
    System.out.println("START of Test ------------------------------------------------");
  System.out.println("spooler_init is called by " + name);
  return true;
}

public boolean spooler_process() {
  if (cnt < 5) {
    cnt++;
    System.out.println("spooler_process: iteration no " +  cnt);
    return true;
  }
  return false;
}

public boolean spooler_exit() {
  System.out.println("END of Test --------------------------------------------------");
  return true;
}
