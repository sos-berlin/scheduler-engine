<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl   = "http://www.w3.org/1999/XSL/Transform" 
                xmlns:msxsl = "urn:schemas-microsoft-com:xslt"
                xmlns:my    = "http://sos-berlin.com/mynamespace"
                version="1.0">

    <xsl:template match="/spooler/answer">
        <table width="100%" cellpadding="0" cellspacing="0">
            <tr>
                <td align="left">
                    <b>
                        Scheduler
                        <xsl:text>&#160; </xsl:text>
                        <xsl:if test="state/@id!=''">id=
                            <xsl:value-of select="state/@id"/>
                        </xsl:if>
                    </b>
                    <br/>
                    <span style="font-size: 8pt"><xsl:value-of select="state/@version"/></span>
                </td>
                <td align="right">
                    <xsl:value-of select="my:format_datetime( string( state/@time ) )"  disable-output-escaping="yes"/>
                </td>
            </tr>
        </table>
        <p>&#160;</p>

        <table cellpadding="0" cellspacing="0">
            <col valign="top" align="left"  width="150" style="padding-right: 2ex; padding-bottom: 1px"/>
            <col valign="top" align="left"  width="100" style="padding-right: 2ex"/>  
            <col valign="top" align="right" width=" 70" style="padding-right: 2ex"/>
            <col valign="top" align="left"  width="200" style="padding-right: 2ex"/>
            
            <thead>
                <tr>
                    <td>Job</td>
                    <td>State</td>
                    <td>Steps</td>
                    <td>Order</td>
                </tr>
                <tr>
                    <td colspan="99" height="9">
                        <hr style="margin-top: 0; margin-bottom: 4px" size="1"/>
                    </td>
                </tr>
            </thead>
            
            <tbody>
                <xsl:for-each select="state/jobs/job">
                
                    <xsl:element name="tr">
                        <xsl:attribute name="id"         >scheduler_tr_job_<xsl:value-of select="@job"/></xsl:attribute>
                        <xsl:attribute name="class"      >job</xsl:attribute>
                        <xsl:attribute name="style"      >cursor: hand</xsl:attribute>
                        <xsl:attribute name="onmouseover">
                            this.className =
                            scheduler_tr_job_<xsl:value-of select="@job"/>__2.className = "job_list_hover"
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className =
                            scheduler_tr_job_<xsl:value-of select="@job"/>__2.className = "job_list"
                        </xsl:attribute>
                        <xsl:attribute name="onclick">show_job_details('<xsl:value-of select="@job"/>')</xsl:attribute>
                        <td colspan="99">
                            <b><xsl:value-of select="@job"/></b>
                            &#160;
                            <xsl:value-of select="@title"/>
                            <xsl:if test="@state_text!=''">
                                <xsl:text> &#160; - </xsl:text>
                                <i><xsl:value-of select="@state_text"/></i>
                            </xsl:if>
                        </td>
                    </xsl:element>
                    
                    <xsl:element name="tr">
                        <xsl:attribute name="id">scheduler_tr_job_<xsl:value-of select="@job"/>__2</xsl:attribute>
                        <xsl:attribute name="class"      >job</xsl:attribute>
                        <xsl:attribute name="style"      >cursor: hand</xsl:attribute>
                        <xsl:attribute name="onmouseover">
                            this.className =
                            scheduler_tr_job_<xsl:value-of select="@job"/>.className = "job_list_hover"
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className = 
                            scheduler_tr_job_<xsl:value-of select="@job"/>.className = "job_list"
                        </xsl:attribute>
                        <xsl:attribute name="onclick">show_job_details('<xsl:value-of select="@job"/>')</xsl:attribute>
                        
                        <td>
                            <xsl:if test="@waiting_for_process='yes'">
                                waiting for process
                            </xsl:if>
                        </td>
                        
                        <td>
                            <xsl:value-of select="@state"/>
                        </td>
                        
                        <td align="right">
                            <xsl:value-of select="@all_steps"/>
                        </td>
                        
                        <xsl:choose>
                            <xsl:when test="@order='yes'">
                                <td class="order">
                                    <xsl:value-of select="order_queue/@length"/> orders to process
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td>
                                </td>
                            </xsl:otherwise>
                        </xsl:choose>
                        
                    </xsl:element>

                    <xsl:if test="/spooler/@my_show_tasks='yes' and tasks/task">
                        <tr>
                            <td>
                            </td>
                            <td colspan="99">
                                <xsl:apply-templates select="tasks" mode="list"/>
                            </td>
                        </tr>
                    </xsl:if>
                    
                    <tr>
                        <td colspan="99">
                            <p style="margin-top: 0; margin-bottom: 0pt; line-height: 7px;">&#160;</p>
                        </td>
                    </tr>
                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template>

    <xsl:template match="tasks" mode="list">
        <xsl:for-each select="task">
            <xsl:element name="tr">
                <xsl:attribute name="class">task</xsl:attribute>
                <xsl:choose>
                    <xsl:when test=" not( @id ) ">
                        <td colspan="3"><span style="margin-left: 2ex">.</span></td>
                        <xsl:choose>
                            <xsl:when test="../../@order='yes'">
                                <td class="order">
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td>
                                </td>
                            </xsl:otherwise>
                        </xsl:choose>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:attribute name="style"      >cursor: hand</xsl:attribute>
                        <xsl:attribute name="onmouseover">this.className='task_list_hover'</xsl:attribute>
                        <xsl:attribute name="onmouseout" >this.className='task_list'</xsl:attribute>
                        <xsl:attribute name="onclick">show_task_details( '<xsl:value-of select="../../@job"/>', <xsl:value-of select="@id"/> )</xsl:attribute>
                        
                        <td>
                            <b>
                                <span style="margin-left: 2ex">
                                    Task <xsl:value-of select="@id"/>
                                </span>
                                <xsl:if test="@name!=''">
                                    &#160; <xsl:value-of select="@name"/>
                                </xsl:if>
                            </b>
                        </td>
                        
                        <td>
                            <xsl:value-of select="@state"/>
                        </td>
                        
                        <td>
                            <xsl:value-of select="@steps"/>
                        </td>
                        
                        <xsl:choose>
                            <xsl:when test="../../@order='yes'">
                                <td class="order">
                                    <xsl:if test="order">
                                        <xsl:value-of select="order/@id"/>
                                        <xsl:if test="order/@title != ''">
                                            &#160;
                                            <i><xsl:value-of select="order/@title"/></i>
                                        </xsl:if>
                                    </xsl:if>
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td>
                                </td>
                            </xsl:otherwise>
                        </xsl:choose>

    <!--                            
                        <td align="right">
                            <xsl:choose>
                                <xsl:when test="@running_since">
                                    started <xsl:value-of select="@start_at"/>
                                </xsl:when>
                                <xsl:when test="@start_at">
                                    start at <xsl:value-of select="@start_at"/>
                                </xsl:when>
                                <xsl:when test="@enqueued">
                                    enqueued <xsl:value-of select="@enqueued"/>
                                </xsl:when>
                                </xsl:choose>
                        </td>
                        <td>
                            <xsl:value-of select="@cause"/>
                        </td>
                        <td>
                            <xsl:value-of select="@idle_since"/>
                        </td>
                        <td>
                            <xsl:value-of select="@running_since"/>
                        </td>
                        <td>
                            <xsl:value-of select="@enqueued"/>
                        </td>
                        <td>
                            <xsl:value-of select="@in_process_since"/>
                        </td>
                        <td>
                            <xsl:value-of select="@calling"/>
                        </td>
                        <td>
                            <xsl:value-of select="@pid"/>
                        </td>
    -->                            
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:element>
        </xsl:for-each>
    </xsl:template>
        
    
    <xsl:template match="job">
        <table cellpadding="0" cellspacing="0" width="100%" class="job">
            <col valign="top" align="left" width="100" style="padding-right: 2ex; padding-bottom: 1pt" />
            <col valign="top" align="left" width="*"   style="padding-right: 2ex"/>  

            <tr>
                <td colspan="2">
                    <b>
                        Job
                        <xsl:value-of select="@job"/>
                        <xsl:if test="@title">
                            <xsl:text>&#160; </xsl:text><xsl:value-of select="@title"/>
                        </xsl:if>
                    </b>
                </td>
            </tr>
                
            <tr>
                <td>description</td>
                <td>
                    <xsl:value-of select="description" disable-output-escaping="yes"/>
                </td>
            </tr>
                
            <tr>
                <td>state</td>
                <td>
                    <xsl:value-of select="@state"/>
                    <xsl:if test="@waiting_for_process='yes'">
                        <xsl:text> </xsl:text>(waiting for process)
                    </xsl:if>
                </td>
            </tr>

            <tr>
                <td>state text</td>
                <td>
                    <i><xsl:value-of select="@state_text"/></i>
                </td>
            </tr>
            
            <tr>
                <td>error</td>
                <td>
                    <span style="color: red">
                        <xsl:value-of select="ERROR/@time"/>
                        <xsl:value-of select="ERROR/@text"/>
                        <xsl:if test="ERROR/@source">
                            <xsl:text> </xsl:text>
                            source <xsl:value-of select="ERROR/@source"/>
                        </xsl:if>
                        <xsl:if test="ERROR/@line">
                            <xsl:text> </xsl:text>
                            line <xsl:value-of select="ERROR/@line"/>
                        </xsl:if>
                        <xsl:if test="ERROR/@source">
                            <xsl:text> </xsl:text>
                            column <xsl:value-of select="ERROR/@col"/>
                        </xsl:if>
                    </span>
                </td>
            </tr>
                
            <tr>
                <td>next start time</td>
                <td><xsl:value-of select="@next_start_time"/></td>
            </tr>

            <tr>
                <td>steps</td>
                <td><xsl:value-of select="@all_steps"/></td>
            </tr>

            <tr>
                <td>orders</td>
                <td>
                    <xsl:choose>
                        <xsl:when test="@order!='yes'">
                            (no order job)
                        </xsl:when>
                        <xsl:when test="order_queue/@length!=''">
                            <xsl:value-of select="order_queue/@length"/> orders to process
                        </xsl:when>
                    </xsl:choose>
                </td>
            </tr>
            
        </table>

        
        <p>&#160; </p>
        
        <xsl:for-each select="tasks/task">
            <p> </p>
            <xsl:apply-templates select="."/>
        </xsl:for-each>


        <p style="margin-top: 5ex; margin-bottom: 3ex"></p>
            
        <span class="order_list">
            <xsl:apply-templates select="order_queue" mode="list"/>
        </span>        
        
    </xsl:template>

    
    <xsl:template match="task">
        <xsl:variable name="now" select="string( /spooler/answer/@time )"/>
    
        <table cellpadding="0" cellspacing="0" class="task" width="100%">
            <col valign="top" align="left"  width="120"  style="padding-right: 2ex; padding-bottom: 1pt" />
            <col valign="top" align="left"  width="*"    style="padding-right: 2ex"/>  

            <tr>
                <td colspan="2">
                    <b>
                        Task
                        <xsl:value-of select="@id"/>
                    </b>
                    
                    <xsl:if test="@name!=''">
                        <xsl:text>&#160; </xsl:text><xsl:value-of select="@name"/>
                    </xsl:if>
                    <xsl:if test="@pid">
                        <xsl:text>, pid </xsl:text>
                        <xsl:value-of select="@pid"/>
                    </xsl:if>
                    <xsl:if test="@cause!=''">
                        <xsl:text> (start&#160;cause:&#160;</xsl:text><xsl:value-of select="@cause"/><xsl:text>)</xsl:text>
                    </xsl:if>
                    <xsl:if test="@state!=''">
                        <xsl:text>, </xsl:text>
                        <xsl:value-of select="@state"/>
                    </xsl:if>
                    <xsl:if test="@calling">
                        <xsl:text> </xsl:text>(<xsl:value-of select="@calling"/>)
                    </xsl:if>
                    <xsl:if test="@steps!=''">
                        <xsl:text>, </xsl:text>
                        <xsl:value-of select="@steps"/> steps
                    </xsl:if>
                </td>
            </tr>
                
            <xsl:if test="order or ../../../job/@order='yes'">
                <tr>
                    <td>order</td>
                    <td class="order">
                        <xsl:value-of select="order/@id"/>
                        &#160;
                        <i><xsl:value-of select="order/@title"/></i>
                    </td>
                </tr>
            </xsl:if>

            <xsl:if test="@enqueued">
                <tr>
                    <td>enqueued at</td>
                    <td><xsl:value-of select="my:format_datetime_with_diff( string( @enqueued ), $now )"  disable-output-escaping="yes"/></td>
                </tr>
            </xsl:if>            

            <xsl:if test="@start_at">
                <tr>
                    <td>start at</td>
                    <td><xsl:value-of select="my:format_datetime_with_diff( string( @start_at ), $now )"  disable-output-escaping="yes"/></td>
                </tr>
            </xsl:if>

            <tr>
                <td>running since</td>
                <td>
                    <xsl:value-of select="my:format_datetime_with_diff( string( @running_since ), $now )"  disable-output-escaping="yes"/>
                </td>
            </tr>

            <xsl:if test="@in_process_since">
                <tr>
                    <td>in process since</td>
                    <td><xsl:value-of select="my:format_datetime_with_diff( string( @in_process_since ), $now )"  disable-output-escaping="yes"/></td>
                </tr>
            </xsl:if>

            <xsl:if test="@idle_since">
                <tr>
                    <td>idle since</td>
                    <xsl:if test="@idle_since">
                        <td><xsl:value-of select="my:format_datetime_with_diff( string( @idle_since ), $now )"  disable-output-escaping="yes"/></td>
                    </xsl:if>
                </tr>
            </xsl:if>

        </table>
    </xsl:template>


    <xsl:template match="order_queue" mode="list">
        <table cellpadding="0" cellspacing="0" width="100%">
            <caption class="order" align="left">
                <p style="margin-top: 0; margin-bottom: 1ex">
                    <b><xsl:value-of select="@length"/> Orders</b>
                </p>
            </caption>
            <col valign="top" align="left"  width=" 40"  style="padding-right: 2ex; padding-bottom: 1pt" />
            <col valign="top" align="left"  width=" 20"  style="padding-right: 2ex"/>  
            <col valign="top" align="left"  width=" 70"  style="padding-right: 2ex"/>  
            <col valign="top" align="left"  width=" 50"  style="padding-right: 2ex"/>  
            <col valign="top" align="left"  width="*"    style="padding-right: 2ex"/>  
            <col valign="top" align="left"  width="*"    style="padding-right: 2ex"/>  
            
            <thead>
                <tr>
                    <td class="order_list">Id</td>
                    <td class="order_list">Pri</td>
                    <td class="order_list">Created</td>
                    <td class="order_list">State</td>
                    <td class="order_list">State text</td>
                    <td class="order_list">Title</td>
                </tr>
                <tr>
                    <td colspan="99"><hr size="1"/></td>
                </tr>
            </thead>
            
            <tbody>
                <xsl:for-each select="order">
                    <xsl:element name="tr">
                        <xsl:if test="@task">
                            <xsl:attribute name="style">font-weight: bold</xsl:attribute>
                        </xsl:if>                
                        
                        <td class="order_list"><xsl:value-of select="@id"/></td>
                        <td class="order_list"><xsl:value-of select="@priority"/></td>
                        <td class="order_list"><xsl:value-of select="my:format_date_or_time( string( @created ) )"  disable-output-escaping="yes"/></td>
                        <td class="order_list"><xsl:value-of select="@state"/></td>
                        <td class="order_list"><i><xsl:value-of select="@state_text"/></i></td>
                        <td class="order_list"><i><xsl:value-of select="@title"/></i></td>
                    </xsl:element>
                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template> 


    <msxsl:script language="JavaScript" implements-prefix="my"><![CDATA[
    
        function format_datetime( datetime ) 
        {
            var date = typeof datetime == "string"? date_from_datetime( datetime ) : datetime;
            if( !datetime )  return "";
            
            var ms = date.getMilliseconds();

            return date.toLocaleDateString() + "&#160; " + date.toLocaleTimeString() +
                   ( ms? ".<span class='milliseconds'>" + ( ms + "000" ).substring( 0, 3 ) + "</span>" : "" );
        }

        
        function format_date_or_time( datetime ) 
        {
            if( !datetime )  return "";
            
            var now = new Date();
            
            if(  1*datetime.substring( 0,  4 ) == now.getYear()
              && 1*datetime.substring( 5,  7 ) == now.getMonth() + 1
              && 1*datetime.substring( 8, 10 ) == now.getDate()  )
            {
                return datetime.substring( 11, 19 );
            }
            else
            {
                return datetime.substring( 0, 10 );
            }
        }

        
        function format_datetime_with_diff( datetime, now )
        {
            var date = date_from_datetime( datetime );
            var result = format_datetime( date );
            if( result && now )  result += " (" + diff_datetime( now, date ) + ")";
            
            return result;
        }
        
        
        function diff_datetime( datetime_later, datetime_earlier ) 
        {
            var date_later   = typeof datetime_later   == "string"? date_from_datetime( datetime_later )   : datetime_later;
            var date_earlier = typeof datetime_earlier == "string"? date_from_datetime( datetime_earlier ) : datetime_earlier;

            if( !date_later   )  return "";
            if( !date_earlier )  return "";
            
            var diff = ( date_later.getTime() - date_earlier.getTime() ) / 1000.0;

            if( diff <       60 )
            {
                var result = diff.toString();
                if( result.match( "." ) )  result = result.replace( ".", ".<span class='milliseconds'>" ) + "</span>";
                return result + "s";
            }
            if( diff <    60*60 )  return Math.floor( diff / (       60 ) ) + "min";
            if( diff < 24*60*60 )  return Math.floor( diff / (    60*60 ) ) + "h";
                                   return Math.floor( diff / ( 24*60*60 ) ) + "days";
        }


        function date_from_datetime( datetime ) 
        {
            if( !datetime )  return null;
            
            var date = new Date();
            
            date.setFullYear    ( 1*datetime.substring( 0, 4 ), 1*datetime.substring( 5, 7 ), 1*datetime.substring( 8, 10 ) );
            date.setHours       ( 1*datetime.substring( 11, 13 ) );
            date.setMinutes     ( 1*datetime.substring( 14, 16 ) );
            date.setSeconds     ( 1*datetime.substring( 17, 19 ) );
            date.setMilliseconds( datetime.length < 23? 0 : 1*datetime.substring( 20, 23 ) );
            
            return date;
        }

        ]]>
    </msxsl:script>
    
</xsl:stylesheet>
