/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

cnt = 0;

public boolean spooler_init() {
  cnt = 0;
  System.out.println("spooler_init is called by " + name);
  return true;
}

public boolean spooler_process() {
  if (cnt < 5) {
    cnt++;
    System.out.println("spooler_process is called by " + name);
    return true;
  }
  return false;
}

public boolean spooler_exit() {
    System.out.println("spooler_exit is called by " + name);
  return true;
}
