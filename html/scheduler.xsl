<?xml version='1.0'?>
<!-- $Id -->
<xsl:stylesheet xmlns:xsl   = "http://www.w3.org/1999/XSL/Transform" 
                xmlns:msxsl = "urn:schemas-microsoft-com:xslt"
                xmlns:my    = "http://sos-berlin.com/scheduler/mynamespace"
                version     = "1.0">

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Gesamtsicht-->

    <xsl:template match="/spooler/answer">
        <xsl:variable name="now" select="string( /spooler/answer/@time )"/>
        
        
        <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Scheduler-Info-->

        <table width="100%" cellpadding="0" cellspacing="0">
            <col class="column1" valign="baseline" align="left"/>
            <col class="column"  valign="baseline" align="left"/>
            <col class="column"  valign="baseline" align="left"/>
            <tr>
                <td colspan="2" align="left">
                    <b>
                        Scheduler
                        <xsl:text>&#160; </xsl:text>
                        <xsl:if test="state/@id!=''">id=
                            <xsl:value-of select="state/@id"/>
                        </xsl:if>
                    </b>
                    <xsl:value-of select="state/@state"/>    
                    <br/>
                    <span style="font-size: 8pt"><xsl:value-of select="state/@version"/></span>
                </td>
                <td align="right">
                    <xsl:value-of select="my:format_datetime( string( state/@time ) )"  disable-output-escaping="yes"/>
                    <span class="small">
                        &#160;
                        (<xsl:value-of select="my:datetime_diff( string( state/@spooler_running_since ), $now )"  disable-output-escaping="yes"/>)
                    </span>
                    <br/>
                    <xsl:call-template name="command_menu">
                        <xsl:with-param name="onclick" select="'scheduler_command__onclick()'"/>
                    </xsl:call-template>
                </td>
            </tr>
        </table>
        <p>&#160;</p>
        
        <xsl:apply-templates select="state/jobs"/>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Jobs-->

    <xsl:template match="jobs">
        <xsl:variable name="now" select="string( /spooler/answer/@time )"/>

        <table cellpadding="0" cellspacing="0">
            <col class="column1" valign="baseline"  width=" 90"/>
            <!--col class="column"  valign="baseline"  width="110"/-->  
            <col class="column"  valign="baseline"  width=" 50"  align="right"/>  
            <col class="column"  valign="baseline"  width=" 30"  align="right"/>
            <col class="column"  valign="baseline"  width=" 70"/>
            <col class="column"  valign="baseline"  width="130"/>
            
            <thead class="job">
                <tr>
                    <td>Job</td>
                    <td>Time</td>
                    <td>Steps</td>
                    <td colspan="2">State,&#160;Order</td>
                </tr>
                <tr>
                    <td colspan="99">
                        <hr style="color: black" size="1"/>
                    </td>
                </tr>
            </thead>
            
            <tbody>
                <xsl:for-each select="job">
                
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
                                <xsl:value-of select="@state_text"/>
                            </xsl:if>
                        </td>
                    </xsl:element>
                    
                    <xsl:element name="tr">
                        <xsl:attribute name="id"   >scheduler_tr_job_<xsl:value-of select="@job"/>__2</xsl:attribute>
                        <xsl:attribute name="class">job         </xsl:attribute>
                        <xsl:attribute name="style">cursor: hand</xsl:attribute>
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
                            <!--xsl:if test="@waiting_for_process='yes'">
                                <span class="waiting_for_process">waiting for process!</span>
                            </xsl:if-->
                        </td>
                        
                        <td>
                        </td>
                        
                        <td align="right">
                            <xsl:value-of select="@all_steps"/>
                        </td>
                        
                        <td>
                            <xsl:value-of select="@state"/>
                        </td>
                        
                        <xsl:choose>
                            <xsl:when test="@order='yes'">
                                <td>
                                    <xsl:value-of select="order_queue/@length"/> orders to process
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td>
                                </td>
                            </xsl:otherwise>
                        </xsl:choose>
                        
                    </xsl:element>
                    
                    <xsl:if test="ERROR">
                        <tr class="job">
                            <td colspan="99" class="error">
                                <xsl:apply-templates select="ERROR"/>
                            </td>
                        </tr>
                    </xsl:if>

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

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Tasks (in Jobs)-->

    <xsl:template match="tasks" mode="list">
        <xsl:variable name="now" select="string( /spooler/answer/@time )"/>

        <xsl:for-each select="task">
            <xsl:element name="tr">
                <xsl:attribute name="class">task</xsl:attribute>
                <xsl:choose>
                    <xsl:when test=" not( @id ) ">
                        <td colspan="3">
                            <span style="margin-left: 2ex">
                                <xsl:choose>
                                    <xsl:when test="../../@waiting_for_process='yes'">
                                        Waiting for process...
                                    </xsl:when>
                                    <xsl:otherwise>
                                        .
                                    </xsl:otherwise>
                                </xsl:choose>
                            </span>
                        </td>
                        
                        <xsl:choose>
                            <xsl:when test="../../@order='yes'">
                                <td colspan="2" class="order">
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td colspan="2">
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
                                    Task&#160;<xsl:value-of select="@id"/>
                                </span>
                                <xsl:if test="@name!=''">
                                    &#160; <xsl:value-of select="@name"/>
                                </xsl:if>
                            </b>
                        </td>
<!--                        
                        <td>
                            <xsl:choose>
                                <xsl:when test="@state='running_waiting_for_order'">
                                    running
                                </xsl:when>
                                <xsl:otherwise>
                                    <xsl:value-of select="@state"/>
                                </xsl:otherwise>
                            </xsl:choose>
                        </td>
-->                        
                        <td>
                            <xsl:if test="@running_since!=''">
                                <xsl:text> &#160;</xsl:text>
                                <!--span class="small"-->
                                    <xsl:value-of select="my:datetime_diff( string( @running_since ), $now, 0 )"  disable-output-escaping="yes"/>   
                                <!--/span-->
                            </xsl:if>
                        </td>
                        
                        <td align="right">
                            <xsl:value-of select="@steps"/>
                        </td>
                        
                        
                        <xsl:choose>
                            <xsl:when test="../../@order='yes'">
                                <td colspan="2" class="order">
                                    <xsl:if test="@state!='running'">
                                        <xsl:value-of select="@state"/>
                                        <xsl:text> </xsl:text>
                                    </xsl:if>
                                    <xsl:if test="order">
                                        <b>
                                            <xsl:value-of select="order/@id"/>
                                            <xsl:if test="order/@title != ''">
                                                &#160;
                                                <xsl:value-of select="order/@title"/>
                                            </xsl:if>
                                        </b>
                                    </xsl:if>
                                    <!--xsl:if test="@state='running_waiting_for_order'">
                                        waiting for order
                                    </xsl:if-->
                                    <xsl:if test="@in_process_since!=''">
                                        <xsl:text> &#160;</xsl:text>
                                        <span class="small">
                                            (<xsl:value-of select="my:datetime_diff( string( @in_process_since ), $now, 0 )"  disable-output-escaping="yes"/>)
                                        </span>
                                    </xsl:if>
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td colspan="2">
                                    <xsl:value-of select="@state"/>
                                    <xsl:text> </xsl:text>
                                    <xsl:if test="@in_process_since!=''">
                                        <span class="small">
                                            (<xsl:value-of select="my:datetime_diff( string( @in_process_since ), $now, 0 )"  disable-output-escaping="yes"/>)
                                        </span>
                                    </xsl:if>
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
        
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Detailsicht eines Jobs-->
    
    <xsl:template match="job">
        <xsl:variable name="now" select="string( /spooler/answer/@time )"/>
        
        <table cellpadding="0" cellspacing="0" width="100%" class="job">
            <col class="column1" valign="baseline" align="left" width="1"/>
            <col class="column"  valign="baseline" align="left" width="*"  />  

            <tr>
                <td colspan="2">
                    <table cellpadding="0" cellspacing="0" width="100%">
                        <tr>
                            <td>
                                <b>
                                    Job
                                    <xsl:value-of select="@job"/>
                                </b>
                                <xsl:if test="@title">
                                    <xsl:text> &#160; </xsl:text><xsl:value-of select="@title"/>
                                </xsl:if>
                            </td>
                            
                            <td align="right">
                                <xsl:call-template name="command_menu">
                                    <xsl:with-param name="onclick" select="concat( 'job_command__onclick(&quot;', @job, '&quot;)' )"/>
                                </xsl:call-template>
<!--                                
                                <xsl:element name="span">
                                    <xsl:attribute name="style">
                                        cursor: hand; text-decoration: underline; padding-left: 4pt
                                    </xsl:attribute>
                                    <xsl:attribute name="onclick">
                                        job_command__onclick('<xsl:value-of select="@job"/>')
                                    </xsl:attribute>
                                    Command
                                </xsl:element>
-->                                
                            </td>
                        </tr>
                    </table>
                </td>                
            </tr>
            
            <tr><td>&#160;</td></tr>
                
            <tr>
                <td><span class="label">Description:</span></td>
                <td>
                    <xsl:value-of select="description" disable-output-escaping="yes"/>
                </td>
            </tr>
                
            <tr>
                <td><span class="label">State:</span></td>
                <td>
                    <xsl:value-of select="@state"/>
                    <!--xsl:if test="@waiting_for_process='yes'">
                        <xsl:text>,</xsl:text>
                        <span class="waiting_for_process"> waiting for process!</span>
                    </xsl:if-->
                    <xsl:if test="@state_text">
                        <xsl:text> - </xsl:text>
                        <xsl:value-of select="@state_text"/>
                    </xsl:if>
                </td>
            </tr>

            <tr>
                <td><span class="label">Error:</span></td>
                <xsl:choose>
                    <xsl:when test="ERROR">
                        <td class="error">
                            <xsl:apply-templates select="ERROR"/>
                        </td>
                    </xsl:when>
                    <xsl:otherwise>
                        <td></td>
                    </xsl:otherwise>
                </xsl:choose>
            </tr>
                
            <tr>
                <td><span class="label">Next start time:</span></td>
                <td>
                    <xsl:value-of select="my:format_datetime_with_diff( string( @next_start_time), $now, true )"  disable-output-escaping="yes"/>
                </td>
            </tr>

            <tr>
                <td><span class="label">Steps:</span></td>
                <td><xsl:value-of select="@all_steps"/></td>
            </tr>

            <tr>
                <td><span class="label">Orders:</span></td>
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
        <xsl:apply-templates select="queued_tasks" mode="list"/>
        
        
        <p style="margin-top: 5ex; margin-bottom: 3ex"></p>
        <xsl:apply-templates select="order_queue" mode="list"/>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Detailsicht eines Task-->
    
    <xsl:template match="task">
        <xsl:variable name="now" select="string( /spooler/answer/@time )"/>
    
        <table cellpadding="0" cellspacing="0" class="task" width="100%" >
            <col class="column1" valign="baseline" align="left"  width="1"/>
            <col class="column"  valign="baseline" align="left"  />  

            <tr>
                <td colspan="2">
                    <table cellpadding="0" cellspacing="0" width="100%">
                        <tr>
                            <td>
                                <b>Task&#160;</b>
                                
                                <xsl:choose>
                                    <xsl:when test="not( @id )">
                                        <xsl:if test="../../@waiting_for_process='yes'">
                                            (waiting for process...)
                                        </xsl:if>
                                    </xsl:when>
                                    <xsl:otherwise>
                                        <b>
                                            <xsl:value-of select="@id"/>
                                        </b>
                                    </xsl:otherwise>
                                </xsl:choose>
                                
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

                            <xsl:if test="@id">
                                <td align="right">
                                    <xsl:call-template name="command_menu">
                                        <xsl:with-param name="onclick" select="concat( 'task_command__onclick(', @id, ')' )"/>
                                    </xsl:call-template>
<!--                               
                                    <xsl:element name="span">
                                        <xsl:attribute name="style">
                                            cursor: hand; text-decoration: underline; padding-left: 4pt
                                        </xsl:attribute>
                                        <xsl:attribute name="onclick">
                                            task_command__onclick(<xsl:value-of select="@id"/>)
                                        </xsl:attribute>
                                        Command
                                    </xsl:element>
-->                                    
                                </td>
                            </xsl:if>
                        </tr>
                    </table>
                </td>
                
            </tr>
            
            <tr><td><span style="line-height: 6px">&#160;</span>   </td></tr>
                
            <xsl:if test="order or ../../@order='yes'">
                <tr>
                    <td><span class="label">Order:</span></td>
                    <td class="order">
                        <b>
                            <xsl:value-of select="order/@id"/>
                            &#160;
                            <xsl:value-of select="order/@title"/>
                        </b>
                    </td>
                </tr>
            </xsl:if>

<!--
            <xsl:if test="@start_at">
                <tr>
                    <td>start at</td>
                    <td><xsl:value-of select="my:format_datetime_with_diff( string( @start_at ), $now, 0 )"  disable-output-escaping="yes"/></td>
                </tr>
            </xsl:if>
-->
            <xsl:choose>
                <xsl:when test="@idle_since">
                    <tr>
                        <td><span class="label">Idle since:</span></td>
                        <xsl:if test="@idle_since">
                            <td><xsl:value-of select="my:format_datetime_with_diff( string( @idle_since ), $now, 0 )"  disable-output-escaping="yes"/></td>
                        </xsl:if>
                    </tr>
                </xsl:when>
                <xsl:otherwise>
                    <tr>
                        <td><span class="label">In process since:</span></td>
                        <td><xsl:value-of select="my:format_datetime_with_diff( string( @in_process_since ), $now, 0 )"  disable-output-escaping="yes"/></td>
                    </tr>
                </xsl:otherwise>
            </xsl:choose>

            <tr>
                <!--xsl:choose>
                    <xsl:when test="@running_since"-->
                        <td><span class="label">Running since:</span></td>
                        <td>
                            <xsl:value-of select="my:format_datetime_with_diff( string( @running_since ), $now, 0 )"  disable-output-escaping="yes"/>
                        </td>
                    <!--/xsl:when>
                    <xsl:otherwise>
                        <td>&#160;</td>
                    </xsl:otherwise>
                </xsl:choose-->
            </tr>

            <xsl:if test="@enqueued">
                <tr>
                    <td><span class="label">Enqueued at:</span></td>
                    <td><xsl:value-of select="my:format_datetime_with_diff( string( @enqueued ), $now, 0 )"  disable-output-escaping="yes"/></td>
                </tr>
            </xsl:if>            

        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~queued_tasks-->

    <xsl:template match="queued_tasks" mode="list">
        <xsl:variable name="now" select="string( /spooler/answer/@time )"/>
        
        <table valign="top" cellpadding="0" cellspacing="0" width="100%" class="task_list">

            <caption class="task" align="left">
                <p class="column1" style="margin-top: 2px; margin-bottom: 1ex">
                    <b><xsl:value-of select="@length"/> enqueued tasks</b>
                </p>
            </caption>
            
            <xsl:if test="queued_task">
                <col class="column1"  valign="baseline" align="left" width="50"/>
                <col class="column"   valign="baseline" align="left" width="70"/>
                <col class="column"   valign="baseline" align="left" width="250"/>
                
                <thead>
                    <tr>
                        <td>Id</td>
                        <td>Enqueued</td>
                        <td>Start at</td>
                    </tr>
                    <tr>
                        <td colspan="99">
                            <hr size="1"/>
                        </td>
                    </tr>
                </thead>
                

                <xsl:for-each select="queued_task">
                    <tr>
                        <td>
                            <xsl:value-of select="@id"/>
                            <xsl:if test="@name!=''">
                                &#160; <xsl:value-of select="@name"/>
                            </xsl:if>
                        </td>

                        <td><xsl:value-of select="my:format_date_or_time      ( string( @enqueued )       )"  disable-output-escaping="yes"/></td>
                        <td><xsl:value-of select="my:format_datetime_with_diff( string( @start_at ), $now )"  disable-output-escaping="yes"/></td>
                    </tr>
                </xsl:for-each>
            </xsl:if>
        </table>
    </xsl:template>
        
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Order_queue-->

    <xsl:template match="order_queue" mode="list">
        <table class="order" cellpadding="0" cellspacing="0" width="100%">
            
            <caption align="left" class="order">
                <p class="column1" style="margin-top: 2px; margin-bottom: 1ex">
                    <b><xsl:value-of select="@length"/> orders</b>
                </p>
            </caption>
            
            <col class="column1" valign="baseline"  width=" 40"/>
            <!--col valign="top"  width=" 15"  style="padding-right: 2ex"/-->  
            <col class="column"  valign="baseline"  width=" 70"/>  
            <col class="column"  valign="baseline"  width=" 40"/>  
            <col class="column"  valign="baseline"  width="*"/>  
            <col class="column"  valign="baseline"  width="*"/>  
            
            <thead>
                <tr>
                    <td class="order_list">Id</td>
                    <!--td class="order_list">Pri</td-->
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
                        <!--td class="order_list"><xsl:value-of select="@priority"/></td-->
                        <td class="order_list"><xsl:value-of select="my:format_date_or_time( string( @created ) )"  disable-output-escaping="yes"/></td>
                        <td class="order_list"><xsl:value-of select="@state"/></td>
                        <td class="order_list"><xsl:value-of select="@state_text"/></td>
                        <td class="order_list"><xsl:value-of select="@title"/></td>
                    </xsl:element>
                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template> 

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ERROR-->

    <xsl:template match="ERROR">
        <span class="error">
            <xsl:value-of select="@time"/><br/>
            <xsl:value-of select="@text"/>
            <br/>
            <xsl:if test="@source">
                <xsl:text> </xsl:text>
                source <xsl:value-of select="@source"/>
            </xsl:if>
            <xsl:if test="@line">
                <xsl:text> </xsl:text>
                line <xsl:value-of select="@line"/>
            </xsl:if>
            <xsl:if test="@col">
                <xsl:text> </xsl:text>
                column <xsl:value-of select="@col"/>
            </xsl:if>
        </span>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_menu-->

    <xsl:template name="command_menu">
        <xsl:param name="onclick"/>
        <xsl:element name="span">
            <xsl:attribute name="style">
                cursor: hand; text-decoration: underline; padding-left: 4pt
            </xsl:attribute>
            <xsl:attribute name="onclick">
                <xsl:value-of select="$onclick"/>
            </xsl:attribute>
            Command
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~script-->


    <msxsl:script language="JavaScript" implements-prefix="my"><![CDATA[
    
        //--------------------------------------------------------------------------format_datetime
    
        function format_datetime( datetime ) 
        {
            var date = typeof datetime == "string"? date_from_datetime( datetime ) : datetime;
            if( !datetime )  return "";
            
            //var ms = date.getMilliseconds();

            return date.toLocaleDateString() + "&#160; " + date.toLocaleTimeString();
                   //+ ( ms? ".<span class='milliseconds'>" + ( ms + "000" ).substring( 0, 3 ) + "</span>" : "" );
        }

        //----------------------------------------------------------------------format_date_or_time
        
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

        //----------------------------------------------------------------------format_date_or_time
        
        function format_datetime_with_diff( datetime, now, show_plus )
        {
            var date = date_from_datetime( datetime );
            var result = format_datetime( date );
            if( result && now )  result += " &#160;(" + datetime_diff( date, now, show_plus ) + ")";
            
            return result;
        }
        
        //----------------------------------------------------------------------------datetime_diff
        
        function datetime_diff( datetime_earlier, datetime_later, show_plus ) 
        {
            var show_ms;
            if( show_ms   == undefined )  show_ms   = false;
            if( show_plus == undefined )  show_plus = false;
            
            var date_later   = typeof datetime_later   == "string"? date_from_datetime( datetime_later )   : datetime_later;
            var date_earlier = typeof datetime_earlier == "string"? date_from_datetime( datetime_earlier ) : datetime_earlier;

            if( !date_later   )  return "";
            if( !date_earlier )  return "";
            
            var diff = ( date_later.getTime() - date_earlier.getTime() ) / 1000.0;
            var abs  = Math.abs( diff );
            var result;

            if( abs < 60 )
            {
                if( show_ms ) 
                {
                    result = abs.toString();
                    if( result.match( "." ) )  result = result.replace( ".", ".<span class='milliseconds'>" ) + "</span>";
                }
                else
                {
                     result = Math.floor( abs );
                }
                result += "s";
            }
            else
            if( abs <    60*60 )  result = Math.floor( abs / (       60 ) ) + "min";
            else
            if( abs < 24*60*60 )  result = Math.floor( abs / (    60*60 ) ) + "h";
            else
                                  result = Math.floor( abs / ( 24*60*60 ) ) + "days";
                                  
            return diff < 0? "-" + result : 
                   show_plus && diff > 0? "+" + result
                           : result;
        }

        //-----------------------------------------------------------------------date_from_datetime
        
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

        //-----------------------------------------------------------------------------------------
        ]]>
    </msxsl:script>
    
</xsl:stylesheet>
