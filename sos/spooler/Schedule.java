// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

package sos.spooler;

/** 
 * @author Joacim Zschimmer
 * @version $Revision$
 */

public class Schedule  extends Idispatch
{
    private                 Schedule            ( long idispatch )                  { super(idispatch); }

    
    
    /*+ Setzt die Run_time oder erweitert sie.
     *  
     * @param xml Ein &lt;run_time>-Dokument.
     */
    
    public void         set_xml                 ( String xml )                      {                   com_call( ">xml", xml           ); }
    public String           xml                 ()                                  { return (String)   com_call( "<xml"                ); }
}
