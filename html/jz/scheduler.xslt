<?xml version='1.0' encoding="utf-8"?>
<!-- $Id$ -->
<xsl:stylesheet xmlns:xsl   = "http://www.w3.org/1999/XSL/Transform"
                xmlns:msxsl = "urn:schemas-microsoft-com:xslt"
                version     = "1.0">

    <xsl:variable name="now"                    select="string( /spooler/answer/@time )"/>
    <xsl:variable name="datetime_column_width"  select="100"/>    <!-- 250 für langes Format, toLocaleDateString() -->
    <xsl:variable name="text_Job_chains"        select="'Job chains'"/>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Gesamtsicht-->
    <!-- Für Antwort auf <show_state> -->

    <xsl:template match="/spooler/answer">

        <table cellspacing="0" cellpadding="0">
            <tr>
                <td style="margin-top: 0px; margin-bottom: 1ex">
                    <xsl:element name="span">
                        <xsl:attribute name="title">Version  <xsl:value-of select="state/@version"/>&#10;pid=<xsl:value-of select="state/@pid"/>&#10;db=<xsl:value-of select="state/@db"/></xsl:attribute>
                        <b>Scheduler</b>
                    </xsl:element>

                    <xsl:if test="state/@id!=''">
                        <xsl:text>&#160;</xsl:text>
                        <b><span style="white-space: nowrap">-id=<xsl:value-of select="state/@id"/></span></b>
                        &#160;
                    </xsl:if>
                </td>
            </tr>
            <tr>
                <td valign="middle" style="padding-left: 0px">
                    <xsl:call-template name="scheduler_info"/>
                </td>
                <td valign="middle" align="right">
                    <xsl:call-template name="update_button"/>
                </td>
            </tr>
        </table>

        <p id="error_message" class="small" style="margin-top: 0px; color: red"> </p>
        <span style="color: black"> </span>    <!-- Für Firefox -->


        <!-- Jobs, Jobketten oder Prozessklassen zeigen? -->

        <table cellpadding="0" cellspacing="0" style="margin-top: 0ex">
            <tr>
                <xsl:apply-templates mode="card_selector" select="/spooler">
                    <xsl:with-param name="name"  select="'jobs'"/>
                    <xsl:with-param name="title" select="'Jobs'"/>
                    <xsl:with-param name="class" select="'job'"/>
                </xsl:apply-templates>

                <xsl:if test="state/job_chains/job_chain">
                    <xsl:apply-templates mode="card_selector" select="/spooler">
                        <xsl:with-param name="name"  select="'job_chains'"/>
                        <xsl:with-param name="title" select="'Job chains'"/>
                        <xsl:with-param name="class" select="'job_chain'"/>
                    </xsl:apply-templates>
                </xsl:if>

                <xsl:apply-templates mode="card_selector" select="/spooler">
                    <xsl:with-param name="name"  select="'process_classes'"/>
                    <xsl:with-param name="title" select="'Process classes'"/>
                    <xsl:with-param name="class" select="'process_class'"/>
                </xsl:apply-templates>
            </tr>
        </table>


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

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~card_selector-->
    <!-- Zeigt einen Selektor an, z.B. Jobs, Jobketten, Prozessklassen -->

    <xsl:template mode="card_selector" match="*">
        <xsl:param name="name" />
        <xsl:param name="title"/>
        <xsl:param name="class"/>

        <td style="margin-bottom: 0pt; padding: 1ex 0pt 1px 5pt">
        <!--    onmouseover="this.className='hover'"
            onmouseout ="this.className=''"-->

            <xsl:element name="span">
                <xsl:attribute name="onclick">call_error_checked( show_card, '<xsl:value-of select="$name"/>' )</xsl:attribute>
                <xsl:attribute name="style"  >cursor: pointer; padding: 1pt 4pt 2px 4pt</xsl:attribute>
                <xsl:attribute name="class"><xsl:value-of select="$class"/></xsl:attribute>
                <xsl:element name="span">
                    <xsl:if test="@my_show_card=$name ">
                        <xsl:attribute name="style">font-weight: bold</xsl:attribute>
                    </xsl:if>
                    <span class="translate">
                        <xsl:value-of select="$title"/>
                    </span>
                </xsl:element>
            </xsl:element>
        </td>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~card_top-->
    <!-- Jede Karte hat etwas Abstand zum Card selector -->

    <xsl:template name="card_top">
        <tr>
            <td colspan="99"><p style="margin-top: 0px; line-height: 5px;">&#160;</p></td>
        </tr>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Scheduler-Info-->
    <!-- Allgemeine Angaben zum Scheduler -->

    <xsl:template name="scheduler_info">
        <table cellpadding="0" cellspacing="0" class="scheduler">
            <!--
            <tr>
                <td valign="baseline" align="left" style="padding-right: 1ex">
                    <span style="margin-top: 2px; margin-bottom: 2pt">


                    </span>
                </td>

                <td valign="baseline" align="right" style="padding-left: 0">
                    <span style="margin-top: 2px; margin-bottom: 2px">
                        <xsl:text> </xsl:text>
                    </span>
                </td>

            </tr>
            -->

            <tr>
                <td colspan="2">
                    <span>
                        <xsl:value-of select="state/@time__xslt_datetime"  disable-output-escaping="yes"/>
                        <xsl:text> (</xsl:text>
                            <xsl:value-of select="state/@spooler_running_since__xslt_datetime_diff"  disable-output-escaping="yes"/>
                        <xsl:text>)</xsl:text>
                    </span>
                    &#160; &#160;
                    <xsl:value-of select="state/@state"/>
                </td>

                <td valign="top" align="right">
                    <!--span onclick = "open_url( '/doc/index.xml', 'scheduler_documentation' )"
                          class   = "small"
                          style   = "cursor: pointer; text-decoration: underline;">
                    </span-->
                    <a href="javascript:void(0)" onclick="open_url( '/doc/index.xml', 'scheduler_documentation' )" target="scheduler_documentation" class="small">Doku</a>&#160;

                    <xsl:call-template name="command_menu">
                        <xsl:with-param name="onclick" select="'scheduler_menu__onclick( mouse_x() - 100, mouse_y() - 1 )'"/>
                    </xsl:call-template>
                </td>
            </tr>

            <tr>
                <td colspan="99">
                    <a href="show_config?" target="config_xml"><xsl:value-of select="state/@config_file"/></a>
                </td>
            </tr>

            <tr>
                <td colspan="3" style="padding-top: 1pt">
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

            <xsl:if test="state/@db_waiting='yes'">
                <tr>
                    <td colspan="99" style="color: red">
                        &#160;<br/>
                        Scheduler wartet auf die Datenbank ...<br/>
                        <xsl:value-of select="state/@db_error"/>
                    </td>
                </tr>
            </xsl:if>

        </table>
    </xsl:template>


    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~update_button-->

    <xsl:template name="update_button">
        <p style="margin-top: 0px; padding-left: 0px; white-space: nowrap">
            <input id="update_button" type="button" value="Update" onclick="call_error_checked( update__onclick )" NAME="update_button"/>
            <br/>
            <input id="update_periodically_checkbox" type="checkbox" onclick="call_error_checked( update_periodically_checkbox__onclick )" NAME="update_periodically_checkbox"/>
            <label for="update_periodically_checkbox"><span class="translate">every </span><xsl:value-of select="/*/@my_update_seconds"/>s</label>
        </p>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Jobs-->

    <xsl:template match="jobs">
        <table cellpadding="0" cellspacing="0" width="100%" class="job">
            <col valign="baseline"  width="100"/>
            <col valign="baseline"  width="50"  align="right"/>
            <col valign="baseline"  width="30"  align="right"/>
            <col valign="baseline"/>

            <thead>
                <xsl:call-template name="card_top"/>
                <tr>
                    <td colspan="4" align="left" class="job">
                        <!--b>Jobs</b>
                        &#160;-->

                        <!-- Checkbox für Show order jobs -->
                        <xsl:element name="input">
                            <xsl:attribute name="id"     >show_order_jobs_checkbox</xsl:attribute>
                            <xsl:attribute name="type"   >checkbox</xsl:attribute>
                            <xsl:attribute name="onclick">call_error_checked( show_order_jobs_checkbox__onclick )</xsl:attribute>
                            <xsl:if test="/spooler/@show_order_jobs_checkbox">
                                <xsl:attribute name="checked">checked</xsl:attribute>
                            </xsl:if>
                        </xsl:element>
                        <label for="show_order_jobs_checkbox">Show order jobs</label>
                        &#160;

                        <!-- Checkbox für Show tasks -->
                        <xsl:element name="input">
                            <xsl:attribute name="id"     >show_tasks_checkbox</xsl:attribute>
                            <xsl:attribute name="type"   >checkbox</xsl:attribute>
                            <xsl:attribute name="onclick">call_error_checked( show_tasks_checkbox__onclick )</xsl:attribute>
                            <xsl:if test="/spooler/@show_tasks_checkbox">
                                <xsl:attribute name="checked">checked</xsl:attribute>
                            </xsl:if>
                        </xsl:element>
                        <label for="show_tasks_checkbox">Show tasks</label>
                    </td>
                </tr>
                <xsl:call-template name="card_top"/>
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
                <xsl:for-each select="job [ /spooler/@show_order_jobs_checkbox or not( @order='yes' ) ]">

                    <xsl:element name="tr">
                        <xsl:attribute name="id"         >scheduler_tr_job_<xsl:value-of select="@job"/></xsl:attribute>
                        <xsl:attribute name="class"      >job</xsl:attribute>
                        <xsl:attribute name="style"      >cursor: default; padding-top: 5pt</xsl:attribute>
                        <xsl:attribute name="onmouseover">
                            this.className =
                            document.getElementById( "scheduler_tr_job_<xsl:value-of select="@job"/>__2" ).className = "job_hover"
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className =
                            document.getElementById( "scheduler_tr_job_<xsl:value-of select="@job"/>__2" ).className = "job"
                        </xsl:attribute>
                        <xsl:attribute name="onclick">call_error_checked( show_job_details, '<xsl:value-of select="@job"/>' )</xsl:attribute>

                        <td colspan="99">
                            <b><xsl:value-of select="@job"/></b>
                            &#160;
                            <xsl:value-of select="@title"/>
                            <xsl:if test="@state_text!=''">
                                <xsl:text> &#160;–&#160; </xsl:text>
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
                        <xsl:attribute name="style">cursor: default</xsl:attribute>
                        <xsl:attribute name="onmouseover">
                            this.className =
                            document.getElementById( "scheduler_tr_job_<xsl:value-of select="@job"/>" ).className = "job_hover"
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className =
                            document.getElementById( "scheduler_tr_job_<xsl:value-of select="@job"/>" ).className = "job"
                        </xsl:attribute>
                        <xsl:attribute name="onclick">call_error_checked( show_job_details, '<xsl:value-of select="@job"/>' )</xsl:attribute>

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
                        <td>
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
                                                ∙
                                            </xsl:otherwise>
                                        </xsl:choose>
                                    </xsl:otherwise>
                                </xsl:choose>
                            </span>
                        </td>

                        <td align="right">
                            <xsl:value-of select="../../@next_start_time__xslt_datetime_diff"  disable-output-escaping="yes"/>
                        </td>

                        <td></td>

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
                        <xsl:attribute name="style"      >cursor: default</xsl:attribute>
                        <xsl:attribute name="onmouseover">this.className='task_hover'</xsl:attribute>
                        <xsl:attribute name="onmouseout" >this.className='task'</xsl:attribute>
                        <xsl:attribute name="onclick">call_error_checked( show_task_details, '<xsl:value-of select="../../@job"/>', <xsl:value-of select="@id"/> )</xsl:attribute>

                        <td>
                            <span style="margin-left: 2ex">
                                Task&#160;<xsl:value-of select="@id"/>
                            </span>
                            <xsl:if test="@name!=''">
                                &#160; <xsl:value-of select="@name"/>
                            </xsl:if>
                        </td>

                        <td align="right">
                            <xsl:if test="@running_since!=''">
                                <xsl:text> &#160;</xsl:text>
                                <!--span class="small"-->
                                    <xsl:value-of select="@running_since__xslt_datetime_diff"  disable-output-escaping="yes"/>
                                <!--/span-->
                            </xsl:if>
                        </td>

                        <td align="right">
                            <xsl:value-of select="@steps"/>
                        </td>


                        <xsl:choose>
                            <xsl:when test="../../@order='yes'">
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
                                    <!--xsl:if test="@state='running_waiting_for_order'">
                                        waiting for order
                                    </xsl:if-->
                                    <xsl:if test="@in_process_since!=''">
                                        <xsl:text> &#160;</xsl:text>
                                        <span class="small">
                                            (<xsl:value-of select="@in_process_since__xslt_datetime_diff"  disable-output-escaping="yes"/>)
                                        </span>
                                    </xsl:if>
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td>
                                    <xsl:value-of select="@state"/>
                                    <xsl:text> </xsl:text>
                                    <xsl:if test="@in_process_since!=''">
                                        <span class="small">
                                            (<xsl:value-of select="@in_process_since__xslt_datetime_diff"  disable-output-escaping="yes"/>)
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
            <col valign="baseline"  width=" 50"/>
            <col valign="baseline"  width="100"/>
            <col valign="baseline"  width="120"/>
            <col valign="baseline"  width="40" align="right"/>
            <col valign="baseline"  width="10" align="right"/>

            <thead class="job_chain">
                <xsl:call-template name="card_top"/>
                <tr>
                    <td colspan="99" align="left" class="job_chain">
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
                                <xsl:attribute name="onclick">call_error_checked( show_job_chain_jobs_checkbox__onclick )</xsl:attribute>
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
                                <xsl:attribute name="onclick">call_error_checked( show_job_chain_orders_checkbox__onclick )</xsl:attribute>
                                <xsl:if test="/spooler/@show_job_chain_orders_checkbox or $single">
                                    <xsl:attribute name="checked">checked</xsl:attribute>
                                </xsl:if>
                            </xsl:element>
                            <label for="show_job_chain_orders_checkbox">Show orders</label>
                        </xsl:if>
                    </td>
                </tr>

                <xsl:call-template name="card_top"/>

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

                    <xsl:if test="( /spooler/@show_job_chain_orders_checkbox or /spooler/@show_job_chain_jobs_checkbox ) and position() &gt; 1">
                        <tr><td colspan="99"><hr style="color: black" size="1"/></td></tr>
                    </xsl:if>

                    <xsl:element name="tr">
                        <xsl:if test="not( $single )">
                            <xsl:attribute name="style">
                                cursor: default;
<!--
                                <xsl:choose>
                                    <xsl:when test="/spooler/@show_job_chain_orders_checkbox">
                                        padding-top: 0pt;
                                    </xsl:when>
                                    <xsl:when test="/spooler/@show_job_chain_jobs_checkbox">
                                        padding-top: 0pt;
                                    </xsl:when>
                                    <xsl:otherwise>
                                        padding-top: 0;
                                    </xsl:otherwise>
                                </xsl:choose>
-->
                            </xsl:attribute>
                            <xsl:attribute name="onmouseover">
                                this.className = "job_chain_hover"
                            </xsl:attribute>
                            <xsl:attribute name="onmouseout" >
                                this.className = "job_chain"
                            </xsl:attribute>
                            <xsl:attribute name="onclick">call_error_checked( show_job_chain_details, '<xsl:value-of select="@name"/>' )</xsl:attribute>
                        </xsl:if>

                        <td colspan="3">
                            <b><xsl:value-of select="@name"/></b>
                        </td>
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
                                        cursor: default;
                                        <xsl:value-of select="$tr_style"/>
                                    </xsl:attribute>
                                    <xsl:attribute name="onmouseover">
                                        this.className = "job"
                                    </xsl:attribute>
                                    <xsl:attribute name="onmouseout" >
                                        this.className = "job_chain"
                                    </xsl:attribute>
                                    <xsl:attribute name="onclick">call_error_checked( show_job_details, '<xsl:value-of select="@job"/>' )</xsl:attribute>
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
                    <xsl:if test="@task">
                        <span style="margin-left: 2ex; white-space: nowrap">
                            Task <xsl:value-of select="@task"/>
                        </span>
                    </xsl:if>
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
                        <xsl:with-param name="onclick" select="concat( 'order_menu__onclick( &quot;', @job_chain, '&quot;, &quot;', @id, '&quot;, mouse_x() - 20, mouse_y() - 1 )' )"/>
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
                    <xsl:attribute name="style"      >cursor: pointer</xsl:attribute>
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
                <xsl:call-template name="card_top"/>
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
                            <td style="white-space: nowrap"><xsl:value-of select="@running_since__xslt_datetime_with_diff" disable-output-escaping="yes"/></td>
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
            <col valign="baseline" align="left" width="1"/>
            <col valign="baseline" align="left" width="160"  />
            <col valign="baseline" align="left" width="1"/>
            <col valign="baseline" align="left" width="*"  />

            <tr>
                <td colspan="99" class="job">
                    <table cellpadding="0" cellspacing="0" width="100%">
                        <tr>
                            <td style="padding-left: 0px">
                                <b>
                                    Job
                                    <xsl:value-of select="@job"/>
                                </b>
                                <xsl:if test="@title">
                                    <xsl:text> &#160; </xsl:text><xsl:value-of select="@title"/>
                                </xsl:if>
                            </td>

                            <td align="right" style="padding-right: 0pt">
                                <xsl:call-template name="command_menu">
                                    <xsl:with-param name="onclick" select="concat( 'job_menu__onclick(  &quot;', @job, '&quot;, mouse_x() - 90, mouse_y() - 1 )' )"/>
                                </xsl:call-template>
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>

            <tr><td>&#160;</td></tr>
<!--
            <tr>
                <td><span class="label">description:</span></td>
                <td>
                    <div class="description">
                        <xsl:value-of select="description" disable-output-escaping="yes"/>
                    </div>
                </td>
            </tr>
-->
            <tr>
                <td><span class="label">state:</span></td>
                <td colspan="99">
                    <xsl:value-of select="@state"/>
                    <!--xsl:if test="@waiting_for_process='yes'">
                        <xsl:text>,</xsl:text>
                        <span class="waiting_for_process"> waiting for process!</span>
                    </xsl:if-->

                    <xsl:if test="@state_text and @state_text != '' ">
                        &#160;–&#160;
                        <xsl:value-of select="@state_text"/>
                    </xsl:if>
                </td>
            </tr>

            <tr>
                <td><span class="label">error:</span></td>
                <xsl:choose>
                    <xsl:when test="ERROR">
                        <td colspan="3" class="job_error">
                            <xsl:apply-templates select="ERROR"/>
                        </td>
                    </xsl:when>
                    <xsl:otherwise>
                        <td></td>
                    </xsl:otherwise>
                </xsl:choose>
            </tr>

            <tr>
                <td><span class="label">next start:</span></td>
                <td colspan="3">
                    <xsl:value-of select="@next_start_time__xslt_datetime_with_diff_plus"  disable-output-escaping="yes"/>
                </td>
            </tr>

            <tr>
                <td><span class="label">steps:</span></td>
                <td><xsl:value-of select="@all_steps"/></td>
                <td><span class="label">tasks:</span></td>
                <td><xsl:value-of select="@all_tasks"/></td>
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

            <xsl:apply-templates mode="trs_from_log_attributes" select="log"/>
        </table>


        <p>&#160; </p>

        <xsl:for-each select="tasks/task">
            <p> </p>
            <xsl:apply-templates select="."/>
        </xsl:for-each>


        <!-- Task-Warteschlange, Task-Historie oder Auftragswarteschlange zeigen? -->

        <p style="margin-top: 5ex; margin-bottom: 3ex"></p>

        <table cellpadding="0" cellspacing="0" style="margin-top: 0ex">
            <tr>
                <xsl:apply-templates mode="card_selector" select=".">
                    <xsl:with-param name="name"  select="'task_queue'"/>
                    <xsl:with-param name="title" select="'Task queue'"/>
                    <xsl:with-param name="class" select="'task'"/>
                </xsl:apply-templates>

                <xsl:apply-templates mode="card_selector" select=".">
                    <xsl:with-param name="name"  select="'task_history'"/>
                    <xsl:with-param name="title" select="'Task history'"/>
                    <xsl:with-param name="class" select="'task'"/>
                </xsl:apply-templates>

                <xsl:if test="@order='yes'">
                    <xsl:apply-templates mode="card_selector" select=".">
                        <xsl:with-param name="name"  select="'order_queue'"/>
                        <xsl:with-param name="title" select="'Order queue'"/>
                        <xsl:with-param name="class" select="'order'"/>
                    </xsl:apply-templates>
                </xsl:if>
            </tr>
        </table>


        <xsl:if test="@my_show_card='task_queue'">
            <xsl:apply-templates select="queued_tasks" mode="list"/>
        </xsl:if>

        <xsl:if test="@my_show_card='task_history'">
            <xsl:apply-templates select="history" mode="list"/>
        </xsl:if>

        <xsl:if test="@my_show_card='order_queue'">
            <xsl:apply-templates select="order_queue" mode="list"/>
        </xsl:if>



        <!-- Tasks queue bei order='yes' nur zeigen, wenn sie nicht leer ist (was aber unsinnig wäre)
        <xsl:if test="not( @order='yes' and ( not( queued_tasks ) or queued_tasks/@length='0' ) )">
            <p style="margin-top: 5ex; margin-bottom: 3ex"></p>
            <xsl:apply-templates select="queued_tasks" mode="list"/>
        </xsl:if>

        <p style="margin-top: 5ex; margin-bottom: 3ex"></p>
        <xsl:apply-templates select="order_queue" mode="list"/>
        -->

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Detailsicht einer Task-->

    <xsl:template match="task">
        <table cellpadding="0" cellspacing="0" class="task" width="100%" >
            <col valign="baseline" align="left"  width="1"/>
            <col valign="baseline" align="left"  />

            <tr>
                <td colspan="99" class="task">
                    <table cellpadding="0" cellspacing="0" width="100%">
                        <tr>
                            <td style="padding-left: 0px">
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
                                    <span style="white-space: nowrap"><xsl:value-of select="@steps"/> steps</span>
                                </xsl:if>
                            </td>

                            <xsl:if test="@id">
                                <td align="right" valign="top" style="padding-right: 0pt">
                                    <xsl:call-template name="command_menu">
                                        <xsl:with-param name="onclick" select="concat( 'task_menu__onclick(  ', @id, ', mouse_x() - 70, mouse_y() - 1 )' )"/>
                                    </xsl:call-template>
                                </td>
                            </xsl:if>
                        </tr>
                    </table>
                </td>
            </tr>

            <tr>
                <td><span style="line-height: 6px">&#160;</span></td>
            </tr>

            <xsl:if test="order or ../../@order='yes'">
                <tr>
                    <td><span class="label">order:</span></td>
                    <td class="order">
                        <xsl:choose>
                            <xsl:when test="order">
                                <b>
                                    <xsl:value-of select="order/@id"/>
                                    &#160;
                                    <xsl:value-of select="order/@title"/>
                                </b>
                            </xsl:when>
                            <xsl:when test="@state='running_waiting_for_order'">
                                waiting for order ...
                            </xsl:when>
                        </xsl:choose>
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
                        <td><xsl:value-of select="@idle_since__xslt_datetime_with_diff"  disable-output-escaping="yes"/></td>
                    </tr>
                </xsl:when>
                <xsl:otherwise>
                    <tr>
                        <td><span class="label">in process since:</span></td>
                        <td><xsl:value-of select="@in_process_since__xslt_datetime_with_diff"  disable-output-escaping="yes"/></td>
                    </tr>
                </xsl:otherwise>
            </xsl:choose>

            <tr>
                <td><span class="label">running since:</span></td>
                <td>
                    <xsl:value-of select="@running_since__xslt_datetime_with_diff"  disable-output-escaping="yes"/>
                </td>
            </tr>

            <xsl:if test="@enqueued">
                <tr>
                    <td><span class="label">enqueued at:</span></td>
                    <td><xsl:value-of select="@enqueued__xslt_datetime_with_diff"  disable-output-escaping="yes"/></td>
                </tr>
            </xsl:if>

            <xsl:apply-templates mode="trs_from_log_attributes" select="log"/>

        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~queued_tasks-->

    <xsl:template match="queued_tasks" mode="list">

        <table valign="top" cellpadding="0" cellspacing="0" width="100%" class="task">

            <col valign="baseline" align="left" width="40"/>
            <col valign="baseline" align="left" width="70"/>
            <col valign="baseline" align="left" width="$datetime_column_width"/>

            <thead>
                <xsl:call-template name="card_top"/>

                <tr>
                    <td colspan="99" class="task" align="left">
                        <p style="margin-top: 2px; margin-bottom: 1ex">
                            <b><xsl:value-of select="@length"/> enqueued tasks</b>
                        </p>
                    </td>
                </tr>

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

                    <td><xsl:value-of select="@enqueued__xslt_date_or_time"        disable-output-escaping="yes"/></td>
                    <td><xsl:value-of select="@start_at__xslt_datetime_with_diff"  disable-output-escaping="yes"/></td>
                </tr>
            </xsl:for-each>
        </table>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~history-->

    <xsl:template match="history" mode="list">

        <table valign="top" cellpadding="0" cellspacing="0" width="100%" class="task">

            <col valign="baseline" align="left" width="1"/>
            <col valign="baseline" align="left" width="1"/>
            <col valign="baseline" align="left" width="$datetime_column_width"/>
            <col valign="baseline" align="right" width="1"/>
            <col valign="baseline" align="left" width="$datetime_column_width"/>
            <col valign="baseline" align="left"/>

            <thead>
                <xsl:call-template name="card_top"/>

                <tr>
                    <td class="head1">Id</td>
                    <td class="head">Cause</td>
                    <td class="head">Started</td>
                    <td class="head">Steps</td>
                    <td class="head" colspan="2">Ended</td>
                </tr>
                <tr>
                    <td colspan="99" class="after_head_space">&#160;</td>
                </tr>
            </thead>


            <xsl:for-each select="history.entry">
                <tr>
                    <td><xsl:value-of select="@task"/></td>
                    <td><xsl:value-of select="@cause"/></td>
                    <td><xsl:value-of select="@start_time__xslt_datetime"   disable-output-escaping="yes"/></td>
                    <td><xsl:value-of select="@steps"/></td>
                    <td><xsl:value-of select="@end_time__xslt_datetime"     disable-output-escaping="yes"/></td>

                    <td align="right" valign="top" style="padding-right: 0pt">
                        <xsl:call-template name="command_menu">
                            <xsl:with-param name="onclick" select="concat( 'history_task_menu__onclick(  ', @id, ', mouse_x() - 70, mouse_y() - 1 )' )"/>
                        </xsl:call-template>
                    </td>
                </tr>

                <xsl:if test="ERROR">
                    <tr>
                        <td> </td>
                        <td colspan="99" class="task_error">
                            <xsl:apply-templates select="ERROR"/>
                        </td>
                    </tr>
                    <tr>
                        <td>&#160;</td>
                    </tr>
                </xsl:if>
            </xsl:for-each>


            <xsl:if test="ERROR">
                <tr>
                    <td colspan="99" class="task_error">
                        <xsl:apply-templates select="ERROR"/>
                    </td>
                </tr>
            </xsl:if>
        </table>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Order_queue-->

    <xsl:template match="order_queue" mode="list">
        <table class="order" cellpadding="0" cellspacing="0" width="100%">

            <!--xsl:if test="order"-->
                <col valign="baseline"  width=" 40"/>
                <!--col valign="top"  width=" 15"  style="padding-right: 2ex"/-->
                <col valign="baseline"  width=" 70"/>
                <col valign="baseline"  width=" 40"/>
                <col valign="baseline"  width="*"/>
                <col valign="baseline"  width="*"/>
                <col valign="baseline"  width="  1" align="right"/>

                <thead class="order">
                    <xsl:call-template name="card_top"/>

                    <tr>
                        <td colspan="99" align="left" class="order">
                            <p style="margin-top: 2px; margin-bottom: 1ex">
                                <b><xsl:value-of select="@length"/> orders</b>
                            </p>
                        </td>
                    </tr>

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
                            <td><xsl:value-of select="@created__xslt_date_or_time"  disable-output-escaping="yes"/></td>
                            <td><xsl:value-of select="@state"/></td>
                            <td><xsl:value-of select="@state_text"/></td>
                            <td><xsl:value-of select="@title"/></td>

                            <td align="right">
                                <xsl:call-template name="command_menu">
                                    <xsl:with-param name="onclick" select="concat( 'order_menu__onclick( &quot;', @job_chain, '&quot;, &quot;', @id, '&quot;, mouse_x() - 70, mouse_y() - 1 )' )"/>
                                </xsl:call-template>
                            </td>
                        </xsl:element>
                    </xsl:for-each>
                </tbody>
            <!--/xsl:if-->
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ERROR-->

    <xsl:template match="ERROR">
        <xsl:if test="@time != ''">
            <xsl:value-of select="@time"/>
            <br/>
        </xsl:if>

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
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~attributes_for_onclick-->
    <!--
    <xsl:template name="attributes_for_onclick">
        <xsl:param name="onclick"/>
        <xsl:param name="class"/>
        <xsl:param name="style"/>

        <xsl:attribute name="style">
            cursor: pointer;
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
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~log-->

    <xsl:template mode="trs_from_log_attributes" match="log">

        <tr>
            <td><span class="label">log:</span></td>

            <td colspan="99">
                <xsl:apply-templates mode="string_from_log_attributes" select="."/>
            </td>
        </tr>

        <xsl:if test="@mail_subject">
            <tr>
                <td>
                    <span class="label">mail subject: </span>
                </td>
                <td colspan="99">
                    <xsl:value-of select="@mail_subject"/>
                </td>
            </tr>
        </xsl:if>

        <xsl:if test="@last_error">
            <tr>
                <td>
                    <span class="label">last error: </span>
                </td>
                <td colspan="99">
                    <xsl:value-of select="@last_error"/>
                </td>
            </tr>
        </xsl:if>

        <xsl:if test="@last_warning">
            <tr>
                <td>
                    <span class="label">last warning: </span>
                </td>
                <td colspan="99">
                    <xsl:value-of select="@last_warning"/>
                </td>
            </tr>
        </xsl:if>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~log-->

    <xsl:template mode="string_from_log_attributes" match="log">

        <xsl:if test="@level">
            <span class="label">log-level: </span>
            <xsl:value-of select="@level"/>
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

        <xsl:if test="@highest_level">
            <span class="label">highest level: </span>
            <xsl:value-of select="@highest_level"/>
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

        <xsl:if test="@mail_on_error='yes'">
            mail on error
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

        <xsl:if test="@mail_on_warning='yes'">
            mail on warning
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

        <xsl:if test="@mail_on_success='yes'">
            mail on success
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

        <xsl:if test="@mail_on_process">
            <span class="label">mail on process: </span>
            <xsl:value-of select="@mail_on_process"/>
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

        <xsl:if test="@mail_to">
            <span class="label">to: </span>
            <xsl:value-of select="@mail_to"/>
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

        <xsl:if test="@mail_cc">
            <span class="label">cc: </span>
            <xsl:value-of select="@mail_cc"/>
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

        <xsl:if test="@mail_bcc">
            <span class="label">bcc: </span>
            <xsl:value-of select="@mail_bcc"/>
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

        <xsl:if test="@mail_from">
            <span class="label">from: </span>
            <xsl:value-of select="@mail_from"/>
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

        <xsl:if test="@smtp">
            <span class="label">smtp: </span>
            <xsl:value-of select="@smtp"/>
            <xsl:text> &#160; </xsl:text>
        </xsl:if>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_menu-->

    <xsl:template name="command_menu">
        <xsl:param name="onclick"/>
        <xsl:element name="span">
            <xsl:attribute name="class">
                small
            </xsl:attribute>
            <xsl:attribute name="style">
                cursor: pointer; text-decoration: underline; padding-left: 4pt; font-weight: normal;
            </xsl:attribute>
            <xsl:attribute name="onclick">
                <xsl:value-of select="$onclick"/>
            </xsl:attribute>

            <xsl:text>Menu</xsl:text>

        </xsl:element>
    </xsl:template>

</xsl:stylesheet>

<!-- Das ist ein Gedankenstrich: – -->
<!-- Das ist drei Punkte: … -->
