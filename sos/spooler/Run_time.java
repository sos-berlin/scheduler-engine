// $Id$

package sos.spooler;

/** &lt;run_time>-Element.
 * 
 * @see Order#run_time()
 *
 * @author Joacim Zschimmer
 * @version $Revision$
 */

public class Run_time  extends Idispatch
{
    private                 Run_time            ( long idispatch )                  { super(idispatch); }

    
    
    /** Setzt die Run_time oder erweitert sie.
     *  
     * @param xml Ein &lt;run_time>-Dokument.
     */
    
    public void         set_xml                 ( String xml )                      {                   com_call( ">xml", xml           ); }
}
