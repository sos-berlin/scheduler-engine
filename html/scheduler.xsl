<?xml version='1.0'?>
<!-- $Id -->
<xsl:stylesheet xmlns:xsl   = "http://www.w3.org/1999/XSL/Transform" 
                xmlns:msxsl = "urn:schemas-microsoft-com:xslt"
                xmlns:my    = "http://sos-berlin.com/scheduler/mynamespace"
                version     = "1.0">

    <xsl:variable name="now" select="string( /spooler/answer/@time )"/>
    <xsl:variable name="datetime_column_width" select="100"/>    <!-- 250 für langes Format, toLocaleDateString() -->
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Gesamtsicht-->

    <xsl:template match="/spooler/answer">
        
        <xsl:call-template name="scheduler_info"/>
        

        <!-- Jobs oder Jobketten zeigen? -->
        <!-- Hier wären Karteikartenreiter schön ... -->
        
        <p style="margin-bottom: 2ex; margin-top: 3ex">
            <span style="cursor: hand" 
                  onmouseover="this.className='hover' "
                  onmouseout ="this.className='' "
                  onclick    ="show_card( 'jobs' )">
                  
                <xsl:element name="span">
                    <xsl:if test="/spooler/@my_show_card='jobs' ">
                        <xsl:attribute name="class">job</xsl:attribute>
                        <xsl:attribute name="style">font-weight: bold;</xsl:attribute>
                    </xsl:if>
                    Jobs
                </xsl:element>
            </span>
            &#160;
            
            <span style="cursor: hand" 
                  onmouseover="this.className='hover' "
                  onmouseout ="this.className='' "
                  onclick    ="show_card( 'job_chains' )">
                  
                <xsl:element name="span">
                    <xsl:if test="/spooler/@my_show_card='job_chains'">
                        <xsl:attribute name="class">job_chain</xsl:attribute>
                        <xsl:attribute name="style">font-weight: bold;</xsl:attribute>
                    </xsl:if>
                    Job chains
                </xsl:element>
            </span>
            &#160;
            
            <span style="cursor: hand" 
                  onmouseover="this.className='hover' "
                  onmouseout ="this.className='' "
                  onclick    ="show_card( 'process_classes' )">
                  
                <xsl:element name="span">
                    <xsl:if test="/spooler/@my_show_card='process_classes'">
                        <xsl:attribute name="class">process_class</xsl:attribute>
                        <xsl:attribute name="style">font-weight: bold;</xsl:attribute>
                    </xsl:if>
                    Process classes
                </xsl:element>
            </span>
        </p>


        <xsl:if test="/spooler/@my_show_card='jobs'">
            <xsl:apply-templates select="state/jobs"/>
        </xsl:if>        
        
        <xsl:if test="/spooler/@my_show_card='job_chains'">
            <xsl:apply-templates select="state/job_chains"/>
        </xsl:if>            
        
        <xsl:if test="/spooler/@my_show_card='process_classes'">
            <xsl:apply-templates select="state/process_classes"/>
        </xsl:if>            
        
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Scheduler-Info-->

    <xsl:template name="scheduler_info">
        <table cellpadding="0" cellspacing="0" class="scheduler">
            <col valign="baseline" align="left"/>
            <col valign="baseline" align="right"/>
            <col valign="baseline" width="10"/>

            <tr>
                <td style="padding-right: 3ex">
                    <span style="margin-top: 2px; margin-bottom: 2pt">
                    
                        <xsl:element name="span">
                            <xsl:attribute name="title">Version  <xsl:value-of select="state/@version"/>&#10;pid=<xsl:value-of select="state/@pid"/></xsl:attribute>
                            
                            <b>Scheduler</b>
                        </xsl:element>
                        
                        <!--xsl:text>&#160;</xsl:text>
                        <span style="font-size: 8pt; white-space: nowrap">(<xsl:value-of select="state/@version"/>)</span-->
    
                        <xsl:if test="state/@id!=''">
                            <xsl:text>&#160; </xsl:text>
                            <b style="white-space: nowrap"><xsl:value-of select="state/@id"/></b>
                        </xsl:if>

                        <xsl:text>&#160; </xsl:text>
                        <xsl:value-of select="state/@state"/>    
                    </span>
                </td>

                <td style="padding-left: 0">
                    <span style="margin-top: 2px; margin-bottom: 2px">
                        <xsl:value-of select="my:format_datetime( string( state/@time ) )"  disable-output-escaping="yes"/>
                        <span class="small">
                            <xsl:text> &#160;</xsl:text>
                            (<xsl:value-of select="my:datetime_diff( string( state/@spooler_running_since ), $now )"  disable-output-escaping="yes"/>)
                        </span>
                        <xsl:text> </xsl:text>
                    </span>
                </td>

                <td>
                    <xsl:call-template name="command_menu">
                        <xsl:with-param name="onclick" select="'scheduler_menu__onclick()'"/>
                    </xsl:call-template>
                </td>
            </tr>

            <tr>
                <td colspan="3">
                    <xsl:value-of select="count( state/jobs/job [ @state='running' ] )" /> jobs running,
                    <span style="white-space: nowrap">
                        <xsl:value-of select="count( state/jobs/job [ @state='stopped' ] )" /> stopped,
                    </span>
                    <xsl:text> </xsl:text>
                    <span style="white-space: nowrap">
                        <xsl:value-of select="count( state/jobs/job [ @waiting_for_process='yes' ] )" /> need process,
                    </span>
                    <xsl:text> </xsl:text>
                    <span style="white-space: nowrap">
                        <xsl:value-of select="count( state/jobs/job/tasks/task[ @id ] )" /> tasks,
                    </span>
                    <xsl:text> </xsl:text>
                    <span style="white-space: nowrap">
                        <xsl:value-of select="sum( state/jobs/job/order_queue/@length )" /> orders
                    </span>
                </td>
            </tr>
        </table>
    </xsl:template>
        
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Jobs-->

    <xsl:template match="jobs">
        <table cellpadding="0" cellspacing="0" class="job">
            <caption align="left" class="job">
                <!--b>Jobs</b>
                &#160;-->

                <!-- Checkbox für Show tasks -->                    
                <xsl:element name="input">
                    <xsl:attribute name="id"     >show_tasks_checkbox</xsl:attribute>
                    <xsl:attribute name="type"   >checkbox</xsl:attribute>
                    <xsl:attribute name="onclick">show_tasks_checkbox__onclick()</xsl:attribute>
                    <xsl:if test="/spooler/@show_tasks_checkbox">
                        <xsl:attribute name="checked">checked</xsl:attribute>
                    </xsl:if>
                </xsl:element>
                <label for="show_tasks_checkbox">Show tasks</label>
            </caption>
            
            <col valign="baseline"  width="100"/>
            <!--col  valign="baseline"  width="110"/-->  
            <col valign="baseline"  width=" 50"  align="right"/>  
            <col valign="baseline"  width=" 30"  align="right"/>
            <col valign="baseline"/>
            
            <thead>
                <tr style="">
                    <td class="head1">Job </td>
                    <td class="head"> Time </td>
                    <td class="head"> Steps </td>
                    <td class="head"> Order etc.</td>
                </tr>
                <tr>
                    <td colspan="99" class="after_head_space">&#160;</td>
                </tr>
            </thead>
            
            <tbody>
                <xsl:for-each select="job">
                
                    <xsl:element name="tr">
                        <xsl:attribute name="id"         >scheduler_tr_job_<xsl:value-of select="@job"/></xsl:attribute>
                        <xsl:attribute name="class"      >job</xsl:attribute>
                        <xsl:attribute name="style"      >cursor: hand; padding-top: 5pt</xsl:attribute>
                        <xsl:attribute name="onmouseover">
                            this.className =
                            scheduler_tr_job_<xsl:value-of select="@job"/>__2.className = "hover"
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className =
                            scheduler_tr_job_<xsl:value-of select="@job"/>__2.className = "job"
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

<!--
                        <td align="right">
                            <xsl:call-template name="command_menu">
                                <xsl:with-param name="onclick" select="concat( 'job_menu__onclick(&quot;', @job, '&quot;)' )"/>
                            </xsl:call-template>
                        </td>
-->                        
                    </xsl:element>
                    
                    <xsl:element name="tr">
                        <xsl:attribute name="id"   >scheduler_tr_job_<xsl:value-of select="@job"/>__2</xsl:attribute>
                        <xsl:attribute name="class">job         </xsl:attribute>
                        <xsl:attribute name="style">cursor: hand</xsl:attribute>
                        <xsl:attribute name="onmouseover">
                            this.className =
                            scheduler_tr_job_<xsl:value-of select="@job"/>.className = "hover"
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className = 
                            scheduler_tr_job_<xsl:value-of select="@job"/>.className = "job"
                        </xsl:attribute>
                        <xsl:attribute name="onclick">show_job_details('<xsl:value-of select="@job"/>')</xsl:attribute>
                        
                        <td colspan="2">
                            <xsl:value-of select="@state"/>
                            <xsl:if test="not( /spooler/@show_tasks_checkbox ) and tasks/@count>0">
                                <xsl:text>, </xsl:text>
                                <span style="white-space: nowrap">
                                    <xsl:value-of select="tasks/@count"/> tasks
                                </span>
                            </xsl:if>
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
                    
                    <xsl:if test="ERROR">
                        <tr class="job">
                            <td colspan="99" class="job_error">
                                <xsl:apply-templates select="ERROR"/>
                            </td>
                        </tr>
                    </xsl:if>

                    <xsl:if test="/spooler/@show_tasks_checkbox and tasks/task">
                        <xsl:apply-templates select="tasks" mode="job_list"/>
                    </xsl:if>
                    
                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Tasks (in Jobs)-->

    <xsl:template match="tasks" mode="job_list">
        <xsl:for-each select="task">
            <xsl:element name="tr">
                <xsl:attribute name="class">task</xsl:attribute>
                <xsl:choose>
                    <xsl:when test=" not( @id ) ">
                        <td colspan="1">
                            <span style="margin-left: 2ex">
                                <xsl:choose>
                                    <xsl:when test="../../@waiting_for_process='yes'">
                                        Needs process
                                    </xsl:when>
                                    <xsl:otherwise>
                                        <xsl:choose>
                                            <xsl:when test="../../@next_start_time">
                                                Next start
                                            </xsl:when>
                                            <xsl:otherwise>
                                                .
                                            </xsl:otherwise>
                                        </xsl:choose>
                                    </xsl:otherwise>
                                </xsl:choose>
                            </span>
                        </td>

                        <td>
                            <xsl:if test="../../@next_start_time">
                                <xsl:value-of select="my:datetime_diff( string( ../../@next_start_time), $now )"  disable-output-escaping="yes"/>
                            </xsl:if>
                        </td>
                        
                        <td></td>
                        
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
                        <xsl:attribute name="onmouseover">this.className='task_hover'</xsl:attribute>
                        <xsl:attribute name="onmouseout" >this.className='task'</xsl:attribute>
                        <xsl:attribute name="onclick">show_task_details( '<xsl:value-of select="../../@job"/>', <xsl:value-of select="@id"/> )</xsl:attribute>
                        
                        <td>
                            <span style="margin-left: 2ex">
                                Task&#160;<xsl:value-of select="@id"/>
                            </span>
                            <xsl:if test="@name!=''">
                                &#160; <xsl:value-of select="@name"/>
                            </xsl:if>
                        </td>

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
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:element>
        </xsl:for-each>
    </xsl:template>
        
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Job_chains-->

    <xsl:template match="job_chains">
        <xsl:call-template name="job_chains">
            <xsl:with-param name="job_chain_select" select="job_chain"/>
            <xsl:with-param name="single"           select="false()"/>
        </xsl:call-template>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Job_chain-->

    <xsl:template match="job_chain">
        <xsl:call-template name="job_chains">
            <xsl:with-param name="job_chain_select" select="."/>
            <xsl:with-param name="single"           select="true()"/>
        </xsl:call-template>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Job_chains-->

    <xsl:template name="job_chains">
        <xsl:param    name="job_chain_select"/> 
        <xsl:param    name="single"             select="false()"/>                              <!-- false: Mehrere im linken Frame, 
                                                                                                     true:  Eine Jobkette im rechten frame -->
        <xsl:variable name="max_orders">
            <xsl:choose>
                <xsl:when test="$single">
                    999999999
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="/spooler/@my_max_orders"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>


        <table cellpadding="0" cellspacing="0">
            <caption align="left" class="job_chain">
                    <!--
                    <xsl:choose>
                        <xsl:when test="not( $single )">
                            <!- -b>Job chains</b- ->
                        </xsl:when>
                        <xsl:otherwise>
                            <!- -<b>Job chain &#160;<xsl:value-of select="@name"/></b>- ->
                        </xsl:otherwise>
                    </xsl:choose>
                    &#160;
                    -->

                    <xsl:if test="not( $single )">
                        <!-- Checkbox für Show jobs-->
                        <xsl:element name="input">
                            <xsl:attribute name="id"     >show_job_chain_jobs_checkbox</xsl:attribute>
                            <xsl:attribute name="type"   >checkbox</xsl:attribute>
                            <xsl:attribute name="onclick">show_job_chain_jobs_checkbox__onclick()</xsl:attribute>
                            <xsl:if test="/spooler/@show_job_chain_jobs_checkbox">
                                <xsl:attribute name="checked">checked</xsl:attribute>
                            </xsl:if>
                        </xsl:element>
                        <label for="show_job_chain_jobs_checkbox">Show jobs</label>

                        <!-- Checkbox für Show orders-->
                        &#160;
                        <xsl:element name="input">
                            <xsl:attribute name="id"     >show_job_chain_orders_checkbox</xsl:attribute>
                            <xsl:attribute name="type"   >checkbox</xsl:attribute>
                            <xsl:attribute name="onclick">show_job_chain_orders_checkbox__onclick()</xsl:attribute>
                            <xsl:if test="/spooler/@show_job_chain_orders_checkbox or $single">
                                <xsl:attribute name="checked">checked</xsl:attribute>
                            </xsl:if>
                        </xsl:element>
                        <label for="show_job_chain_orders_checkbox">Show orders</label>
                    </xsl:if>                    
            </caption>
            
            <col valign="baseline"  width=" 50"/>
            <col valign="baseline"  width="100"/>
            <col valign="baseline"  width="150"/>
            <col valign="baseline"  width="40" align="right"/>
            <col valign="baseline"  width="10" align="right"/>
            
            <thead class="job_chain">
                <tr>
                    <td class="head1">
                        <span style="margin-left: 2ex">
                            State
                        </span>
                    </td>
                    <td class="head">
                        Job
                    </td>
                    <td class="head">
                        Job state
                    </td>
                    <td class="head">
                        Orders
                    </td>
                    <td class="head1">&#160;</td>
                </tr>
                <tr>
                    <td colspan="99" class="after_head_space">&#160;</td>
                </tr>
            </thead>
            
            <tbody class="job_chain">
                <xsl:for-each select="$job_chain_select">

                    <xsl:element name="tr">
                        <xsl:if test="not( $single )">
                            <xsl:attribute name="style">
                                cursor: hand;
                                padding-top: 1ex
                            </xsl:attribute>
                            <xsl:attribute name="onmouseover">
                                this.className = "hover"
                            </xsl:attribute>
                            <xsl:attribute name="onmouseout" >
                                this.className = "job_chain"
                            </xsl:attribute>
                            <xsl:attribute name="onclick">show_job_chain_details( '<xsl:value-of select="@name"/>' )</xsl:attribute>
                        </xsl:if>
                                
                        <td>
                            <b><xsl:value-of select="@name"/></b>
                        </td>
                        <td></td>
                        <td></td>
                        <td>
                            <xsl:value-of select="@orders"/>
                        </td>
                        <td></td>
                    </xsl:element>
    
                    <xsl:if test="$single  or  /spooler/@show_job_chain_jobs_checkbox  or  /spooler/@show_job_chain_orders_checkbox and job_chain_node/job/order_queue/order">
                        <xsl:for-each select="job_chain_node[ @job ]">
                            <!-- $show_orders vergrößert den Abstand zwischen den Job_chain_nodes. Aber nur, wenn überhaupt ein Order in der Jobkette ist -->
                            <xsl:variable name="show_orders" select="( /spooler/@show_job_chain_orders_checkbox or $single ) and ../job_chain_node/job/order_queue/order"/>

                            <xsl:variable name="tr_style">
                                <xsl:if test="$show_orders">
                                    padding-top: 1ex;
                                </xsl:if>
                            </xsl:variable>

                            <xsl:element name="tr">
                                <xsl:if test="$single ">
                                    <xsl:attribute name="style">
                                        <xsl:value-of select="$tr_style"/>
                                    </xsl:attribute>
                                </xsl:if>
                                
                                <xsl:if test="not( $single )">
                                    <xsl:attribute name="style">
                                        cursor: hand; 
                                        <xsl:value-of select="$tr_style"/>
                                    </xsl:attribute>
                                    <xsl:attribute name="onmouseover">
                                        this.className = "hover"
                                    </xsl:attribute>
                                    <xsl:attribute name="onmouseout" >
                                        this.className = "job_chain"
                                    </xsl:attribute>
                                    <xsl:attribute name="onclick">show_job_details('<xsl:value-of select="@job"/>')</xsl:attribute>
                                </xsl:if>
                                                                
                                <td>
                                    <span style="margin-left: 2ex">
                                        <xsl:value-of select="@state"/>
                                    </span>
                                </td>
                
                                <td>
                                    <xsl:value-of select="@job"/>
                                </td>                            
                                
                                <td>
                                    <xsl:value-of select="job/@state"></xsl:value-of>
                                    <xsl:if test="job/tasks/@count>0">
                                        <xsl:text>, </xsl:text>
                                        <xsl:value-of select="job/tasks/@count"></xsl:value-of>
                                        tasks
                                    </xsl:if>
                                </td>
                                
                                <td>
                                    <xsl:value-of select="@orders"/>
                                </td>
                                
                                <td></td>
                            </xsl:element>
                            
                            <xsl:if test="$show_orders">
                                <xsl:apply-templates select="job/order_queue" mode="job_chain_list">
                                    <xsl:with-param name="max_orders" select="$max_orders"/>
                                </xsl:apply-templates>
                            </xsl:if>

                            <!--xsl:apply-templates select="job/tasks/task" mode="job_chain_list"/-->
                            
                        </xsl:for-each>                        
                    </xsl:if>
                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Order in Job chain list-->

    <xsl:template match="order_queue" mode="job_chain_list">
        <xsl:param name="max_orders" select="999999999"/>

        <xsl:for-each select="order[ position() &lt;= $max_orders ]">
            <xsl:element name="tr">
                <xsl:attribute name="class">order</xsl:attribute>

                <td></td>
                
                <td>
                    <span style="margin-left: 2ex; white-space: nowrap">
                        <xsl:value-of select="@task"/>
                    </span>
                </td>
                
                <xsl:element name="td">
                    <xsl:if test="@task">
                        <xsl:attribute name="style">font-weight: bold</xsl:attribute>
                    </xsl:if>                
                    <!--xsl:attribute name="colspan">2</xsl:attribute-->
                    
                    <span style="margin-left: 2ex; white-space: nowrap">
                        Order <xsl:value-of select="@id"/>
                        <xsl:text>&#160; </xsl:text>
                    </span>
                </xsl:element>

                <xsl:element name="td">
                    <xsl:attribute name="align">left</xsl:attribute>
                    <xsl:if test="@task">
                        <xsl:attribute name="style">font-weight: bold</xsl:attribute>
                    </xsl:if>                
                    <span style="white-space: nowrap">
                        <xsl:value-of select="@title"/>
                    </span>
                </xsl:element>
                
                <td>
                    <xsl:call-template name="command_menu">
                        <xsl:with-param name="onclick" select="concat( 'order_menu__onclick(&quot;', @job_chain, '&quot;, &quot;', @id, '&quot;)' )"/>
                    </xsl:call-template>
                </td>
                
            </xsl:element>
        </xsl:for-each>
        
        <xsl:if test="@length >= $max_orders  or  order[ ../@length > last() ]">
            <tr class="order">
                <td></td>
                <td></td>
                <td>
                    <span style="margin-left: 2ex">...</span>
                </td>
                <td></td>
                <td></td>
            </tr>
        </xsl:if>
    </xsl:template> 

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Task (in Job chain list)-->
    
    <!-- Hier ist einiges von <xsl:template match="tasks" mode="job_list"> kopiert.
         Die gleichen Teile sollten mit <call-template> ausgelagert werden. 

    <xsl:template match="task" mode="job_chain_list">
        <xsl:element name="tr">
            <xsl:attribute name="class">task</xsl:attribute>
            <xsl:choose>
                <xsl:when test=" not( @id ) ">
                    <td colspan="2">
                        <span style="margin-left: 2ex">
                            <xsl:choose>
                                <xsl:when test="../../@waiting_for_process='yes'">
                                    Need process
                                </xsl:when>
                                <xsl:otherwise>
                                    <xsl:choose>
                                        <xsl:when test="../../@next_start_time">
                                            Next start
                                        </xsl:when>
                                        <xsl:otherwise>
                                            .
                                        </xsl:otherwise>
                                    </xsl:choose>
                                </xsl:otherwise>
                            </xsl:choose>
                        </span>
                    </td>

                    <td>
                        <xsl:if test="../../@next_start_time">
                            <xsl:value-of select="my:datetime_diff( string( ../../@next_start_time), $now )"  disable-output-escaping="yes"/>
                        </xsl:if>
                    </td>
                    
                    <td></td>
                    
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
                    
                    <td></td>
                    
                    <td>
                        <span style="margin-left: 2ex">
                            Task&#160;<xsl:value-of select="@id"/>
                        </span>
                        <xsl:if test="@name!=''">
                            &#160; <xsl:value-of select="@name"/>
                        </xsl:if>
                    </td>

                    <td></td>
                    
                    <td class="order">
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
                        <xsl:if test="@in_process_since!=''">
                            <xsl:text> &#160;</xsl:text>
                            <span class="small">
                                (<xsl:value-of select="my:datetime_diff( string( @in_process_since ), $now, 0 )"  disable-output-escaping="yes"/>)
                            </span>
                        </xsl:if>
                    </td>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:element>
    </xsl:template>
    -->
            
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~process_classes-->

    <xsl:template match="process_classes">
        <table width="100%" cellpadding="0" cellspacing="0" class="process_class">
            <!--caption align="left" class="process_class">
                <b>Process classes</b>
            </caption-->
            
            <col valign="baseline"  width=" 50"/>
            <col valign="baseline"  width=" 150"/>  
            <!--col valign="baseline"  width=" 150"/-->
            <col valign="baseline"  width="$datetime_column_width"/>
            <col valign="baseline"  width=" 10"  align="right"/>
            <col valign="baseline"  width=" 10"  align="right"/>
            <col valign="baseline"  width="*"/>
            
            <thead>
                <tr style="">
                    <td class="head1" style="padding-left: 2ex">Pid </td>
                    <td class="head">Task</td>
                    <td class="head">Running since</td>
                    <td class="head">Operations</td>
                    <td class="head">Callbacks</td>
                    <td class="head">Current operation</td>
                </tr>
                <tr>
                    <td colspan="99" class="after_head_space">&#160;</td>
                </tr>
            </thead>
            
            <tbody>
                <xsl:for-each select="process_class">
                
                    <tr style="padding-top: 1ex">
                        <td colspan="99">
                            <b>
                                <xsl:choose>
                                    <xsl:when test="@name!=''">
                                        <xsl:value-of select="@name"/>
                                    </xsl:when>
                                    <xsl:otherwise>
                                        (default)
                                    </xsl:otherwise>
                                </xsl:choose>
                            </b>
                            
                            <xsl:text>&#160; </xsl:text>
                            max_processes=<xsl:value-of select="@max_processes"/>
                        </td>
                    </tr>
                    
                    <xsl:for-each select="processes/process">
                        <tr>
                            <td style="padding-left: 2ex"><xsl:value-of select="@pid"/></td>
                            <td><xsl:value-of select="@job"/><xsl:text>&#160;&#160;</xsl:text><xsl:value-of select="@task_id"/></td>
                            <td style="white-space: nowrap"><xsl:value-of select="my:format_datetime_with_diff( string( @running_since ), $now, 0 )" disable-output-escaping="yes"/></td>
                            <td class="small"><xsl:value-of select="@operations"/></td>
                            <td class="small"><xsl:value-of select="@callbacks"/></td>
                            <td class="small"><xsl:value-of select="@operation"/></td>
                        </tr>
                    </xsl:for-each>
                    
                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Detailsicht eines Jobs-->
    
    <xsl:template match="job">
        <table cellpadding="0" cellspacing="0" width="100%" class="job">
            <caption class="job">
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
                                <xsl:with-param name="onclick" select="concat( 'job_menu__onclick(&quot;', @job, '&quot;)' )"/>
                            </xsl:call-template>
                        </td>
                    </tr>
                </table>
            </caption>
            
            <col valign="baseline" align="left" width="1"/>
            <col valign="baseline" align="left" width="*"  />  

            
            <tr><td>&#160;</td></tr>
                
            <tr>
                <td><span class="label">description:</span></td>
                <td>
                    <div class="description">
                        <xsl:value-of select="description" disable-output-escaping="yes"/>
                    </div>
                </td>
            </tr>
                
            <tr>
                <td><span class="label">state:</span></td>
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
                <td><span class="label">error:</span></td>
                <xsl:choose>
                    <xsl:when test="ERROR">
                        <td class="job_error">
                            <xsl:apply-templates select="ERROR"/>
                        </td>
                    </xsl:when>
                    <xsl:otherwise>
                        <td></td>
                    </xsl:otherwise>
                </xsl:choose>
            </tr>
                
            <tr>
                <td><span class="label">next start time:</span></td>
                <td>
                    <xsl:value-of select="my:format_datetime_with_diff( string( @next_start_time), $now, true )"  disable-output-escaping="yes"/>
                </td>
            </tr>

            <tr>
                <td><span class="label">steps:</span></td>
                <td><xsl:value-of select="@all_steps"/></td>
            </tr>

            <xsl:if test="@order='yes'">
                <tr>
                    <td><span class="label">orders:</span></td>
                    <td>
                        <!--xsl:choose>
                            <xsl:when test="@order!='yes'">
                                (no order job)
                            </xsl:when-->
                            <xsl:if test="order_queue/@length!=''">
                                <xsl:value-of select="order_queue/@length"/> orders to process
                            </xsl:if>
                        <!--/xsl:choose-->
                    </td>
                </tr>
            </xsl:if>
            
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

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Detailsicht einer Task-->
    
    <xsl:template match="task">
        <table cellpadding="0" cellspacing="0" class="task" width="100%" >
            <col valign="baseline" align="left"  width="1"/>
            <col valign="baseline" align="left"  />  

            <caption class="task">
                <table cellpadding="0" cellspacing="0" width="100%">
                    <tr>
                        <td>
                            <b>Task&#160;</b>
                            
                            <xsl:choose>
                                <xsl:when test="not( @id )">
                                    <xsl:if test="../../@waiting_for_process='yes'">
                                        (needs process)
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
                                    <xsl:with-param name="onclick" select="concat( 'task_menu__onclick(', @id, ')' )"/>
                                </xsl:call-template>
                            </td>
                        </xsl:if>
                    </tr>
                </table>
            </caption>
            
            <tr><td><span style="line-height: 6px">&#160;</span>   </td></tr>
                
            <xsl:if test="order or ../../@order='yes'">
                <tr>
                    <td><span class="label">order:</span></td>
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
                        <td><span class="label">idle since:</span></td>
                        <xsl:if test="@idle_since">
                            <td><xsl:value-of select="my:format_datetime_with_diff( string( @idle_since ), $now, 0 )"  disable-output-escaping="yes"/></td>
                        </xsl:if>
                    </tr>
                </xsl:when>
                <xsl:otherwise>
                    <tr>
                        <td><span class="label">in process since:</span></td>
                        <td><xsl:value-of select="my:format_datetime_with_diff( string( @in_process_since ), $now, 0 )"  disable-output-escaping="yes"/></td>
                    </tr>
                </xsl:otherwise>
            </xsl:choose>

            <tr>
                <!--xsl:choose>
                    <xsl:when test="@running_since"-->
                        <td><span class="label">running since:</span></td>
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
                    <td><span class="label">enqueued at:</span></td>
                    <td><xsl:value-of select="my:format_datetime_with_diff( string( @enqueued ), $now, 0 )"  disable-output-escaping="yes"/></td>
                </tr>
            </xsl:if>            

        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~queued_tasks-->

    <xsl:template match="queued_tasks" mode="list">
        
        <table valign="top" cellpadding="0" cellspacing="0" width="100%" class="task">

            <caption class="task" align="left">
                <p style="margin-top: 2px; margin-bottom: 1ex">
                    <b><xsl:value-of select="@length"/> enqueued tasks</b>
                </p>
            </caption>
            
            <xsl:if test="queued_task">
                <col valign="baseline" align="left" width="40"/>
                <col valign="baseline" align="left" width="70"/>
                <col valign="baseline" align="left" width="$datetime_column_width"/>
                
                <thead>
                    <tr>
                        <td class="head1">Id</td>
                        <td class="head1">Enqueued</td>
                        <td class="head1">Start at</td>
                    </tr>
                    <tr>
                        <td colspan="99" class="after_head_space">&#160;</td>
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
                <p style="margin-top: 2px; margin-bottom: 1ex">
                    <b><xsl:value-of select="@length"/> orders</b>
                </p>
            </caption>
            
            <col valign="baseline"  width=" 40"/>
            <!--col valign="top"  width=" 15"  style="padding-right: 2ex"/-->  
            <col valign="baseline"  width=" 70"/>  
            <col valign="baseline"  width=" 40"/>  
            <col valign="baseline"  width="*"/>  
            <col valign="baseline"  width="*"/>  
            <col valign="baseline"  width="  1" align="right"/>  
            
            <thead class="order">
                <tr>
                    <td class="head1">Id</td>
                    <!--td class="order">Pri</td-->
                    <td class="head1">Created</td>
                    <td class="head1">State</td>
                    <td class="head1">State text</td>
                    <td class="head1">Title</td>
                    <td class="head1">&#160;</td>
                </tr>
                <tr>
                    <td colspan="99" class="after_head_space">&#160;</td>
                </tr>
            </thead>
            
            <tbody>
                <xsl:for-each select="order">
                    <xsl:element name="tr">
                        <xsl:if test="@task">
                            <xsl:attribute name="style">font-weight: bold</xsl:attribute>
                        </xsl:if>                
                        
                        <td><xsl:value-of select="@id"/></td>
                        <!--td class="order"><xsl:value-of select="@priority"/></td-->
                        <td><xsl:value-of select="my:format_date_or_time( string( @created ) )"  disable-output-escaping="yes"/></td>
                        <td><xsl:value-of select="@state"/></td>
                        <td><xsl:value-of select="@state_text"/></td>
                        <td><xsl:value-of select="@title"/></td>
                        
                        <td align="right">
                            <xsl:call-template name="command_menu">
                                <xsl:with-param name="onclick" select="concat( 'order_menu__onclick(&quot;', @job_chain, '&quot;, &quot;', @id, '&quot;)' )"/>
                            </xsl:call-template>
                        </td>
                    </xsl:element>
                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template> 

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ERROR-->

    <xsl:template match="ERROR">
        <span class="job_error">
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
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~attributes_for_onclick-->
    <!--
    <xsl:template name="attributes_for_onclick">
        <xsl:param name="onclick"/>
        <xsl:param name="class"/>
        <xsl:param name="style"/>
        
        <xsl:attribute name="style">
            cursor: hand;
            <xsl:value-of select="$style"/>
        </xsl:attribute>
        <xsl:attribute name="onmouseover">
            this.className = "hover"
        </xsl:attribute>
        <xsl:attribute name="onmouseout" >
            this.className = "<xsl:value-of select="$class"/>"
        </xsl:attribute>
        <xsl:attribute name="onclick"><xsl:value-of select="$onclick"/></xsl:attribute>
    </xsl:template>
    -->                                
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_menu-->

    <xsl:template name="command_menu">
        <xsl:param name="onclick"/>
        <xsl:element name="span">
            <xsl:attribute name="class">
                small
            </xsl:attribute>
            <xsl:attribute name="style">
                cursor: hand; text-decoration: underline; padding-left: 4pt; font-weight: normal;
            </xsl:attribute>
            <xsl:attribute name="onclick">
                <xsl:value-of select="$onclick"/>
            </xsl:attribute>
            
            Menu
            
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~script-->


    <msxsl:script language="JavaScript" implements-prefix="my"><![CDATA[
    
        //--------------------------------------------------------------------------format_datetime
    
        function format_datetime( datetime ) 
        {
            if( !datetime )  return "";
            return datetime.replace( /\.\d*$/, "" );
            /*            
            var date = typeof datetime == "string"? date_from_datetime( datetime ) : datetime;
            
            //var ms = date.getMilliseconds();

            return date.toLocaleDateString() + ", " + date.toLocaleTimeString();
                   //+ ( ms? ".<span class='milliseconds'>" + ( ms + "000" ).substring( 0, 3 ) + "</span>" : "" );
            */                   
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
            var result = format_datetime( datetime );
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
            
            date.setFullYear    ( 1*datetime.substring( 0, 4 ), 1*datetime.substring( 5, 7 ) - 1, 1*datetime.substring( 8, 10 ) );
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
