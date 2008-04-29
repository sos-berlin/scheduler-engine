<?xml version='1.0' encoding="utf-8"?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl   = "http://www.w3.org/1999/XSL/Transform"
                xmlns:msxsl = "urn:schemas-microsoft-com:xslt"
                version     = "1.0">

    <xsl:variable name="now"                    select="string( /spooler/answer/@time )"/>
    <xsl:variable name="datetime_column_width"  select="100"/>
    <!-- 250 für langes Format, toLocaleDateString() -->
    <xsl:variable name="text_Job_chains"        select="'Job chains'"/>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Gesamtsicht-->
    <!-- Für Antwort auf <show_state> -->

    <xsl:template match="/spooler/answer">

        <table cellspacing="0" cellpadding="0" width="100%">
            <tr>
                <td>
                    <table cellspacing="0" cellpadding="0">
                        <tr>
                            <td style="margin-top: 0px; padding-bottom: 2pt">
                                <b>
                                    <xsl:element name="span">
                                        <!--xsl:attribute name="style">cursor: default;</xsl:attribute-->
                                        <xsl:attribute name="title">
                                            Version  <xsl:value-of select="state/@version"/>&#10;pid=<xsl:value-of select="state/@pid"/>&#10;db=<xsl:value-of select="state/@db"/>
                                        </xsl:attribute>
                                        Scheduler

                                        <xsl:value-of select="state/@host" />:<xsl:value-of select="state/@tcp_port" />

                                        <xsl:if test="state/@id!=''">
                                            <xsl:text>&#160;</xsl:text>
                                            <span style="white-space: nowrap">
                                                -id=<xsl:value-of select="state/@id"/>
                                            </span>
                                            &#160;
                                        </xsl:if>
                                    </xsl:element>
                                </b>
                            </td>
                        </tr>

                        <tr>
                            <td style="vertical-align: top; padding-left: 0px">
                                <xsl:call-template name="scheduler_info"/>
                            </td>
                            <td style="vertical-align: top;">
                                <xsl:call-template name="update_button"/>
                            </td>
                        </tr>
                    </table>
                </td>

                <td style="text-align: right">
                    <img alt="logo" width="45" height="45" src="job_scheduler_rabbit_circle_45x45.gif"/>
                </td>
            </tr>
        </table>

        <p id="error_message" class="small" style="margin-top: 0px; color: #e00000">&#160;</p>
        <span style="color: black"> </span>    <!-- Für Firefox -->

        &#160;<br/>

        <!-- Jobs, Jobketten oder Prozessklassen zeigen? -->

        <table cellpadding="0" cellspacing="0" style="margin-top: 0ex">
            <tr>
                <xsl:apply-templates mode="card_selector" select="/spooler">
                    <xsl:with-param name="name"  select="'jobs'"/>
                    <xsl:with-param name="title" select="'Jobs'"/>
                    <xsl:with-param name="class" select="'job'"/>
                </xsl:apply-templates>

                <!--xsl:if test="state/job_chains/@count > 0"-->
                <xsl:apply-templates mode="card_selector" select="/spooler">
                    <xsl:with-param name="name"  select="'job_chains'"/>
                    <xsl:with-param name="title" select="'Job chains'"/>
                    <xsl:with-param name="class" select="'job_chain'"/>
                </xsl:apply-templates>
                <!--/xsl:if-->

                <xsl:if test="state/schedules">
                    <xsl:apply-templates mode="card_selector" select="/spooler">
                        <xsl:with-param name="name"  select="'schedules'"/>
                        <xsl:with-param name="title" select="'Schedules'"/>
                        <xsl:with-param name="class" select="'schedule'"/>
                    </xsl:apply-templates>
                </xsl:if>

                <xsl:apply-templates mode="card_selector" select="/spooler">
                    <xsl:with-param name="name"  select="'calendar'"/>
                    <xsl:with-param name="title" select="'Calendar'"/>
                    <xsl:with-param name="class" select="'calendar'"/>
                </xsl:apply-templates>

                <xsl:if test="state/locks">
                    <xsl:apply-templates mode="card_selector" select="/spooler">
                        <xsl:with-param name="name"  select="'locks'"/>
                        <xsl:with-param name="title" select="'Locks'"/>
                        <xsl:with-param name="class" select="'lock'"/>
                    </xsl:apply-templates>
                </xsl:if>

                <xsl:apply-templates mode="card_selector" select="/spooler">
                    <xsl:with-param name="name"  select="'process_classes'"/>
                    <xsl:with-param name="title" select="'Process classes'"/>
                    <xsl:with-param name="class" select="'process_class'"/>
                </xsl:apply-templates>

                <xsl:if test="state/cluster">
                    <xsl:apply-templates mode="card_selector" select="/spooler">
                        <xsl:with-param name="name"  select="'cluster'"/>
                        <xsl:with-param name="title" select="'Cluster'"/>
                        <xsl:with-param name="class" select="'cluster'"/>
                    </xsl:apply-templates>
                </xsl:if>

                <xsl:if test="state/remote_schedulers[ @count > 0 ]">
                    <xsl:apply-templates mode="card_selector" select="/spooler">
                        <xsl:with-param name="name"  select="'remote_schedulers'"/>
                        <xsl:with-param name="title" select="'Remote schedulers'"/>
                        <xsl:with-param name="class" select="'remote_scheduler'"/>
                    </xsl:apply-templates>
                </xsl:if>

                <xsl:call-template name="card_selector_end"/>
            </tr>
        </table>


        <xsl:if test="/spooler/@my_show_card='jobs'">
            <xsl:apply-templates select="state/jobs"/>
        </xsl:if>

        <xsl:if test="/spooler/@my_show_card='job_chains'">
            <xsl:apply-templates select="state/job_chains"/>
        </xsl:if>

        <xsl:if test="/spooler/@my_show_card='locks'">
            <xsl:apply-templates select="state/locks"/>
        </xsl:if>

        <xsl:if test="/spooler/@my_show_card='process_classes'">
            <xsl:apply-templates select="state/process_classes"/>
        </xsl:if>

        <xsl:if test="/spooler/@my_show_card='schedules'">
            <xsl:apply-templates select="state/schedules"/>
        </xsl:if>

        <xsl:if test="/spooler/@my_show_card='cluster'">
            <xsl:apply-templates select="state/cluster"/>
        </xsl:if>

        <xsl:if test="/spooler/@my_show_card='remote_schedulers'">
            <xsl:apply-templates select="state/remote_schedulers"/>
        </xsl:if>

        <xsl:if test="/spooler/@my_show_card='calendar'">
            <xsl:apply-templates select="calendar"/>
        </xsl:if>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~card_selector-->
    <!-- Zeigt einen Selektor an, z.B. Jobs, Jobketten, Prozessklassen -->

    <xsl:template mode="card_selector" match="*">
        <xsl:param name="name" />
        <xsl:param name="title"/>
        <xsl:param name="class"/>

        <td style="margin-bottom: 0pt; padding: 1ex 0pt 2px 0pt;">
            <!--    onmouseover="this.className='hover'"
            onmouseout ="this.className=''"-->

            <span style="padding-left: 3pt; padding-bottom: 2pt; border-bottom: 1px solid gray;">&#160;</span>

            <xsl:element name="span">
                <xsl:attribute name="onclick">
                    call_error_checked( show_card, '<xsl:value-of select="$name"/>' )
                </xsl:attribute>
                <xsl:attribute name="class">
                    <xsl:value-of select="$class"/>
                </xsl:attribute>
                <xsl:choose>
                    <xsl:when test="@my_show_card=$name ">
                        <xsl:attribute name="style">cursor: pointer; padding: 1pt 4pt 2px 4pt; font-weight: bold; border-top: 1px solid gray; border-left: 1px solid gray; border-right: 1px solid gray;</xsl:attribute>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:attribute name="style">cursor: pointer; padding: 1pt 2pt 2px 4pt; font-size: 8pt; border: 1px solid gray; </xsl:attribute>
                    </xsl:otherwise>
                </xsl:choose>

                <span class="translate" style="white-space: nowrap">
                    <xsl:value-of select="$title"/>
                </span>
            </xsl:element>
        </td>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~card_selector_end-->
    
    <xsl:template name="card_selector_end">
        <td style="margin-bottom: 0pt; padding: 1ex 0pt 2px 0pt;">
            <span style="padding-left: 3pt; padding-bottom: 2pt; border-bottom: 1px solid gray;">&#160;&#160;&#160;&#160;&#160;&#160;</span>
        </td>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~card_top-->
    <!-- Jede Karte hat etwas Abstand zum Card selector -->

    <xsl:template name="card_top">
        <tr>
            <td colspan="6">
                <p style="margin-top: 0px; line-height: 5px;">&#160;</p>
            </td>
        </tr>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Scheduler-Info-->
    <!-- Allgemeine Angaben zum Scheduler -->

    <xsl:template name="scheduler_info">
        <table cellpadding="0" cellspacing="0" class="scheduler">
            <!--
            <tr>
                <td align="left" style="padding-right: 1ex">
                    <span style="margin-top: 2px; margin-bottom: 2pt">


                    </span>
                </td>

                <td align="right" style="padding-left: 0">
                    <span style="margin-top: 2px; margin-bottom: 2px">
                        <xsl:text> </xsl:text>
                    </span>
                </td>

            </tr>
            -->

            <tr>
                <td colspan="2">
                    <span>
                        <xsl:apply-templates mode="time" select="state/@time">
                            <xsl:with-param name="show" select="'datetime'"/>
                            <xsl:with-param name="modified_short_attribute" select="state/@time__xslt_datetime"/>
                        </xsl:apply-templates>
                        <!--xsl:value-of select="state/@time__xslt_datetime"  disable-output-escaping="yes"/-->

                        <xsl:text> (</xsl:text>
                        <xsl:apply-templates mode="time" select="state/@time">
                            <xsl:with-param name="show" select="'datetime_diff'"/>
                            <xsl:with-param name="modified_short_attribute" select="state/@spooler_running_since__xslt_datetime_diff"/>
                        </xsl:apply-templates>
                        <!--xsl:value-of select="state/@spooler_running_since__xslt_datetime_diff"  disable-output-escaping="yes"/-->
                        <xsl:text>)</xsl:text>
                    </span>

                    <xsl:text>&#160;</xsl:text>
                    <xsl:apply-templates select="state/@state"/>

                    <xsl:choose>
                        <xsl:when test="state/@wait_until">
                            <xsl:element name="span">
                                <!--xsl:attribute name="style">
                                    <xsl:text>cursor: default;</xsl:text>
                                </xsl:attribute-->
                                <xsl:attribute name="class"></xsl:attribute>
                                <xsl:attribute name="title">
                                    <xsl:text>Waiting until </xsl:text>
                                    <xsl:value-of select="state/@wait_until__xslt_datetime_with_diff"/>
                                    <xsl:if test="state/@resume_at">
                                        <xsl:text>, resuming at </xsl:text>
                                        <xsl:value-of select="state/@resume_at__xslt_datetime_with_diff"/>
                                    </xsl:if>
                                </xsl:attribute>
                                <xsl:text>, </xsl:text>
                                <span style="white-space: nowrap">
                                    <xsl:text>next: </xsl:text>
                                    <xsl:value-of select="state/@wait_until__xslt_date_or_time"/>
                                </span>
                            </xsl:element>
                        </xsl:when>
                        <xsl:otherwise>
                            <span>, processing...</span>
                        </xsl:otherwise>
                    </xsl:choose>
                    <xsl:text>&#160;</xsl:text>
                </td>

                <td valign="top" style="text-align: right">
                    <!--span onclick = "open_url( '/doc/index.xml', 'scheduler_documentation' )"
                          class   = "small"
                          style   = "cursor: pointer; text-decoration: underline;">
                    </span-->
                    <a href="/doc/" target="scheduler_documentation" onclick="open_url( '/doc/', 'scheduler_documentation' )" class="small">Doc</a>&#160;
                    <!--a href="javascript:void(0)" onclick="open_url( '/doc/index.xml', 'scheduler_documentation' )" target="scheduler_documentation" class="small">Doku</a>&#160;-->

                    <xsl:call-template name="command_menu">
                        <xsl:with-param name="onclick" select="'scheduler_menu__onclick( mouse_x() - 100, mouse_y() - 1 )'"/>
                    </xsl:call-template>
                </td>
            </tr>

            <tr>
                <td colspan="99">
                    <a href="show_config?" target="config_xml">
                        <xsl:value-of select="state/@config_file"/>
                    </a>
                </td>
            </tr>

            <tr>
                <td colspan="3" style="padding-top: 1pt">
                    <span class="job" style="padding-left: 1pt; padding-right: 1pt;">
                        <xsl:call-template name="bold_counter">
                            <xsl:with-param name="counter" select="count( state/jobs/job [ @state='running' ] )" />
                            <xsl:with-param name="suffix" select="'jobs running'" />
                        </xsl:call-template>,

                        <xsl:call-template name="bold_counter">
                            <xsl:with-param name="counter" select="count( state/jobs/job [ @state='stopped' ] )" />
                            <xsl:with-param name="suffix" select="'stopped'" />
                            <xsl:with-param name="class" select="'scheduler_error'" />
                        </xsl:call-template>,

                        <xsl:call-template name="bold_counter">
                            <xsl:with-param name="counter" select="count( state/jobs/job [ @waiting_for_process='yes' ] )" />
                            <xsl:with-param name="suffix" select="'need process'" />
                        </xsl:call-template>
                    </span>

                    <span class="task" style="margin-left: 1ex; padding-left: 1pt; padding-right: 1pt;">
                        <xsl:call-template name="bold_counter">
                            <xsl:with-param name="counter" select="count( state/jobs/job/tasks/task[ @id ] )" />
                            <xsl:with-param name="suffix" select="'tasks'" />
                        </xsl:call-template>
                    </span>

                    <span class="order" style="margin-left: 1ex; padding-left: 1pt; padding-right: 1pt;">
                        <xsl:call-template name="bold_counter">
                            <xsl:with-param name="counter" select="sum( state/jobs/job/order_queue/@length )" />
                            <xsl:with-param name="suffix" select="'orders'" />
                        </xsl:call-template>
                    </span>
                </td>
            </tr>

            <xsl:if test="state/@db_waiting='yes'">
                <tr>
                    <td colspan="99" style="color: #e00000; font-weight: bold">
                        &#160;<br/>
                        Scheduler wartet auf die Datenbank ...<br/>
                        <xsl:value-of select="state/@db_error"/>
                    </td>
                </tr>
            </xsl:if>

            <xsl:if test="state/@waiting_errno">
                <tr>
                    <td colspan="99" style="color: #e00000; font-weight: bold">
                        Scheduler wartet wegen Dateifehlers:<br/>
                        <xsl:value-of select="state/@waiting_errno_text"/><br/>
                        <xsl:value-of select="state/@waiting_errno_filename"/>
                    </td>
                </tr>
            </xsl:if>

        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~bold_counter-->

    <xsl:template name="bold_counter">
        <xsl:param name="counter"/>
        <xsl:param name="suffix"/>
        <xsl:param name="style"  select="'font-weight: bold'"/>
        <xsl:param name="class"  select="''"/>

        <xsl:element name="span">
            <xsl:attribute name="style">
                white-space: nowrap;
                <xsl:if test="$counter &gt; 0">
                    <xsl:value-of select="$style"/>
                </xsl:if>
            </xsl:attribute>
            <xsl:if test="$counter &gt; 0 and $class">
                <xsl:attribute name="class">
                    <xsl:value-of select="$class"/>
                </xsl:attribute>
            </xsl:if>

            <xsl:value-of select="$counter"/>
            <xsl:text> </xsl:text>
            <xsl:value-of select="$suffix"/>
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~update_button-->

    <xsl:template name="update_button">
        <p style="margin-top: 0px; padding-left: 0px; white-space: nowrap">
            <input id="update_button" type="button" value="Update" onclick="call_error_checked( update__onclick )" NAME="update_button"/>
            <br/>

            <xsl:element name="input">
                <xsl:attribute name="id"     >update_periodically_checkbox</xsl:attribute>
                <xsl:attribute name="type"   >checkbox</xsl:attribute>
                <xsl:attribute name="onclick">call_error_checked( update_periodically_checkbox__onclick )</xsl:attribute>
                <xsl:if test="/spooler/@update_periodically_checkbox">
                    <xsl:attribute name="checked">checked</xsl:attribute>
                </xsl:if>
            </xsl:element>

            <label for="update_periodically_checkbox">
                <span class="translate">every </span><xsl:value-of select="/*/@my_update_seconds"/>s
            </label>
        </p>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~state/@state-->

    <xsl:template match="state/@state">
        <xsl:choose>
            <xsl:when test=".='running'">
                <xsl:value-of select="."/>
            </xsl:when>
            <xsl:otherwise>
                <span class="scheduler_error">
                    <xsl:value-of select="."/>
                </span>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Jobs-->

    <xsl:template match="jobs">
        <table cellpadding="0" cellspacing="0" width="100%" class="job">
            <col width="100"/>
            <col width="50"  align="right"/>
            <col width="30"  align="right"/>
            <col/>

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
                    <td class="head">
                        Next start
                        <xsl:if test="/spooler/@show_order_jobs_checkbox">
                            / Orders
                        </xsl:if>
                    </td>
                </tr>
                <tr>
                    <td colspan="99" class="after_head_space">&#160;</td>
                </tr>
            </thead>

            <tbody>
                <xsl:for-each select="job [ /spooler/@show_order_jobs_checkbox or not( @order='yes' ) ]">
                    <xsl:sort select="translate( @path, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz' )"/>

                    <xsl:element name="tr">
                        <xsl:attribute name="id"         >
                            <xsl:text>scheduler_tr_job_</xsl:text>
                            <xsl:value-of select="@path"/>
                        </xsl:attribute>
                        <xsl:attribute name="class"      >job</xsl:attribute>
                        <xsl:attribute name="style"      >padding-top: 1pt</xsl:attribute>
                        <xsl:attribute name="onmouseover">
                            this.className =
                            document.getElementById( "scheduler_tr_job_<xsl:value-of select="@path"/>__2" ).className = "job_hover";
                            this.style.cursor = "pointer";
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className =
                            document.getElementById( "scheduler_tr_job_<xsl:value-of select="@path"/>__2" ).className = "job"
                            this.style.cursor = "default";
                        </xsl:attribute>
                        <xsl:attribute name="onclick">
                            call_error_checked( show_job_details, '<xsl:value-of select="@path"/>' )
                        </xsl:attribute>

                        <td colspan="4">
                            <xsl:apply-templates mode="bold_job_path" select="@path"/>
                            &#160;
                            <xsl:value-of select="@title"/>
                            <xsl:if test="@state_text!=''">
                                <xsl:text> &#160;–&#160; </xsl:text>
                                <xsl:call-template name="show_text_with_url">
                                    <xsl:with-param name="text" select="@state_text"/>
                                </xsl:call-template>
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
                        <xsl:attribute name="id"   >
                            <xsl:text>scheduler_tr_job_</xsl:text>
                            <xsl:value-of select="@path"/>
                            <xsl:text>__2</xsl:text>
                        </xsl:attribute>
                        <xsl:attribute name="class">job         </xsl:attribute>
                        <!--xsl:attribute name="style">cursor: default</xsl:attribute-->
                        <xsl:attribute name="onmouseover">
                            this.className =
                            document.getElementById( "scheduler_tr_job_<xsl:value-of select="@path"/>" ).className = "job_hover";
                            this.style.cursor = "pointer";
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className =
                            document.getElementById( "scheduler_tr_job_<xsl:value-of select="@path"/>" ).className = "job"
                            this.style.cursor = "default";
                        </xsl:attribute>
                        <xsl:attribute name="onclick">
                            call_error_checked( show_job_details, '<xsl:value-of select="@path"/>' )
                        </xsl:attribute>

                        <td colspan="2">
                            <xsl:apply-templates select="@state"/>
                            <xsl:if test="not( /spooler/@show_tasks_checkbox ) and tasks/@count>0">
                                <xsl:text>, </xsl:text>
                                <span style="white-space: nowrap">
                                    <xsl:value-of select="tasks/@count"/> tasks
                                </span>
                            </xsl:if>
                        </td>

                        <td style="text-align: right">
                            <xsl:value-of select="@all_steps"/>
                        </td>

                        <xsl:choose>
                            <xsl:when test="@order='yes'">
                                <td class="order">
                                    <xsl:if test="order_queue/@next_start_time">
                                        <xsl:value-of select="order_queue/@next_start_time__xslt_date_or_time_with_diff"/>
                                        &#160;
                                    </xsl:if>
                                    <xsl:call-template name="bold_counter">
                                        <xsl:with-param name="counter" select="order_queue/@length" />
                                        <xsl:with-param name="suffix"  select="'orders'" />
                                    </xsl:call-template>
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td>
                                    <xsl:value-of select="@next_start_time__xslt_date_or_time_with_diff"/>
                                </td>
                            </xsl:otherwise>
                        </xsl:choose>

                    </xsl:element>

                    <xsl:if test="ERROR">
                        <tr class="job">
                            <td colspan="99" class="error_message">
                                <xsl:apply-templates select="ERROR"/>
                            </td>
                        </tr>
                    </xsl:if>

                    <xsl:if test="file_based/ERROR or file_based/removed or replacement or file_based/requisites/requisite [ @is_missing='yes' ]">
                        <tr>
                            <td colspan="4" style="padding-left: 4ex; padding-bottom: 0.5em;">
                                <xsl:apply-templates mode="file_based_line" select="."/>
                            </td>
                        </tr>
                    </xsl:if>
                    
                    <xsl:if test="/spooler/@show_tasks_checkbox and tasks/task">
                        <xsl:apply-templates select="tasks" mode="job_list"/>
                    </xsl:if>

                    <tr>
                        <td colspan="99" style="border-bottom: 1px solid #00b0ff; line-height: 3pt">
                            &#160;
                        </td>
                    </tr>

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
                        <td colspan="3">
                            <span style="margin-left: 2ex">
                                <xsl:choose>
                                    <xsl:when test="../../@waiting_for_process='yes'">
                                        <span class="task_error">Needs process!</span>
                                    </xsl:when>
                                </xsl:choose>
                            </span>
                        </td>

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
                        <!--xsl:attribute name="style"      >cursor: default</xsl:attribute-->
                        <xsl:attribute name="onmouseover">this.className='task_hover'; this.cursor = "pointer";</xsl:attribute>
                        <xsl:attribute name="onmouseout" >this.className='task'      ; this.cursor = "default";</xsl:attribute>
                        <xsl:attribute name="onclick">
                            call_error_checked( show_task_details, '<xsl:value-of select="../../@path"/>', <xsl:value-of select="@id"/> )
                        </xsl:attribute>

                        <td>
                            <span style="margin-left: 2ex">
                                Task&#160;<xsl:value-of select="@id"/>
                            </span>
                            <xsl:if test="@name!=''">
                                &#160; <xsl:value-of select="@name"/>
                            </xsl:if>
                        </td>

                        <td style="text-align: right">
                            <xsl:if test="@running_since!=''">
                                <xsl:text> &#160;</xsl:text>
                                <!--span class="small"-->
                                <xsl:value-of select="@running_since__xslt_datetime_diff"  disable-output-escaping="yes"/>
                                <!--/span-->
                            </xsl:if>
                        </td>

                        <td style="text-align: right">
                            <xsl:value-of select="@steps"/>
                        </td>


                        <xsl:choose>
                            <xsl:when test="../../@order='yes'">
                                <td class="order">
                                    <xsl:if test="@state!='running'">
                                        <xsl:apply-templates select="@state" />
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
                                            (<xsl:value-of select="@in_process_since__xslt_datetime_diff"  disable-output-escaping="yes"/> in step)
                                        </span>
                                    </xsl:if>
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td>
                                    <xsl:apply-templates select="." mode="task_line"/>
                                </td>
                            </xsl:otherwise>
                        </xsl:choose>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:element>
        </xsl:for-each>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~task-->

    <xsl:template match="task" mode="task_line">
        <xsl:apply-templates select="@state" />
        <xsl:text> </xsl:text>
        <xsl:if test="@in_process_since!=''">
            <span class="small">
                (<xsl:value-of select="@in_process_since__xslt_datetime_diff"  disable-output-escaping="yes"/> in step)
            </span>
        </xsl:if>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Job_chains-->

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
        <xsl:param name="job_chain_select"/>
        <xsl:param name="single"             select="false()"/>
        <!-- false: Mehrere im linken Frame,
             true:  Eine Jobkette im rechten frame -->
        <xsl:variable name="max_orders">
            <xsl:choose>
                <xsl:when test="$single">
                    <xsl:text>9999</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="/spooler/@my_max_orders"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>


        <table cellpadding="0" cellspacing="0" width="100%">
            <col               width="10"/>
            <col valign="top"  width=" 40"/>
            <col valign="top"  width="100"/>
            <col valign="top"  width="900"/>
            <col valign="top"  width="40"/>
            <col valign="top"  width="10" align="right"/>

            <thead class="job_chain">
                <xsl:call-template name="card_top"/>

                <xsl:if test="$single">
                    <tr>
                        <td colspan="6">
                            Job chain&#160;
                            <b>
                                <xsl:value-of select="@path"/>
                            </b>
                            <xsl:if test="@title">
                                <xsl:text> </xsl:text>
                                "<xsl:value-of select="@title"/>"
                            </xsl:if>
                            <br/>&#160;
                        </td>
                    </tr>
                </xsl:if>

                <xsl:call-template name="card_top"/>

                <tr>
                    <td colspan="2" class="head1" style="padding-left: 2ex">
                        State
                    </td>
                    <td class="head">
                        <span style="white-space: nowrap">
                            Job
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
                                <label for="show_job_chain_jobs_checkbox">show</label>
                            </xsl:if>
                        </span>
                    </td>
                    <td class="head">
                        Job state, <span style="white-space: nowrap">next start</span>
                    </td>
                    <td colspan="2" class="head">
                        <span style="white-space: nowrap">
                            Orders
                            <xsl:if test="not( $single )">
                                <!-- Checkbox für Show orders-->
                                <xsl:element name="input">
                                    <xsl:attribute name="id"     >show_job_chain_orders_checkbox</xsl:attribute>
                                    <xsl:attribute name="type"   >checkbox</xsl:attribute>
                                    <xsl:attribute name="onclick">call_error_checked( show_job_chain_orders_checkbox__onclick )</xsl:attribute>
                                    <xsl:if test="/spooler/@show_job_chain_orders_checkbox or $single">
                                        <xsl:attribute name="checked">checked</xsl:attribute>
                                    </xsl:if>
                                </xsl:element>
                                <label for="show_job_chain_orders_checkbox">show</label>
                            </xsl:if>
                        </span>
                    </td>
                </tr>
                <tr>
                    <td colspan="6" class="after_head_space">&#160;</td>
                </tr>
            </thead>

            <tbody class="job_chain">
                <xsl:choose>
                    <xsl:when test="not( $job_chain_select )">
                        <tr>
                            <td colspan="6" class="small">(no job chains)</td>
                        </tr>
                    </xsl:when>

                    <xsl:when test="$single">
                        <xsl:apply-templates select="$job_chain_select [ not( @order_id_space ) ]" mode="job_chains_view">
                            <xsl:with-param name="single" select="$single"/>
                            <xsl:with-param name="max_orders" select="$max_orders"/>
                        </xsl:apply-templates>
                    </xsl:when>

                    <xsl:otherwise>
                        <xsl:apply-templates select="$job_chain_select [ not( @order_id_space ) ]" mode="job_chains_view">
                            <xsl:with-param name="single" select="$single"/>
                            <xsl:with-param name="max_orders" select="$max_orders"/>
                        </xsl:apply-templates>

                        <xsl:for-each select="/spooler/answer/state/order_id_spaces/order_id_space">
                            <tr>
                                <td colspan="6" class="job_chains_gap" style="">&#160;</td>
                            </tr>
                            <tr>
                                <td colspan="6" class="order_id_space" style="padding-top: 1.5em; padding-bottom: 0.5em;">
                                    <span title="Following job chains are connected, order IDs are unique">
                                        <span class="label" style="padding-right: 2px">Order_id_space </span>
                                        <span style="font-weight: bold;">
                                            <xsl:value-of select="@name"/>
                                        </span>
                                    </span>
                                </td>
                            </tr>
                            <xsl:apply-templates select="$job_chain_select [ @order_id_space=current()/@name ]" mode="job_chains_view">
                                <xsl:with-param name="single" select="$single"/>
                                <xsl:with-param name="max_orders" select="$max_orders"/>
                                <xsl:sort select="translate( @path, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz' )"/>
                            </xsl:apply-templates>
                        </xsl:for-each>
                    </xsl:otherwise>
                </xsl:choose>

                <!-- Nichtauftragsjobs zeigen -->

                <!--xsl:if test="/spooler/@show_job_chain_jobs_checkbox and not( $single ) and /spooler/answer/state/jobs/job [ not( @order='yes' ) ]"-->
                <xsl:if test="/spooler/@show_job_chain_jobs_checkbox and not( $single ) ">

                <tr>
                        <td colspan="6" class="job_chains_gap">&#160;</td>
                    </tr>

                    <tr>
                        <!--td colspan="5" class="label" style="padding-top: 4pt; border-top: 1px solid gray"-->
                        <td colspan="6" class="label" style="padding-top: 4pt;">
                            jobs not in a job chain
                        </td>
                    </tr>

                    <xsl:for-each select="/spooler/answer/state/jobs/job">
                        <xsl:sort select="translate( @path, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz' )"/>

                        <xsl:if test="not( /spooler/answer/state/job_chains/job_chain/job_chain_node/@job=current()/@path )">
                            <!--xsl:variable name="job" select="/spooler/answer/state/jobs/job[ @path = current()/@job ]"/-->

                            <xsl:element name="tr">
                                <xsl:attribute name="colspan">2</xsl:attribute>
                                <xsl:attribute name="class">job</xsl:attribute>
                                <!--xsl:attribute name="style">cursor: default;</xsl:attribute-->
                                <xsl:attribute name="onmouseover">
                                    <xsl:text>this.className = "job_hover";</xsl:text>
                                    <xsl:text>this.style.cursor = "pointer";</xsl:text>
                                </xsl:attribute>
                                <xsl:attribute name="onmouseout" >
                                    <xsl:text>this.className = "job";</xsl:text>
                                    <xsl:text>this.style.cursor = "default";</xsl:text>
                                </xsl:attribute>
                                <xsl:attribute name="onclick">
                                    call_error_checked( show_job_details, '<xsl:value-of select="@path"/>' )
                                </xsl:attribute>

                                <td></td>

                                <td class="right_border"></td>

                                <td class="right_border">
                                    <xsl:apply-templates mode="bold_job_path" select="@path"/>
                                </td>

                                <td colspan="3">
                                    <xsl:choose>
                                        <xsl:when test="file_based/ERROR">
                                            <xsl:apply-templates mode="file_based_error" select="file_based"/>
                                        </xsl:when>
                                        <xsl:otherwise>
                                            <xsl:apply-templates mode="job_state_line" select="." />

                                            <xsl:choose>
                                                <xsl:when test="tasks/@count>0">
                                                    <xsl:text>, </xsl:text>
                                                    <span class="task" style="padding-left: 2pt; padding-right: 2pt; padding-bottom: 2pt;">
                                                        <xsl:call-template name="bold_counter">
                                                            <xsl:with-param name="counter" select="tasks/@count" />
                                                            <xsl:with-param name="suffix" select="'tasks'" />
                                                        </xsl:call-template>
                                                    </span>
                                                </xsl:when>
                                                <xsl:otherwise>
                                                    <xsl:if test="@next_start_time">
                                                        <xsl:text>, </xsl:text>
                                                        <span style="white-space: nowrap">
                                                            <xsl:value-of select="@next_start_time__xslt_date_or_time_with_diff"/>
                                                        </span>
                                                    </xsl:if>
                                                </xsl:otherwise>
                                            </xsl:choose>

                                            <!--xsl:if test="@waiting_for_process='yes'">
                                                <xsl:text>, </xsl:text>
                                                <span style="color: #e00000">needs process</span>
                                            </xsl:if-->
                                        </xsl:otherwise>
                                    </xsl:choose>
                                </td>
                            </xsl:element>

                            <xsl:if test="file_based/removed or replacement">
                                <tr class="job">
                                    <td colspan="2" class="right_border"></td>
                                    <td colspan="4" style="padding-left: 4ex; padding-bottom: 0.5em;">
                                        <xsl:apply-templates mode="file_based_line" select="."/>
                                    </td>
                                </tr>
                            </xsl:if>

                        </xsl:if>
                    </xsl:for-each>
                </xsl:if>
            </tbody>
        </table>


        <xsl:if test="$single">
            <p>&#160;</p>
            <xsl:element name="input">
                <xsl:attribute name="id"     >show_order_history_checkbox</xsl:attribute>
                <xsl:attribute name="type"   >checkbox</xsl:attribute>
                <xsl:attribute name="onclick">call_error_checked( show_order_history_checkbox__onclick )</xsl:attribute>
                <xsl:if test="/spooler/@show_order_history_checkbox">
                    <xsl:attribute name="checked">checked</xsl:attribute>
                </xsl:if>
            </xsl:element>
            <label for="show_order_history_checkbox">Show order history</label>

            <xsl:apply-templates select="order_history" mode="list"/>

        </xsl:if>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~job_chain-->
    
    <xsl:template match="job_chain" mode="job_chains_view">

        <xsl:param name="single"/>
        <xsl:param name="max_orders"/>

        <xsl:variable name="job_chain_path" select="@path"/>

        <xsl:if test="( /spooler/@show_job_chain_orders_checkbox or /spooler/@show_job_chain_jobs_checkbox ) and position() &gt; 1">
            <tr>
                <td colspan="6">
                    <div>&#160;</div>
                    <!--hr style="margin-top: 5pt;"/-->
                </td>
            </tr>
        </xsl:if>

        <xsl:if test="not( $single )">
            <xsl:element name="tr">
                <!--xsl:attribute name="style">
                    cursor: default;
                </xsl:attribute-->
                <xsl:attribute name="onmouseover">
                    this.className = "job_chain_hover";
                    this.style.cursor = "pointer";
                </xsl:attribute>
                <xsl:attribute name="onmouseout" >
                    this.className = "job_chain";
                    this.style.cursor = "default"
                </xsl:attribute>
                <xsl:attribute name="onclick">
                    call_error_checked( show_job_chain_details, '<xsl:value-of select="@path"/>' )
                </xsl:attribute>

                <td colspan="3">
                    <b>
                        <xsl:value-of select="@path"/>
                    </b>
                </td>

                <td class="right_border">
                    <xsl:choose>
                        <xsl:when test="not( /spooler/@show_job_chain_jobs_checkbox)">
                            <xsl:if test="job_chain_node.job_chain">
                                <xsl:value-of select="count( job_chain_node.job_chain )"/> job chains
                                &#160;
                            </xsl:if>
                            <xsl:if test="job_chain_node[@job]">
                                <xsl:value-of select="count( job_chain_node[@job] )"/> jobs,
                                <xsl:call-template name="bold_counter">
                                    <xsl:with-param name="counter" select="count( /spooler/answer/state/jobs/job/tasks/task [ order/@job_chain=$job_chain_path ] )" />
                                    <xsl:with-param name="suffix"  select="'tasks'" />
                                </xsl:call-template>
                                &#160;
                            </xsl:if>
                        </xsl:when>
                        <xsl:when test="@title">
                            <xsl:text>&#160; "</xsl:text>
                            <xsl:value-of select="@title"/>
                            <xsl:text>" </xsl:text>
                        </xsl:when>
                    </xsl:choose>

                    <xsl:choose>
                        <xsl:when test="not( @state )"/>
                        <!--xsl:when test="@state='under_construction'"/-->
                        <xsl:when test="@state='running'"/>
                        <xsl:when test="@state='stopped'">
                            <span class="job_chain_error" title="Whole job chain is stopped">
                                <xsl:value-of select="@state"/>
                            </span>
                        </xsl:when>
                        <xsl:otherwise>
                            <span class="job_chain_error">
                                <xsl:value-of select="@state"/>
                            </span>
                        </xsl:otherwise>
                    </xsl:choose>
                </td>

                <td class="right_border" style="text-align: right">
                    <xsl:call-template name="bold_counter">
                        <xsl:with-param name="counter" select="@orders" />
                        <xsl:with-param name="suffix"  select="'orders'" />
                    </xsl:call-template>
                </td>

                <td>
                    <xsl:call-template name="command_menu">
                        <xsl:with-param name="onclick_call"       select="'job_chain_menu__onclick'"/>
                        <xsl:with-param name="onclick_param1_str" select="@path"/>
                        <xsl:with-param name="onclick_param2"     select="'mouse_x() - 25'"/>
                        <xsl:with-param name="onclick_param3"     select="'mouse_y() - 1'"/>
                    </xsl:call-template>
                </td>
            </xsl:element>
        </xsl:if>

        <xsl:if test="file_based/removed or replacement">
            <tr>
                <td></td>
                <td colspan="5" style="padding-left: 4ex; padding-bottom: 0.5em;">
                    <xsl:apply-templates mode="file_based_line" select="."/>
                </td>
            </tr>
        </xsl:if>

        <xsl:if test="/spooler/answer/state/http_server/web_service [ @job_chain = current()/@path ]">
            <tr>
                <td></td>
                <td colspan="5">
                    <span style="font-size: 8pt">
                        <xsl:text>Web_service </xsl:text>
                        <xsl:for-each select="/spooler/answer/state/http_server/web_service [ @job_chain = current()/@path ]">
                            <xsl:value-of select="@name"/>
                            <xsl:text> </xsl:text>
                        </xsl:for-each>
                    </span>
                </td>
            </tr>
        </xsl:if>

        <!--xsl:if test="$single  or  /spooler/@show_job_chain_jobs_checkbox  or  /spooler/@show_job_chain_orders_checkbox and job_chain_node/job/order_queue/order"-->
        <xsl:if test="$single  or  /spooler/@show_job_chain_jobs_checkbox  or  /spooler/@show_job_chain_orders_checkbox and job_chain_node/order_queue/order">

            <!-- $show_orders vergrößert den Abstand zwischen den Job_chain_nodes. Aber nur, wenn überhaupt ein Order in der Jobkette ist -->
            <!--xsl:variable name="show_orders" select="( /spooler/@show_job_chain_orders_checkbox or $single ) and job_chain_node/job/order_queue/order"/-->
            <xsl:variable name="show_orders" select="( /spooler/@show_job_chain_orders_checkbox or $single ) and job_chain_node/order_queue/order"/>

            <xsl:variable name="tr_style">
                <xsl:if test="$show_orders">
                    padding-top: 1ex;
                </xsl:if>
            </xsl:variable>


            <xsl:for-each select="file_order_source">

                <xsl:element name="tr">
                    <xsl:if test="$single">
                        <xsl:attribute name="style">
                            <xsl:value-of select="$tr_style"/>
                        </xsl:attribute>
                    </xsl:if>

                    <xsl:attribute name="class">job_chain</xsl:attribute>

                    <td></td>
                    
                    <td class="label">Source</td>

                    <td colspan="2">
                        <xsl:value-of select="@directory"/>

                        <xsl:if test="@regex != ''">
                            <xsl:text> &#160; </xsl:text>
                            regex=<xsl:value-of select="@regex"/>
                        </xsl:if>
                    </td>

                    <td colspan="2" style="text-align: right">
                        <xsl:if test="files/@count > 0">
                            <xsl:element name="span">
                                <xsl:attribute name="title">
                                    <xsl:text>Snapshot of</xsl:text>
                                    <xsl:value-of select="files/@snapshot_time"/>
                                </xsl:attribute>
                                <xsl:call-template name="bold_counter">
                                    <xsl:with-param name="counter" select="files/@count" />
                                    <xsl:with-param name="suffix"  select="'files'" />
                                </xsl:call-template>
                            </xsl:element>
                        </xsl:if>
                    </td>

                </xsl:element>

                <xsl:if test="ERROR">
                    <tr>
                        <td> </td>
                        <td> </td>
                        <td colspan="5">
                            <xsl:apply-templates select="ERROR"/>
                        </td>
                    </tr>
                </xsl:if>

                <xsl:if test="( /spooler/@show_job_chain_orders_checkbox or $single ) and files/file">
                    <xsl:variable name="limited_files" select="files/file[ position() &lt;= $max_orders ]"/>
                    <xsl:if test="files/@count > count( $limited_files )">
                        <xsl:variable name="remaining" select="files/@count - count( $limited_files )"/>
                        <tr>
                            <td> </td>
                            <td> </td>
                            <td colspan="4">
                                <span style="font-size: 8pt">
                                    <xsl:text>(</xsl:text>
                                    <xsl:value-of select="$remaining"/>
                                    <xsl:text> more files)</xsl:text>
                                </span>
                            </td>
                        </tr>
                    </xsl:if>
                    <xsl:for-each select="$limited_files">
                        <xsl:sort select="-position()" data-type="number"/>
                        <tr>
                            <td> </td>
                            <td> </td>
                            <td colspan="3">
                                <xsl:value-of select="@path"/>
                            </td>
                            <td>
                                <!--xsl:value-of select="@last_write_time__xslt_date_or_time_with_diff"/-->
                            </td>
                        </tr>
                    </xsl:for-each>
                </xsl:if>

                <!--xsl:if test="( /spooler/@show_job_chain_orders_checkbox or $single ) and bad_files/file">
                                <xsl:for-each select="bad_files/file">
                                    <tr>
                                        <td style="padding-left: 2ex"> </td>
                                        <td colspan="4" class="job_chain_error">
                                            <xsl:value-of select="@path"/>
                                        </td>
                                    </tr>
                                    <tr>
                                        <td style="padding-left: 2ex"> </td>
                                        <td colspan="4" class="bad_files_error">
                                            <xsl:apply-templates select="ERROR"/>
                                        </td>
                                    </tr>
                                </xsl:for-each>
                            </xsl:if-->

            </xsl:for-each>


            <xsl:for-each select="job_chain_node[ @job ] | job_chain_node.job_chain">

                <!--xsl:if test="$show_orders and job/order_queue/order"-->
                <xsl:if test="$show_orders and order_queue/order">
                    <tr class="order">
                        <td colspan="6" style="font-size: 4pt">&#160;</td>
                    </tr>
                    <tr class="job">
                        <td colspan="6">
                            <table cellspacing="0" cellpadding="0" width="100%">
                                <tr>
                                    <td class="order">
                                        <!--xsl:apply-templates select="job/order_queue" mode="job_chain_list"-->
                                        <xsl:apply-templates select="order_queue" mode="job_chain_list">
                                            <!--xsl:with-param name="orders"
                                                select="job/order_queue/order[ @job_chain = current()/parent::job_chain/@name  and  @state = current()/@state ]"/>
                                            <xsl:with-param name="max_orders" select="$max_orders"/-->
                                            <xsl:with-param name="orders"
                                                select="order_queue/order[ @job_chain = current()/parent::job_chain/@path  and  @state = current()/@state ]"/>
                                            <xsl:with-param name="max_orders" select="$max_orders"/>
                                        </xsl:apply-templates>
                                    </td>
                                </tr>
                            </table>
                        </td>
                    </tr>
                </xsl:if>

                <xsl:variable name="job" select="/spooler/answer/state/jobs/job[ @path = current()/@job ]"/>

                <xsl:element name="tr">
                    <xsl:if test="$single ">
                        <xsl:attribute name="style">
                            <xsl:value-of select="$tr_style"/>
                        </xsl:attribute>
                    </xsl:if>

                    <xsl:attribute name="class">job</xsl:attribute>

                    <xsl:if test="$job and not( $single )">
                        <xsl:attribute name="style">
                            <!--xsl:text>cursor: default;</xsl:text-->
                            <xsl:value-of select="$tr_style"/>
                        </xsl:attribute>
                        <xsl:attribute name="onmouseover">
                            this.className = "job_hover";
                            this.style.cursor = "pointer";
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className = "job";
                            this.style.cursor = "default";
                        </xsl:attribute>
                        <xsl:attribute name="onclick">
                            call_error_checked( show_job_details, '<xsl:value-of select="@job"/>', '<xsl:value-of select="$job_chain_path"/>' )
                        </xsl:attribute>
                    </xsl:if>

                    <td></td>
                    
                    <td class="right_border">
                        <!--span style="position: relative; left: -6pt; color: #e00000">&#x2022;</span-->

                        <xsl:choose>
                            <xsl:when test="@action='stop'">
                                <span style="color: #e00000" title="Job chain node is stopped (action='stop')">
                                    <xsl:value-of select="@state"/>
                                </span>
                            </xsl:when>

                            <xsl:when test="@action='next_state'">
                                <span title="Orders skip this job chain node (action='next_state')">
                                    <xsl:text>(</xsl:text>
                                    <xsl:value-of select="@state"/>
                                    <xsl:text>)</xsl:text>
                                </span>
                            </xsl:when>

                            <xsl:otherwise>
                                <xsl:value-of select="@state"/>
                            </xsl:otherwise>
                        </xsl:choose>
                    </td>

                    <xsl:choose>
                        <xsl:when test="self::job_chain_node/@job">
                            <td class="right_border">
                                <xsl:apply-templates mode="bold_job_path" select="@job"/>
                            </td>

                            <td class="right_border">
                                <xsl:choose>
                                    <xsl:when test="$job/file_based/ERROR">
                                        <xsl:apply-templates mode="file_based_error" select="$job/file_based"/>
                                    </xsl:when>
                                    <xsl:when test="not( $job )">
                                        <span class="file_based_error">missing</span>
                                    </xsl:when>
                                    <xsl:otherwise>
                                        <xsl:apply-templates mode="job_state_line" select="$job" />

                                        <xsl:choose>
                                            <xsl:when test="$job/tasks/@count &gt; 0">
                                                <xsl:text>, </xsl:text>
                                                <span class="task" style="padding-left: 2pt; padding-right: 2pt; padding-bottom: 2pt;">
                                                    <xsl:variable name="job_chain_task_count" select="count( $job/tasks/task[ order/@job_chain=$job_chain_path ] )"/>
                                                    <xsl:element name="span">
                                                        <xsl:attribute name="style">white-space: nowrap</xsl:attribute>
                                                        <xsl:attribute name="title">
                                                            <xsl:value-of select="$job_chain_task_count"/> tasks processing orders from job chain <xsl:value-of select="$job_chain_path"/>
                                                        </xsl:attribute>
                                                        <xsl:call-template name="bold_counter">
                                                            <xsl:with-param name="counter" select="$job_chain_task_count" />
                                                            <xsl:with-param name="suffix" select="'working tasks'" />
                                                        </xsl:call-template>
                                                    </xsl:element>

                                                    <xsl:variable name="waiting_task_count" select="count( $job/tasks/task[ @state='running_waiting_for_order' ] )"/>
                                                    <xsl:if test="$waiting_task_count &gt; 0">
                                                        <xsl:text>, </xsl:text>
                                                        <span style="white-space: nowrap">
                                                            <xsl:value-of select="$waiting_task_count"/>
                                                            <xsl:text> idle</xsl:text>
                                                        </span>
                                                    </xsl:if>

                                                    <xsl:variable name="rest" select="$job/tasks/@count - $job_chain_task_count - $waiting_task_count"/>
                                                    <xsl:if test="$rest &gt; 0">
                                                        <xsl:text>, </xsl:text>
                                                        <span style="white-space: nowrap">
                                                            <xsl:value-of select="$rest"/> others
                                                        </span>
                                                    </xsl:if>
                                                </span>
                                            </xsl:when>
                                            <xsl:otherwise>
                                                <xsl:if test="$job/@next_start_time">
                                                    <xsl:text>, </xsl:text>
                                                    <span style="white-space: nowrap">
                                                        <xsl:value-of select="$job/@next_start_time__xslt_date_or_time_with_diff"/>
                                                    </span>
                                                </xsl:if>
                                                <!--
                                                <xsl:if test="$job/order_queue/@next_start_time">
                                                    <xsl:text>, </xsl:text>
                                                    <span style="white-space: nowrap">
                                                        <xsl:value-of select="$job/order_queue/@next_start_time__xslt_date_or_time_with_diff"/>
                                                    </span>
                                                </xsl:if>
                                                -->
                                            </xsl:otherwise>
                                        </xsl:choose>
                                    </xsl:otherwise>
                                </xsl:choose>
                            </td>

                            <td class="right_border" style="text-align: right">
                                <xsl:if test="order_queue">
                                    <xsl:call-template name="bold_counter">
                                        <xsl:with-param name="counter" select="order_queue/@length" />
                                        <xsl:with-param name="suffix"  select="'orders'" />
                                    </xsl:call-template>
                                </xsl:if>
                            </td>

                            <td>
                                <xsl:call-template name="command_menu">
                                    <xsl:with-param name="onclick_call"       select="'job_chain_node_menu__onclick'"/>
                                    <xsl:with-param name="onclick_param1_str" select="parent::job_chain/@path"/>
                                    <xsl:with-param name="onclick_param2_str" select="@state"/>
                                    <xsl:with-param name="onclick_param3"     select="'mouse_x() - 50'"/>
                                    <xsl:with-param name="onclick_param4"     select="'mouse_y() - 1'"/>
                                </xsl:call-template>
                            </td>
                        </xsl:when>
                        
                        <xsl:when test="self::job_chain_node.job_chain">
                            <td colspan="4">
                                <span class="label" style="padding-right: 2pt;">Job_chain </span>
                                <xsl:value-of select="@job_chain"/>
                            </td>
                        </xsl:when>
                    </xsl:choose>
                </xsl:element>

                <xsl:if test="$job/file_based/removed or $job/replacement">
                    <tr class="job">
                        <td colspan="6" style="padding-left: 4ex; padding-bottom: 0.5em;">
                            <xsl:apply-templates mode="file_based_line" select="$job"/>
                        </td>
                    </tr>
                </xsl:if>
                
                <!--xsl:apply-templates select="job/tasks/task" mode="job_chain_list"/-->

            </xsl:for-each>

            <xsl:if test="blacklist/@count >= 1">

                <tr>
                    <td colspan="6" style="line-height: 1ex">&#160;</td>
                </tr>

                <xsl:element name="tr">
                    <xsl:attribute name="class">job_chain</xsl:attribute>

                    <td></td>
                    
                    <td colspan="3">
                        <b>Blacklist</b>
                    </td>
                    
                    <td colspan="2">
                        <xsl:call-template name="bold_counter">
                            <xsl:with-param name="counter" select="blacklist/@count" />
                            <xsl:with-param name="suffix"  select="'orders'" />
                        </xsl:call-template>
                    </td>
                </xsl:element>

                <xsl:if test="/spooler/@show_job_chain_orders_checkbox or $single">
                    <tr class="order">
                        <td colspan="6">
                            <xsl:apply-templates select="blacklist" mode="job_chain_list">
                                <xsl:with-param name="orders" select="blacklist/order[ @job_chain = current()/@path ]"/>
                                <xsl:with-param name="max_orders" select="$max_orders"/>
                            </xsl:apply-templates>
                        </td>
                    </tr>
                </xsl:if>
            </xsl:if>
        </xsl:if>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~bold_job_path-->
    
    <xsl:template mode="bold_job_path" match="@*">

        <xsl:variable name="job" select="/spooler/answer/state/jobs/job[ @path=current() ]"/>
        
        <xsl:element name="span">
            <xsl:choose>
                <xsl:when test="not( $job )">
                    <xsl:attribute name="class">job_missing</xsl:attribute>
                </xsl:when>
                <xsl:when test="@remove='yes'">
                    <xsl:attribute name="class">job_remove</xsl:attribute>
                </xsl:when>
                <xsl:when test="$job/@state='not_initialized' or $job/@state='initialized' or $job/@state='loaded'">
                    <xsl:attribute name="class">job_not_initialized</xsl:attribute>
                </xsl:when>
                <xsl:when test="$job/@state='pending' or $job/@state='running'">
                </xsl:when>
            </xsl:choose>
            
            <xsl:attribute name="style">font-weight: bold;</xsl:attribute>

            <xsl:if test="$job/@state='not_initialized' or $job/@state='stopped' or $job/@state='read_error' or $job/@state='error'">
                <xsl:attribute name="class">job_error</xsl:attribute>
            </xsl:if>
            
            <xsl:value-of select="."/>
        </xsl:element>
        
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~job_state_line-->

    <xsl:template mode="job_state_line" match="job">

        <xsl:apply-templates select="@state"/>

        <xsl:if test="@waiting_for_process='yes'">
            <xsl:text>, </xsl:text>
            <span style="color: #e00000; font-weight: bold">needs process</span>
        </xsl:if>

        <xsl:if test="lock.requestor">
            <xsl:text>, </xsl:text>

            <xsl:choose>
                <xsl:when test="lock.requestor[ @enqueued='yes' ]">
                    <span style="color: #e00000; font-weight: bold;">
                        <xsl:text>needs lock </xsl:text>
                    </span>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>lock </xsl:text>
                </xsl:otherwise>
            </xsl:choose>

            <xsl:if test="lock.requestor/lock.use">
                <xsl:for-each select="lock.requestor/lock.use">
                    <xsl:if test="position()>1">
                        <xsl:text>, </xsl:text>
                    </xsl:if>

                    <xsl:apply-templates select="." mode="short"/>
                </xsl:for-each>
            </xsl:if>
        </xsl:if>

        <!--xsl:if test="replacement">
            <xsl:choose>
                <xsl:when test="replacement/job/file_based/ERROR">
                    <xsl:text>, </xsl:text>
                    <span class="file_based_error">changed file has error</span>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>, is being replaced</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:if-->
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~lock.use-->

    <xsl:template match="lock.use" mode="short">

        <xsl:element name="span">
            <xsl:if test="@is_available='no'">
                <xsl:attribute name="class">lock_locked</xsl:attribute>
                <xsl:attribute name="title">Lock is not available, it is locked</xsl:attribute>
                <!--xsl:attribute name="style">cursor: default;</xsl:attribute-->
            </xsl:if>

            <xsl:if test="@is_available='yes'">
                <xsl:attribute name="class">lock_free</xsl:attribute>
                <xsl:attribute name="title">Lock is available</xsl:attribute>
            </xsl:if>

            <xsl:element name="span">
                <xsl:if test="@is_missing='yes'">
                    <xsl:attribute name="class">lock_missing</xsl:attribute>
                    <xsl:attribute name="title">Lock is missing</xsl:attribute>
                </xsl:if>

                <xsl:value-of select="@lock"/>
            </xsl:element>

            <xsl:if test="@exclusive='no'">
                <span class="small"> non-excl.</span>
            </xsl:if>
        </xsl:element>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~lock.use-->
    <!-- <locks> ist im rechten Fenster nicht bekannt, nur <job>.-->

    <!--xsl:template match="lock.use" mode="long">

        <xsl:apply-templates select="." mode="short"/>

        <xsl:variable name="lock" select="/spooler/answer/state/locks/lock [ @name = current()/@lock ]"/>

        <xsl:choose>
            <xsl:when test="$lock/lock.holders/lock.holder">
                <xsl:text> </xsl:text>
                <xsl:choose>
                    <xsl:when test="lock.holders/@exclusive='no'">
                        <xsl:value-of select="count( lock.holders/lock.holder )"/>
                        <xsl:text> non-exclusive holders</xsl:text>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:text> exclusively held</xsl:text>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text> (free)</xsl:text>
            </xsl:otherwise>

        </xsl:choose>

    </xsl:template-->

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Order in Job chain list-->

    <xsl:template match="order_queue | blacklist" mode="job_chain_list">
        <xsl:param name="max_orders" select="100"/>
        <xsl:param name="orders"/>

        <xsl:param name="limited_orders" select="$orders[ position() &lt;= $max_orders ]"/>

        <xsl:if test="@length > count( $limited_orders )">
            <xsl:variable name="remaining" select="@length - count( $limited_orders )"/>
            <span style="margin-left: 0.5ex; font-size: 8pt">
                <xsl:text>(</xsl:text>
                <xsl:value-of select="$remaining"/>
                <xsl:text> other orders)</xsl:text>
            </span>
        </xsl:if>

        <table cellspacing="0" cellpadding="0" width="100%">
            <col valign="top"/>
            <col valign="top"/>
            <col valign="top"/>
            <col valign="top" align="right"/>

            <xsl:for-each select="$limited_orders">
                <xsl:sort select="-position()" data-type="number"/>

                <!--tr class="order">
                    <td colspan="2"></td>
                    <td colspan="99" style="border-top: 1px solid lightgrey; line-height: 0px">&#160;</td>
                </tr-->
                <xsl:variable name="order_style">
                    <xsl:if test="@removed">text-decoration: line-through; </xsl:if>
                    <xsl:if test="@replacement">font-style: italic; </xsl:if>
                    <xsl:if test="@suspended">color: gray; </xsl:if>
                </xsl:variable>

                <xsl:element name="tr">
                    <xsl:attribute name="class">order</xsl:attribute>

                    <xsl:element name="td">
                        <xsl:attribute name="style">
                            white-space: nowrap;
                            <xsl:if test="@task or @occupied_by_cluster_member_id">font-weight: bold;</xsl:if>
                        </xsl:attribute>

                        <!--xsl:if test="@replacement">
                            <span style="cursor: pointer; position: absolute" title="This order is going to replace a currently processed order with same ID">
                                r
                            </span>
                        </xsl:if-->

                        <!--xsl:if test="@suspended">
                            <span style="cursor: pointer; position: absolute" title="Order is suspended">
                                s
                            </span>
                        </xsl:if-->

                        <xsl:element name="span">
                            <xsl:attribute name="style">
                                <xsl:value-of select="$order_style"/>
                            </xsl:attribute>

                            <xsl:value-of select="@id"/>&#160;
                        </xsl:element>
                    </xsl:element>

                    <xsl:element name="td">
                        <xsl:attribute name="align">left</xsl:attribute>
                        <xsl:attribute name="style">
                            <!--xsl:if test="@task or @occupied_by_cluster_member_id">font-weight: bold;</xsl:if-->
                            <xsl:text>padding-left: 2ex;</xsl:text>
                            <xsl:value-of select="$order_style"/>
                        </xsl:attribute>

                        <xsl:value-of select="@title"/>

                        <xsl:if test="@removed">
                            <xsl:text> &#160; </xsl:text>
                            <span style="font-size: 8pt;">
                                <xsl:text>(removed)</xsl:text>
                            </span>
                        </xsl:if>

                        <xsl:if test="@replacement">
                            <xsl:text> &#160; </xsl:text>
                            <xsl:element name="span">
                                <xsl:attribute name="style">font-size: 8pt;</xsl:attribute>
                                <xsl:attribute name="title">
                                    <xsl:text>This order is a replacement for another order with the same ID </xsl:text>
                                    <xsl:choose>
                                        <xsl:when test="@replaced_order_occupator">
                                            <xsl:text>currently processed by </xsl:text>
                                            <xsl:value-of select="@replaced_order_occupator"/>
                                        </xsl:when>
                                    </xsl:choose>
                                </xsl:attribute>

                                <xsl:text>(replacement)</xsl:text>
                            </xsl:element>
                        </xsl:if>

                        <xsl:if test="@suspended">
                            <xsl:text> &#160; </xsl:text>
                            <span style="font-size: 8pt;">
                                <xsl:text>(suspended)</xsl:text>
                            </span>
                        </xsl:if>
                        <xsl:if test="@setback">
                            <xsl:text> &#160; </xsl:text>
                            <span style="font-size: 8pt;">
                                <xsl:text>(set&#160;back)</xsl:text>
                            </span>
                        </xsl:if>
                    </xsl:element>

                    <td style="text-align: right">
                        <xsl:if test="@web_service_client">
                            <span class="web_service" style="white-space: nowrap; font-size: 8pt; margin-right: 1ex;">
                                <xsl:element name="span">
                                    <!--xsl:attribute name="style">cursor: default; </xsl:attribute-->
                                    <xsl:attribute name="title">
                                        <xsl:text>Web_service_operation </xsl:text>
                                        <xsl:value-of select="@web_service"/>
                                        <xsl:text>:</xsl:text>
                                        <xsl:value-of select="@web_service_operation"/>
                                    </xsl:attribute>
                                    <xsl:value-of select="@web_service_client"/>
                                </xsl:element>
                            </span>
                            <xsl:text> </xsl:text>
                        </xsl:if>
                        
                        <xsl:choose>
                            <xsl:when test="@task">
                                <span class="task" style="white-space: nowrap; font-size: 8pt;">
                                    Task <xsl:value-of select="@task"/>
                                </span>
                            </xsl:when>

                            <xsl:when test="@occupied_by_cluster_member_id">
                                <xsl:element name="span">
                                    <xsl:attribute name="class">cluster</xsl:attribute>
                                    <xsl:attribute name="style">white-space: nowrap; font-size: 8pt;</xsl:attribute>
                                    <xsl:attribute name="title">
                                        <xsl:text>Order is processed by Scheduler member </xsl:text>
                                        <xsl:value-of select="@occupied_by_cluster_member_id"/>
                                    </xsl:attribute>

                                    <xsl:choose>
                                        <xsl:when test="@occupied_by_http_url">
                                            <xsl:element name="a">
                                                <xsl:attribute name="href">
                                                    <xsl:call-template name="scheduler_url">
                                                        <xsl:with-param name="url" select="@occupied_by_http_url"/>
                                                    </xsl:call-template>
                                                </xsl:attribute>
                                                <xsl:attribute name="target">
                                                    <xsl:call-template name="translate_target">
                                                        <xsl:with-param name="target" select="@occupied_by_http_url"/>
                                                    </xsl:call-template>
                                                </xsl:attribute>
                                                <xsl:choose>
                                                    <xsl:when test="contains( @occupied_by_http_url, 'http://' )">
                                                        <xsl:value-of select="substring-after( @occupied_by_http_url, 'http://' )"/>
                                                    </xsl:when>
                                                    <xsl:otherwise>
                                                        <xsl:value-of select="@occupied_by_cluster_member_id"/>
                                                    </xsl:otherwise>
                                                </xsl:choose>
                                            </xsl:element>
                                        </xsl:when>
                                        <xsl:otherwise>
                                            <xsl:value-of select="@occupied_by_cluster_member_id"/>
                                        </xsl:otherwise>
                                    </xsl:choose>
                                </xsl:element>
                            </xsl:when>

                            <xsl:otherwise>
                                <xsl:if test="@next_start_time">
                                    <span class="small" style="white-space: nowrap;" title="Start time">
                                        <xsl:value-of select="@next_start_time__xslt_date_or_time_with_diff"/>
                                    </span>
                                </xsl:if>
                            </xsl:otherwise>
                        </xsl:choose>
                    </td>

                    <td style="text-align: right">
                        <xsl:call-template name="command_menu">
                            <xsl:with-param name="onclick_call"         select="'order_menu__onclick'"/>
                            <xsl:with-param name="onclick_param1_str"   select="@job_chain"/>
                            <xsl:with-param name="onclick_param2_str"   select="@id"/>
                            <xsl:with-param name="onclick_param3"       select="'mouse_x() - 70'"/>
                            <xsl:with-param name="onclick_param4"       select="'mouse_y() - 1'"/>
                        </xsl:call-template>
                    </td>

                </xsl:element>

                
                <xsl:if test="file_based/removed or replacement">
                    <tr>
                        <td colspan="4" style="padding-left: 4ex; padding-bottom: 0.5em;">
                            <xsl:apply-templates mode="file_based_line" select="."/>
                        </td>
                    </tr>
                </xsl:if>

            </xsl:for-each>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~locks-->

    <xsl:template match="locks">
        <table width="100%" cellpadding="0" cellspacing="0" class="lock">

            <xsl:for-each select="lock">
                <tr>
                    <td style="line-height: 5pt">&#160;</td>
                </tr>

                <tr>
                    <td style="padding-right: 2ex; width: 10ex">
                        <xsl:element name="span">
                            <xsl:choose>
                                <xsl:when test="lock.holders/lock.holder">
                                    <xsl:attribute name="class">lock_locked</xsl:attribute>
                                </xsl:when>
                                <xsl:otherwise>
                                    <xsl:attribute name="class">lock_free</xsl:attribute>
                                </xsl:otherwise>
                            </xsl:choose>

                            <xsl:attribute name="style">
                                <xsl:text>font-weight: bold; white-space: nowrap;</xsl:text>
                                <xsl:choose>
                                    <xsl:when test="@state='not_initialized'">color: gray;</xsl:when>
                                    <xsl:when test="@state='removing'">text-decoration: line-through;</xsl:when>
                                </xsl:choose>
                            </xsl:attribute>

                            <xsl:value-of select="@path"/>
                        </xsl:element>
                    </td>

                    <td>
                        <xsl:choose>
                            <xsl:when test="lock.holders/lock.holder">
                                <span class="lock_locked">
                                    <xsl:choose>
                                        <xsl:when test="lock.holders/@exclusive='no'">
                                            <!--xsl:value-of select="count( lock.holders/lock.holder )"/-->
                                            <xsl:text> non-exclusively locked</xsl:text>
                                        </xsl:when>
                                        <xsl:otherwise>
                                            <xsl:text> locked</xsl:text>
                                        </xsl:otherwise>
                                    </xsl:choose>
                                </span>
                            </xsl:when>
                            <xsl:when test="file_based/ERROR">
                                 <xsl:apply-templates mode="file_based_error" select="file_based"/>
                            </xsl:when>
                            <xsl:when test="@is_free='yes'">
                                <span>free</span>
                            </xsl:when>
                        </xsl:choose>
                        <!--xsl:if test="lock.holders/lock.holder">
                            <xsl:value-of select="count( lock.holders/lock.holder )"/>
                            <xsl:choose>
                                <xsl:when test="lock.holders/@exclusive='no'">
                                    <xsl:text> non-exclusive holders</xsl:text>
                                </xsl:when>
                                <xsl:otherwise>
                                    <xsl:text> exclusive holders</xsl:text>
                                </xsl:otherwise>
                            </xsl:choose>
                        </xsl:if>

                        <xsl:if test="lock.queue [ @exclusive='yes' ]/lock.queue.entry">
                            <xsl:text>, </xsl:text>
                            <xsl:value-of select="count( lock.queue[ @exclusive='yes' ]/lock.queue.entry )"/>
                            <xsl:text> exclusive waiting</xsl:text>
                        </xsl:if>

                        <xsl:if test="lock.queue [ @exclusive='no' ]/lock.queue.entry">
                            <xsl:text>, </xsl:text>
                            <xsl:value-of select="count( lock.queue[ @exclusive='no' ]/lock.queue.entry )"/>
                            <xsl:text> non-exclusive waiting</xsl:text>
                        </xsl:if-->
                    </td>
                </tr>

                <xsl:if test="file_based/removed or replacement">
                    <tr>
                        <td colspan="5" style="padding-left: 4ex; padding-bottom: 0.5em;">
                            <xsl:apply-templates mode="file_based_line" select="."/>
                        </td>
                    </tr>
                </xsl:if>

                <tr>
                    <td colspan="5" style="padding-left: 4ex">
                        <xsl:if test="lock.holders/lock.holder">
                            <div class="label">
                                <xsl:choose>
                                    <xsl:when test="lock.holders/@exclusive='no'">
                                        <!--xsl:value-of select="count( lock.holders/lock.holder )"/-->
                                        <xsl:text>Holders (non-exclusive)</xsl:text>
                                    </xsl:when>
                                    <xsl:otherwise>
                                        <xsl:text>Holder (exclusive)</xsl:text>
                                    </xsl:otherwise>
                                </xsl:choose>
                            </div>

                            <xsl:apply-templates select="lock.holders"/>
                        </xsl:if>

                        <xsl:if test="lock.queue [ not( @exclusive='no' ) ]/lock.queue.entry">
                            <div class="label" style="margin-top: 5pt;">
                                <!--xsl:value-of select="count( lock.queue[ not( @exclusive='no' ) ]/lock.queue.entry )"/-->
                                <xsl:text>Waiting jobs (exclusive)</xsl:text>
                            </div>

                            <xsl:apply-templates select="lock.queue [ not( @exclusive='no' ) ] "/>
                        </xsl:if>

                        <xsl:if test="lock.queue [ @exclusive='no' ]/lock.queue.entry">
                            <div class="label" style="margin-top: 5pt;">
                                <!--xsl:value-of select="count( lock.queue[ @exclusive='no' ]/lock.queue.entry )"/-->
                                <xsl:text>Waiting jobs (non-exclusive)</xsl:text>
                            </div>

                            <xsl:apply-templates select="lock.queue [ @exclusive='no' ]"/>
                        </xsl:if>
                    </td>
                </tr>
            </xsl:for-each>

        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~bold_lock_name-->
    
    <!--xsl:template mode="bold_lock_name" match="node()">

        (<xsl:variable name="lock" select="/spooler/answer/state/locks/lock [ @name=current() ]"/>

        <xsl:element name="span">
            <xsl:attribute name="style">
                font-weight: bold;
                <xsl:choose>
                    <xsl:when test="$lock/@state='not_initialized'">color: gray;</xsl:when>
                    <xsl:when test="$lock/@state='removing'">text-decoration: line-through;</xsl:when>
                </xsl:choose>
            </xsl:attribute>

            <xsl:value-of select="."/>*
        </xsl:element>)

    </xsl:template-->
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~lock.queue-->

    <xsl:template match="lock.queue">
        <table width="100%" cellpadding="0" cellspacing="0" class="lock">

            <xsl:for-each select="lock.queue.entry">
                <xsl:variable name="tr_id" select="concat( 'scheduler_tr_lock_', parent::lock/@path, '_job_', @job )"/>
                <xsl:element name="tr">
                    <xsl:attribute name="id">
                        <xsl:value-of select="$tr_id"/>
                    </xsl:attribute>

                    <xsl:element name="td">
                        <xsl:attribute name="class"  >job</xsl:attribute>
                        <xsl:attribute name="style"  >cursor: pointer; </xsl:attribute>
                        <xsl:attribute name="onclick">
                            call_error_checked( show_job_details, '<xsl:value-of select="@job"/>' )
                        </xsl:attribute>
                        <xsl:attribute name="onmouseover">
                            this.className = document.getElementById( "<xsl:value-of select="$tr_id"/>" ).className = "job_hover";
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className = document.getElementById( "<xsl:value-of select="$tr_id"/>" ).className = "job"
                        </xsl:attribute>

                        <xsl:text>Job </xsl:text>
                        <xsl:value-of select="@job"/>
                    </xsl:element>

                    <xsl:variable name="job" select="/spooler/answer/state/jobs/job[ @path = current()/@job ]"/>
                    
                    <td class="job">
                        <xsl:apply-templates mode="job_state_line" select="$job" />
                    </td>
                </xsl:element>

            </xsl:for-each>

        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~lock.queue-->

    <xsl:template match="lock.holders">
        <table width="100%" cellpadding="0" cellspacing="0" class="lock">

            <xsl:for-each select="lock.holder">
                <xsl:variable name="tr_id" select="concat( 'scheduler_tr_lock_', parent::lock.holders/parent::lock/@path, '_task_', @job, ':', @task )"/>
                <xsl:element name="tr">
                    <xsl:attribute name="id">
                        <xsl:value-of select="$tr_id"/>
                    </xsl:attribute>

                    <xsl:element name="td">
                        <xsl:attribute name="class"  >task</xsl:attribute>
                        <xsl:attribute name="style"  >cursor: pointer; </xsl:attribute>
                        <xsl:attribute name="onclick">
                            call_error_checked( show_task_details, '<xsl:value-of select="@job"/>', '<xsl:value-of select="@task"/>' )
                        </xsl:attribute>
                        <xsl:attribute name="onmouseover">
                            this.className = document.getElementById( "<xsl:value-of select="$tr_id"/>" ).className = "task_hover";
                        </xsl:attribute>
                        <xsl:attribute name="onmouseout" >
                            this.className = document.getElementById( "<xsl:value-of select="$tr_id"/>" ).className = "task"
                        </xsl:attribute>

                        <span style="padding-right: 2ex">
                            <xsl:text>Task </xsl:text>
                            <xsl:value-of select="@job"/>:<xsl:value-of select="@task"/>
                        </span>
                        <span>
                            <xsl:apply-templates select="/spooler/answer/state/jobs/job [ @path=current()/@job ]/tasks/task[ @task=current()/@task ]" mode="task_line"/>
                        </span>
                    </xsl:element>
                </xsl:element>
            </xsl:for-each>

        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~process_classes-->

    <xsl:template match="process_classes">
        <table width="100%" cellpadding="0" cellspacing="0" class="process_class">
            <!--caption align="left" class="process_class">
                <b>Process classes</b>
            </caption-->

            <col width=" 50"/>
            <col width=" 150"/>
            <!--col width=" 150"/-->
            <col width="$datetime_column_width"/>
            <col width=" 10" align="right"/>
            <col width=" 10" align="right"/>
            <col width="*"/>

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
                                    <xsl:when test="@path!=''">
                                        <xsl:value-of select="@path"/>
                                    </xsl:when>
                                    <xsl:otherwise>
                                        (default)
                                    </xsl:otherwise>
                                </xsl:choose>
                            </b>

                            <xsl:text>&#160; </xsl:text>
                            <xsl:choose>
                                <xsl:when test="file_based/ERROR">
                                    <xsl:apply-templates mode="file_based_error" select="file_based"/>
                                </xsl:when>
                                <xsl:otherwise>
                                    max_processes=<xsl:value-of select="@max_processes"/>
                                </xsl:otherwise>
                            </xsl:choose>
                        </td>
                    </tr>

                    <xsl:if test="file_based/removed or replacement">
                        <tr>
                            <td colspan="6" style="padding-left: 4ex; padding-bottom: 0.5em;">
                                <xsl:apply-templates mode="file_based_line" select="."/>
                            </td>
                        </tr>
                    </xsl:if>
                    
                    <xsl:for-each select="processes/process">
                        <tr>
                            <td style="padding-left: 2ex">
                                <xsl:value-of select="@pid"/>
                            </td>
                            <td class="task">
                                <xsl:value-of select="@job"/>
                                <xsl:text>:</xsl:text>
                                <xsl:value-of select="@task_id"/>
                            </td>
                            <td style="white-space: nowrap">
                                <xsl:value-of select="@running_since__xslt_datetime_with_diff" disable-output-escaping="yes"/>
                            </td>
                            <td class="small">
                                <xsl:value-of select="@operations"/>
                            </td>
                            <td class="small">
                                <xsl:value-of select="@callbacks"/>
                            </td>
                            <td class="small">
                                <xsl:value-of select="@operation"/>
                            </td>
                        </tr>
                    </xsl:for-each>

                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~schedules-->

    <xsl:template match="schedules">
        <table width="100%" cellpadding="0" cellspacing="0" class="schedules">
            <col width="100"/>
            <col width="$datetime_column_width"/>
            <col width="$datetime_column_width"/>
            <col width="*"/>

            <thead>
                <xsl:call-template name="card_top"/>
                <tr style="">
                    <td class="head1" style="padding-left: 2ex">Schedule</td>
                    <td class="head">valid from</td>
                    <td class="head">valid to</td>
                    <td class="head">state</td>
                </tr>
                <tr>
                    <td colspan="4" class="after_head_space">&#160;</td>
                </tr>
            </thead>

            <tbody>
                <xsl:for-each select="schedule">
                    <xsl:sort select="translate( @path, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz' )"/>
                    
                    <xsl:if test="not( /spooler/answer/state/schedules/schedule [ @path = current()/@substitute ] ) ">

                        <tr>
                            <td colspan="4">
                                <p style="margin: 0px; line-height: 50%">&#160;</p>
                            </td>
                        </tr>

                        <tr>
                            <td colspan="3">
                                <b>
                                    <xsl:value-of select="@path"/>
                                </b>
                            </td>
                            <td>
                                <xsl:apply-templates select="file_based/@state"/>
                            </td>
                        </tr>

                        <xsl:if test="file_based/ERROR or file_based/removed or replacement or file_based/requisites/requisite [ @is_missing='yes' ]">
                            <tr>
                                <td colspan="4" style="padding-left: 4ex; padding-bottom: 0.5em;">
                                    <xsl:apply-templates mode="file_based_line" select="."/>
                                </td>
                            </tr>
                        </xsl:if>

                        <xsl:for-each select="parent::schedules/schedule [ @substitute = current()/@path ]">
                            <tr>
                                <td style="padding-left: 2ex">
                                    <xsl:value-of select="@path"/>
                                </td>
                                <td>
                                    <xsl:value-of select="@valid_from"/>
                                </td>
                                <td>
                                    <xsl:value-of select="@valid_to"/>
                                </td>
                                <td>
                                    <xsl:apply-templates select="file_based/@state"/>
                                </td>
                            </tr>
                            
                            <xsl:if test="file_based/ERROR or file_based/removed or replacement or file_based/requisites/requisite [ @is_missing='yes' ]">
                                <tr>
                                    <td colspan="4" style="padding-left: 4ex; padding-bottom: 0.5em;">
                                        <xsl:apply-templates mode="file_based_line" select="."/>
                                    </td>
                                </tr>
                            </xsl:if>
                        </xsl:for-each>
                    </xsl:if>
                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~cluster-->

    <xsl:template match="cluster">
        <table cellpadding="0" cellspacing="0" width="100%" class="cluster">
            <thead>
                <xsl:call-template name="card_top"/>

                <!--tr>
                    <td colspan="99">
                        <xsl:value-of select="@count" /> schedulers,
                        <xsl:value-of select="@connected" /> connected
                    </td>
                </tr-->
                <tr>
                    <td colspan="99">
                        <span style="margin-right: 1em">
                            <xsl:call-template name="bold_counter">
                                <xsl:with-param name="counter" select="count( cluster_member [ @active='yes' ] )" />
                                <xsl:with-param name="suffix" select="'active Schedulers. '" />
                            </xsl:call-template>
                        </span>

                        <span style="margin-right: 1em">
                            <xsl:call-template name="bold_counter">
                                <xsl:with-param name="counter" select="count( cluster_member [ @exclusive='yes' ] )" />
                                <xsl:with-param name="suffix" select="'exclusive Schedulers. '" />
                            </xsl:call-template>
                        </span>

                        <xsl:if test="@active='yes'">
                            <span style="margin-right: 1em; color: green; font-weight: bold;">
                                <xsl:text>This Scheduler is active</xsl:text>

                                <xsl:if test="@exclusive='yes'">
                                    <xsl:text> and exclusive</xsl:text>
                                </xsl:if>

                                <xsl:text>. </xsl:text>
                            </span>
                        </xsl:if>

                        <xsl:if test="@backup='yes' and not( @is_backup_allowed_to_start='yes' )">
                            <span style="margin-right: 1em; color: #e00000">Only non-backup Schedulers are allowed to start operation.</span>
                        </xsl:if>
                    </td>
                </tr>
                <tr>
                    <td style="line-height: 5pt">&#160;</td>
                </tr>

                <tr>
                    <td class="head1" style="font-size: 8pt;">URL</td>
                    <td class="head"  style="font-size: 8pt; width: 20ex">Started at</td>
                    <td class="head"  style="font-size: 8pt;">State</td>
                    <td class="head"  style="font-size: 8pt;">Pid</td>
                    <td class="head"  style="font-size: 8pt;">Last heart beat</td>
                    <td class="head"  style="font-size: 8pt;">
                        Detected<br/>heart beats
                    </td>
                    <td class="head"  style="font-size: 8pt;" colspan="2">
                        Backup<br/>precedence
                    </td>
                </tr>

                <tr>
                    <td colspan="99" class="after_head_space">&#160;</td>
                </tr>
            </thead>

            <tbody>
                <xsl:apply-templates select="cluster_member">
                    <xsl:sort select="@http_url"/>
                    <xsl:sort select="@dead" data-type="number"/>
                    <xsl:sort select="@heart_beat_count = 0"/>
                </xsl:apply-templates>
            </tbody>

        </table>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~cluster_member-->

    <xsl:template match="cluster_member">

        <xsl:element name="tr">
            <xsl:attribute name="style">
                <!--xsl:text>cursor: default;</xsl:text-->
                <xsl:choose>
                    <xsl:when test="@heart_beat_count=0">color: gray;</xsl:when>
                    <xsl:when test="@dead='yes'">color: #e00000;</xsl:when>
                    <xsl:when test="@active='yes'">color: green;</xsl:when>
                </xsl:choose>
            </xsl:attribute>

            <xsl:attribute name="onmouseover">this.original_class_name = this.className;  this.className='remote_scheduler_hover'</xsl:attribute>
            <xsl:attribute name="onmouseout" >this.className=this.original_class_name </xsl:attribute>

            <td>
                <xsl:choose>
                    <xsl:when test="not( @dead='yes' ) and @cluster_member_id != ancestor::cluster/@cluster_member_id">
                        <xsl:element name="a">
                            <xsl:attribute name="class">no_underline</xsl:attribute>

                            <xsl:attribute name="style">
                                <xsl:text>cursor: pointer;</xsl:text>
                            </xsl:attribute>

                            <xsl:attribute name="href">
                                <xsl:call-template name="scheduler_url">
                                    <xsl:with-param name="url" select="@http_url"/>
                                </xsl:call-template>
                            </xsl:attribute>

                            <xsl:attribute name="target">
                                <xsl:call-template name="translate_target">
                                    <xsl:with-param name="target" select="@http_url"/>
                                </xsl:call-template>
                            </xsl:attribute>

                            <xsl:apply-templates mode="cluster_member_http_url" select="."/>

                        </xsl:element>
                    </xsl:when>

                    <xsl:otherwise>
                        <xsl:element name="span">
                            <xsl:attribute name="style">
                                <xsl:choose>
                                    <xsl:when test="@dead='yes'">text-decoration: line-through;</xsl:when>
                                    <xsl:otherwise              >font-weight: bold;</xsl:otherwise>
                                </xsl:choose>
                            </xsl:attribute>

                            <xsl:apply-templates mode="cluster_member_http_url" select="."/>

                        </xsl:element>
                    </xsl:otherwise>
                </xsl:choose>

                <xsl:text> </xsl:text>
            </td>

            <td>
                <xsl:value-of select="@running_since__xslt_date_or_time_with_diff"/>
            </td>

            <td>
                <xsl:choose>
                    <xsl:when test="@active='yes'">
                        <xsl:choose>
                            <xsl:when test="@dead='yes'">
                                <span style="margin-right: 1ex;">(was active!) </span>
                            </xsl:when>
                            <xsl:otherwise>
                                <span style="margin-right: 1ex; font-weight: bold">active </span>
                            </xsl:otherwise>
                        </xsl:choose>
                    </xsl:when>
                    <xsl:when test="not( @dead='yes' )">
                        <span style="margin-right: 1ex;">inactive </span>
                    </xsl:when>
                </xsl:choose>

                <xsl:if test="@distributed_orders='yes'">
                    <xsl:element name="span">
                        <xsl:attribute name="style">
                            <xsl:text>margin-right: 1ex; white-space: nowrap; </xsl:text>
                            <xsl:if test="not( @dead='yes' )">font-weight: bold; </xsl:if>
                        </xsl:attribute>
                        <xsl:text>distributed orders </xsl:text>
                    </xsl:element>
                </xsl:if>

                <xsl:if test="@exclusive='yes'">
                    <xsl:element name="span">
                        <xsl:attribute name="style">
                            <xsl:text>margin-right: 1ex; white-space: nowrap; </xsl:text>
                            <xsl:if test="not( @dead='yes' )">font-weight: bold; </xsl:if>
                        </xsl:attribute>
                        <xsl:text>exclusive </xsl:text>
                    </xsl:element>
                </xsl:if>

                <xsl:if test="@backup='yes'">
                    <span style="margin-right: 1ex;">backup </span>
                </xsl:if>

                <xsl:if test="not( @dead='yes' ) and not( @last_detected_heart_beat_age )">
                    <span style="margin-right: 1ex;">still checking... </span>
                </xsl:if>

                <xsl:if test="@dead='yes'">
                    <span style="margin-right: 1ex; font-weight: normal">
                        <xsl:text>dead</xsl:text>
                        <xsl:choose>
                            <xsl:when test="@last_detected_heart_beat">
                                <xsl:text>, </xsl:text>
                                <span style="white-space: nowrap; font-size: 8pt;">
                                    <xsl:text>discovered </xsl:text>
                                    <xsl:value-of select="@last_detected_heart_beat"/>
                                </span>
                            </xsl:when>
                            <xsl:when test="@database_last_heart_beat">
                                <xsl:text>, </xsl:text>
                                <span style="white-space: nowrap; font-size: 8pt">
                                    <xsl:text>after </xsl:text>
                                    <xsl:value-of select="@database_last_heart_beat"/>
                                </span>
                            </xsl:when>
                        </xsl:choose>
                        <xsl:text> </xsl:text>
                    </span>
                </xsl:if>

                <xsl:if test="@deactivating_cluster_member_id">
                    <span style="margin-right: 1ex;">
                        <xsl:text>Deactivated by </xsl:text>
                        <xsl:value-of select="@deactivating_cluster_member_id"/>
                        <xsl:text> </xsl:text>
                    </span>
                </xsl:if>
            </td>

            <td>
                <xsl:value-of select="@pid"/>
            </td>

            <td>
                <xsl:choose>
                    <xsl:when test="@last_detected_heart_beat_age">
                        <xsl:element name="span">
                            <xsl:attribute name="style">
                                <xsl:choose>
                                    <xsl:when test="@heart_beat_quality != 'good'">color: #e00000;</xsl:when>
                                </xsl:choose>
                            </xsl:attribute>

                            <xsl:value-of select="@last_detected_heart_beat_age"/>
                            <xsl:text>s ago </xsl:text>

                            <xsl:if test="@heart_beat_quality">
                                <span style="margin-left: 1ex;">
                                    <xsl:text>(</xsl:text>
                                    <xsl:value-of select="@heart_beat_quality"/>
                                    <xsl:text>)</xsl:text>
                                </span>
                            </xsl:if>
                        </xsl:element>
                    </xsl:when>
                    <xsl:when test="@dead != 'yes'  and  @database_last_heart_beat">
                        <span style="font-size: 8pt">
                            <xsl:value-of select="@database_last_heart_beat"/>
                        </span>
                    </xsl:when>
                </xsl:choose>
            </td>

            <td>
                <xsl:value-of select="@heart_beat_count"/>

                <xsl:if test="@late_heart_beat_count > 0">
                    <xsl:text> </xsl:text>
                    <span style="margin-left: 1ex; color: #e00000">
                        <xsl:text>(</xsl:text>
                        <xsl:value-of select="@late_heart_beat_count"/>
                        <xsl:text> late)</xsl:text>
                    </span>
                </xsl:if>
            </td>

            <td>
                <xsl:value-of select="@backup_precedence"/>
            </td>

            <td style="text-align: right">
                <!--xsl:if test="@cluster_member_id != ancestor::cluster/@cluster_member_id"-->
                <xsl:call-template name="command_menu">
                    <xsl:with-param name="onclick_call"       select="'cluster_member__onclick'"/>
                    <xsl:with-param name="onclick_param1_str" select="@cluster_member_id"/>
                    <xsl:with-param name="onclick_param2"     select="'mouse_x() - 70'"/>
                    <xsl:with-param name="onclick_param3"     select="'mouse_y() - 1'"/>
                </xsl:call-template>
                <!--/xsl:if-->
            </td>
        </xsl:element>


        <tr>
            <td colspan="99" style="border-top: 1px solid #fefe00; line-height: 1px;">
            </td>
        </tr>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~cluster_member-->

    <xsl:template mode="cluster_member_http_url" match="cluster_member">

        <xsl:element name="span">
            <xsl:attribute name="title">
                <xsl:text>ID=</xsl:text>
                <xsl:value-of select="@cluster_member_id"/>
                <xsl:text>, &#10;</xsl:text>
                <xsl:value-of select="@version"/>
            </xsl:attribute>

            <xsl:choose>
                <xsl:when test="@cluster_member_id=ancestor::cluster/@cluster_member_id">
                    <xsl:text>This Scheduler</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="@http_url"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:element>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~remote_schedulers-->

    <xsl:template match="remote_schedulers">
        <table cellpadding="0" cellspacing="0" width="100%" class="remote_scheduler">
            <col/>
            <col/>
            <col/>
            <col/>
            <col/>
            <col/>
            <col/>

            <thead>
                <xsl:call-template name="card_top"/>

                <tr>
                    <td colspan="99">
                        <xsl:value-of select="@count" /> Schedulers,
                        <xsl:value-of select="@connected" /> connected
                    </td>
                </tr>

                <tr>
                    <td class="head1">IP</td>
                    <td class="head">Hostname</td>
                    <td class="head">Port</td>
                    <td class="head">Id</td>
                    <td class="head">Connected</td>
                    <td class="head">Disconnected</td>
                    <td class="head">Version</td>
                </tr>

                <tr>
                    <td colspan="99" class="after_head_space">&#160;</td>
                </tr>
            </thead>

            <tbody>
                <xsl:for-each select="remote_scheduler">
                    <xsl:element name="tr">
                        <xsl:variable name="url" select="concat( 'http://', @hostname, ':', @tcp_port, /*/@my_url_path_base )" />

                        <xsl:attribute name="style">
                            cursor: pointer;
                            <xsl:if test="@connected='no'">color: gray;</xsl:if>
                        </xsl:attribute>
                        <xsl:attribute name="title">
                            <xsl:value-of select="$url"/>
                        </xsl:attribute>
                        <xsl:attribute name="onclick">
                            <xsl:text>window.open( "</xsl:text>
                            <xsl:value-of select="$url"/>
                            <xsl:text>", "</xsl:text>
                            <xsl:call-template name="translate_target">
                                <xsl:with-param name="target" select="$url"/>
                            </xsl:call-template>
                            <xsl:text>" )</xsl:text>
                        </xsl:attribute>
                        <xsl:attribute name="onmouseover">this.original_class_name = this.className;  this.className='remote_scheduler_hover'</xsl:attribute>
                        <xsl:attribute name="onmouseout" >this.className=this.original_class_name </xsl:attribute>

                        <td>
                            <xsl:value-of select="@ip"/>
                        </td>
                        <td>
                            <xsl:value-of select="@hostname"/>
                        </td>
                        <td>
                            <xsl:value-of select="@tcp_port"/>
                        </td>
                        <td>
                            <xsl:value-of select="@scheduler_id"/>
                        </td>
                        <td>
                            <xsl:value-of select="@connected_at__xslt_date_or_time_with_diff"/>
                        </td>
                        <td style="color: #e00000">
                            <xsl:if test="@connected='no'">
                                <xsl:value-of select="@disconnected_at__xslt_date_or_time_with_diff"/>
                            </xsl:if>
                        </td>
                        <td>
                            <xsl:value-of select="@version"/>
                        </td>
                    </xsl:element>
                </xsl:for-each>
            </tbody>

        </table>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~remote_schedulers-->

    <xsl:template match="calendar">

        <table cellspacing="0" cellpadding="0" class="calendar">

            <col/>
            <col/>
            <col/>
            <col/>

            <thead>
                <xsl:call-template name="card_top"/>

                <tr>
                    <th class="head1">Date</th>
                    <th class="head">Until</th>
                    <th class="head">Repetition</th>
                    <th class="head">Schedule</th>
                    <th class="head">Object</th>
                </tr>
                <tr>
                    <td colspan="2" class="after_head_space">&#160;</td>
                </tr>
            </thead>

            <tbody>
            
                <xsl:for-each select="at | period">
                    <xsl:sort select="concat( self::at/@at, self::period/@begin )"/>

                    <tr>
                        <td style="padding-right: 2ex">
                            <xsl:value-of select="@at__xslt_datetime_with_diff_plus"/>
                            <xsl:value-of select="@begin__xslt_datetime_with_diff_plus"/>
                        </td>

                        <td style="padding-right: 2ex">
                            <xsl:if test="self::period">
                                <xsl:choose>
                                    <xsl:when test="substring-before( @begin, 'T' ) != substring-before( @end, 'T' ) and substring-after( @end, 'T' ) = '00:00:00'">
                                        <xsl:text>24:00:00 </xsl:text>
                                        <xsl:value-of select="@end_xslt_datetime_diff_plus"/>
                                    </xsl:when>
                                    <xsl:otherwise>
                                        <xsl:value-of select="substring( @end__xslt_datetime_with_diff_plus, 12 )"/>
                                    </xsl:otherwise>
                                </xsl:choose>
                            </xsl:if>
                        </td>

                        <td style="padding-right: 2ex">
                            <xsl:if test="self::period">
                                <xsl:choose>
                                    <xsl:when test="@absolute_repeat">
                                        <xsl:text>every </xsl:text>
                                        <xsl:value-of select="@absolute_repeat"/>
                                        <xsl:text>s</xsl:text>
                                    </xsl:when>
                                    <xsl:when test="@repeat">
                                        <xsl:text>repeats after </xsl:text>
                                        <xsl:value-of select="@repeat"/>
                                        <xsl:text>s</xsl:text>
                                    </xsl:when>
                                </xsl:choose>
                            </xsl:if>
                        </td>

                        <td style="padding-right: 2ex">
                            <xsl:value-of select="@schedule"/>
                        </td>

                        <xsl:choose>
                            <xsl:when test="@job_chain">
                                <xsl:element name="td">
                                    <xsl:attribute name="class">order</xsl:attribute>
                                    <!--xsl:attribute name="style">cursor: default;</xsl:attribute>
                                    <xsl:attribute name="onmouseover">
                                        this.className = "job_chain_hover";
                                        this.style.cursor = "pointer";
                                    </xsl:attribute>
                                    <xsl:attribute name="onmouseout" >
                                        this.className = "order";
                                        this.style.cursor = "default"
                                    </xsl:attribute>
                                    <xsl:attribute name="onclick">
                                        call_error_checked( show_job_chain_details, '<xsl:value-of select="@job_chain"/>' )
                                    </xsl:attribute-->

                                    <span class="label">Order </span>
                                    <xsl:text> </xsl:text>
                                    <xsl:value-of select="@job_chain"/>:<xsl:value-of select="@order"/>
                                </xsl:element>
                            </xsl:when>
                            <xsl:when test="@task">
                                <td class="task">
                                    <span class="label">Task </span>
                                    <xsl:text> </xsl:text>
                                    <xsl:value-of select="@job"/>:<xsl:value-of select="@task"/>
                                </td>
                            </xsl:when>
                            <xsl:when test="@job">
                                <td class="job">
                                    <span class="label">Job </span>
                                    <xsl:text> </xsl:text>
                                    <xsl:value-of select="@job"/>
                                </td>
                            </xsl:when>
                        </xsl:choose>
                    </tr>
                    
                </xsl:for-each>
                
            </tbody>
        </table>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Detailsicht eines Jobs-->

    <xsl:template match="job">
        <table cellpadding="0" cellspacing="0" width="100%" class="job">
            <col align="left" width="1"/>
            <col align="left" width="160"  />
            <col align="left" width="1"/>
            <col align="left" width="*"  />

            <tr>
                <td colspan="99" class="job">
                    <table cellpadding="0" cellspacing="0" width="100%">
                        <tr>
                            <td style="padding-left: 0px">
                                <b>
                                    Job
                                    <xsl:value-of select="@path"/>
                                </b>
                                <xsl:if test="@title">
                                    <xsl:text> &#160; </xsl:text>
                                    <xsl:value-of select="@title"/>
                                </xsl:if>
                            </td>

                            <td style="padding-right: 0pt; text-align: right">
                                <xsl:call-template name="command_menu">
                                    <xsl:with-param name="onclick_call"       select="'job_menu__onclick'"/>
                                    <xsl:with-param name="onclick_param1_str" select="@path"/>
                                    <xsl:with-param name="onclick_param2"     select="'mouse_x() - 90'"/>
                                    <xsl:with-param name="onclick_param3"     select="'mouse_y() - 1'"/>
                                </xsl:call-template>
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>

            <tr>
                <td>&#160;</td>
            </tr>

            <tr>
                <td>
                    <span class="label">file timestamp:</span>
                </td>
                <td colspan="3">
                    <xsl:apply-templates mode="date_time" select="file_based/@last_write_time"/>
                </td>
            </tr>

            <tr>
                <td>
                    <span class="label">state:</span>
                </td>
                <td colspan="3">
                    <xsl:apply-templates select="@state"/>

                    <!--xsl:if test="@waiting_for_process='yes'">
                        <xsl:text>,</xsl:text>
                        <span class="waiting_for_process"> waiting for process!</span>
                    </xsl:if-->

                    <xsl:if test="@state_text and @state_text != '' ">
                        &#160;–&#160;
                        <xsl:call-template name="show_text_with_url">
                            <xsl:with-param name="text" select="@state_text"/>
                        </xsl:call-template>
                    </xsl:if>
                </td>
            </tr>

            <tr>
                <td>
                    <span class="label">requisites:</span>
                </td>
                <td colspan="3">
                    <xsl:for-each select="file_based/requisites/requisite">
                        <xsl:if test="position() > 1">, </xsl:if>
                        
                        <xsl:element name="span">
                            <xsl:if test="@is_missing='yes'">
                                <xsl:attribute name="class">requisite_missing</xsl:attribute>
                                <xsl:attribute name="title">is missing</xsl:attribute>
                            </xsl:if>
                        </xsl:element>

                        <xsl:value-of select="@type"/>
                        <xsl:text> </xsl:text>
                        <xsl:value-of select="@path"/>
                    </xsl:for-each>
                </td>
            </tr>

            <tr>
                <td>
                    <span class="label">error:</span>
                </td>
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


            <xsl:if test="file_based/ERROR">
                <tr>
                    <td>
                        <span class="label">error in configuration file:</span>
                    </td>
                    <td colspan="3">
                        <xsl:apply-templates mode="file_based_error" select="file_based"/>
                    </td>
                </tr>
            </xsl:if>


            <xsl:if test="file_based/removed">
                <tr>
                    <td>
                        <span class="label">removing is delayed:</span>
                    </td>
                    <td colspan="3">
                        <xsl:apply-templates mode="file_based_error" select="file_based/removed"/>
                    </td>
                </tr>
            </xsl:if>


            <xsl:if test="replacement/job/file_based">
                <tr>
                    <td>
                        <span class="label">
                            error in changed file:
                            <br/>
                            (not loaded)
                        </span>
                    </td>
                    <td colspan="3">
                        <xsl:apply-templates mode="file_based_error" select="replacement/job/file_based"/>
                    </td>
                </tr>
            </xsl:if>


            <tr>
                <td>
                    <span class="label">locks:</span>
                </td>
                <td colspan="3">
                    <xsl:for-each select="lock.requestor/lock.use">
                        <xsl:if test="position()>1">
                            <xsl:text>, </xsl:text>
                        </xsl:if>

                        <xsl:apply-templates select="." mode="short"/>
                        <!-- Für mode="long" fehlt /state/locks -->
                    </xsl:for-each>
                </td>
            </tr>


            <tr>
                <td>
                    <span class="label">next start:</span>
                </td>
                <td colspan="3">
                    <xsl:value-of select="@next_start_time__xslt_datetime_with_diff_plus"  disable-output-escaping="yes"/>
                </td>
            </tr>

            <tr>
                <td>
                    <span class="label">steps:</span>
                </td>
                <td>
                    <xsl:value-of select="@all_steps"/>
                </td>
                <td>
                    <span class="label">tasks:</span>
                </td>
                <td>
                    <xsl:value-of select="@all_tasks"/>
                </td>
            </tr>

            <xsl:if test="@order='yes'">
                <tr>
                    <td>
                        <span class="label">orders:</span>
                    </td>
                    <td>
                        <!--xsl:choose>
                            <xsl:when test="@order!='yes'">
                                (no order job)
                            </xsl:when-->
                        <xsl:if test="order_queue/@length!=''">
                            <xsl:value-of select="order_queue/@length"/>
                            <span class="small"> orders to process</span>
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

                <xsl:call-template name="card_selector_end"/>
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

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~job/@state-->

    <xsl:template match="job/@state">
        <xsl:choose>
            <xsl:when test=".='pending'">
                <xsl:value-of select="."/>
            </xsl:when>

            <xsl:when test=".='running'">
                <xsl:value-of select="."/>
            </xsl:when>

            <xsl:when test=".='not_initialized'">
                <span class="file_based_error">
                    <xsl:value-of select="."/>
                </span>
            </xsl:when>

            <xsl:otherwise>
                <span class="job_error">
                    <xsl:value-of select="."/>
                </span>
            </xsl:otherwise>
        </xsl:choose>

        <xsl:if test="parent::job/file_based/@state and parent::job/file_based/@state != .">
            <xsl:text> ›</xsl:text>
            <span class="has_title" title="generic state">
                <xsl:apply-templates select="parent::job/file_based/@state"/>
            </span>
            <xsl:text>‹</xsl:text>
        </xsl:if>

        <xsl:choose>
            <xsl:when test="parent::job/@delay_until">
                <xsl:text> (</xsl:text>
                <span class="task_error">delayed after error</span>
                <xsl:text>)</xsl:text>
            </xsl:when>
            <xsl:when test="parent::job/@in_period='no'">
                <xsl:text> (not in period)</xsl:text>
            </xsl:when>
        </xsl:choose>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Detailsicht einer Task-->

    <xsl:template match="task">
        <table cellpadding="0" cellspacing="0" class="task" width="100%" >
            <col align="left"  width="1"/>
            <col align="left"  />

            <tr>
                <td colspan="2" class="task">
                    <table cellpadding="0" cellspacing="0" width="100%">
                        <tr>
                            <td style="padding-left: 0px">
                                <b>Task&#160;</b>

                                <xsl:choose>
                                    <xsl:when test="not( @id )">
                                        <xsl:if test="../../@waiting_for_process='yes'">
                                            <span style="margin-left: 1ex; color: #e00000">needs process</span>
                                        </xsl:if>
                                    </xsl:when>
                                    <xsl:otherwise>
                                        <b>
                                            <xsl:value-of select="@id"/>
                                        </b>
                                    </xsl:otherwise>
                                </xsl:choose>

                                <xsl:if test="@name!=''">
                                    <xsl:text>&#160; </xsl:text>
                                    <xsl:value-of select="@name"/>
                                </xsl:if>
                                <xsl:if test="@pid">
                                    <xsl:text>, pid </xsl:text>
                                    <xsl:value-of select="@pid"/>
                                </xsl:if>
                                <xsl:if test="@cause!=''">
                                    <xsl:text> (start&#160;cause:&#160;</xsl:text>
                                    <xsl:value-of select="@cause"/>
                                    <xsl:text>)</xsl:text>
                                </xsl:if>
                                <xsl:if test="@state!=''">
                                    <xsl:text>, </xsl:text>
                                    <xsl:apply-templates select="@state" />
                                </xsl:if>
                                <xsl:if test="@calling">
                                    <xsl:text> </xsl:text>(<xsl:value-of select="@calling"/>)
                                </xsl:if>
                                <xsl:if test="@steps!=''">
                                    <xsl:text>, </xsl:text>
                                    <span style="white-space: nowrap">
                                        <xsl:value-of select="@steps"/> steps
                                    </span>
                                </xsl:if>
                            </td>

                            <xsl:if test="@id">
                                <td valign="top" style="padding-right: 0pt; text-align: right;">
                                    <xsl:call-template name="command_menu">
                                        <xsl:with-param name="onclick_call"       select="'task_menu__onclick'"/>
                                        <xsl:with-param name="onclick_param1_str" select="@job"/>
                                        <xsl:with-param name="onclick_param2_str" select="@id"/>
                                        <xsl:with-param name="onclick_param3"     select="'mouse_x() - 70'"/>
                                        <xsl:with-param name="onclick_param4"     select="'mouse_y() - 1'"/>
                                    </xsl:call-template>
                                </td>
                            </xsl:if>
                        </tr>
                    </table>
                </td>
            </tr>

            <tr>
                <td>
                    <span style="line-height: 6px">&#160;</span>
                </td>
            </tr>

            <xsl:if test="order or ../../@order='yes'">
                <tr>
                    <td>
                        <span class="label">order:</span>
                    </td>
                    <td class="order" colspan="99">
                        <xsl:choose>
                            <xsl:when test="order">
                                <xsl:element name="span">
                                    <xsl:attribute name="style">
                                        cursor: pointer;
                                        <xsl:if test="order/@removed='yes' or order/@replaced='yes'">
                                            text-decoration: line-through;
                                        </xsl:if>
                                    </xsl:attribute>

                                    <xsl:if test="order/@removed='yes'">
                                        <xsl:attribute name="title">Order is removed</xsl:attribute>
                                    </xsl:if>

                                    <xsl:if test="order/@replaced='yes'">
                                        <xsl:attribute name="title">Order is replaced</xsl:attribute>
                                    </xsl:if>

                                    <b>
                                        <xsl:value-of select="order/@id"/>
                                        &#160;
                                        <xsl:value-of select="order/@title"/>
                                    </b>
                                </xsl:element>
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
                        <td>
                            <span class="label">idle since:</span>
                        </td>
                        <td>
                            <xsl:value-of select="@idle_since__xslt_datetime_with_diff"  disable-output-escaping="yes"/>
                        </td>
                    </tr>
                </xsl:when>
                <xsl:otherwise>
                    <tr>
                        <td>
                            <span class="label">in process since:</span>
                        </td>
                        <td>
                            <xsl:value-of select="@in_process_since__xslt_datetime_with_diff"  disable-output-escaping="yes"/>
                        </td>
                    </tr>
                </xsl:otherwise>
            </xsl:choose>

            <tr>
                <td>
                    <span class="label">running since:</span>
                </td>
                <td>
                    <xsl:value-of select="@running_since__xslt_datetime_with_diff"  disable-output-escaping="yes"/>
                </td>
            </tr>

            <xsl:if test="@enqueued">
                <tr>
                    <td>
                        <span class="label">enqueued at:</span>
                    </td>
                    <td>
                        <xsl:value-of select="@enqueued__xslt_datetime_with_diff"  disable-output-escaping="yes"/>
                    </td>
                </tr>
            </xsl:if>

            <xsl:apply-templates mode="trs_from_log_attributes" select="log"/>

            <xsl:if test="subprocesses/subprocess">
                <xsl:for-each select="subprocesses/subprocess">
                    <tr class="process_class">
                        <td>
                            <span class="label">
                                pid <xsl:value-of select="@pid" />
                            </span>
                        </td>
                        <td colspan="99">
                            <span>
                                <xsl:value-of select="@title"/>
                            </span>
                        </td>
                    </tr>
                </xsl:for-each>
            </xsl:if>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~task/@state-->

    <xsl:template match="task/@state">
        <xsl:choose>
            <xsl:when test=". = 'suspended' or . = 'waiting_for_process' or . = 'deleting_files'">
                <span class="task_error">
                    <xsl:value-of select="."/>
                </span>
            </xsl:when>

            <xsl:otherwise>
                <xsl:value-of select="."/>
            </xsl:otherwise>
        </xsl:choose>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~queued_tasks-->

    <xsl:template match="queued_tasks" mode="list">

        <table valign="top" cellpadding="0" cellspacing="0" width="100%" class="task">

            <col align="left" width="40"/>
            <col align="left" width="70"/>
            <col align="left" width="$datetime_column_width"/>

            <thead>
                <xsl:call-template name="card_top"/>

                <tr>
                    <td colspan="99" class="task" align="left">
                        <p style="margin-top: 2px; margin-bottom: 1ex">
                            <b>
                                <xsl:value-of select="@length"/> enqueued tasks
                            </b>
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

                    <td>
                        <xsl:value-of select="@enqueued__xslt_date_or_time"        disable-output-escaping="yes"/>
                    </td>
                    <td>
                        <xsl:value-of select="@start_at__xslt_datetime_with_diff"  disable-output-escaping="yes"/>
                        <xsl:if test="@delayed_after_error_task">
                            <xsl:text>, </xsl:text>
                            delay_after_error of task <xsl:value-of select="@delayed_after_error_task"/>
                        </xsl:if>
                    </td>
                </tr>
            </xsl:for-each>
        </table>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~history-->

    <xsl:template match="history" mode="list">

        <table valign="top" cellpadding="0" cellspacing="0" width="100%" class="task">

            <col align="left" width="1"/>
            <col align="left" width="1"/>
            <col align="left" width="$datetime_column_width"/>
            <col align="right" width="1"/>
            <col align="left" width="$datetime_column_width"/>
            <col align="left"/>

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
                    <td>
                        <xsl:value-of select="@task"/>
                    </td>
                    <td>
                        <xsl:value-of select="@cause"/>
                    </td>
                    <td>
                        <xsl:value-of select="@start_time__xslt_datetime"   disable-output-escaping="yes"/>
                    </td>
                    <td>
                        <xsl:value-of select="@steps"/>
                    </td>
                    <td>
                        <xsl:value-of select="@end_time__xslt_datetime"     disable-output-escaping="yes"/>
                    </td>

                    <td valign="top" style="padding-right: 0pt; text-align: right">
                        <xsl:call-template name="command_menu">
                            <xsl:with-param name="onclick" select="concat( 'history_task_menu__onclick( ', @task, ', mouse_x() - 70, mouse_y() - 1 )' )"/>
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

    <xsl:template match="order_queue | order_history" mode="list">

        <table class="order" cellpadding="0" cellspacing="0" width="100%">

            <!--xsl:if test="order"-->
            <col width=" 30"/>
            <!--col valign="top"  width=" 15"  style="padding-right: 2ex"/-->
            <!--col width=" 55"/-->

            <xsl:if test="order/@title">
                <col width="*"/>
            </xsl:if>

            <xsl:if test="self::order_history">
                <col width="40"/>
            </xsl:if>

            <xsl:if test="order/@setback">
                <col width="40"/>
            </xsl:if>

            <col width=" 30"/>

            <xsl:if test="order/@state_text">
                <col width="*"/>
            </xsl:if>

            <col width="  1" align="right"/>

            <thead class="order">
                <xsl:call-template name="card_top"/>

                <xsl:if test="@length">
                    <tr>
                        <td colspan="99" align="left" class="order">
                            <p style="margin-top: 2px; margin-bottom: 1ex">
                                <b>
                                    <xsl:value-of select="@length"/> orders
                                </b>
                            </p>
                        </td>
                    </tr>
                </xsl:if>

                <tr>
                    <td class="head1">Id</td>
                    <!--td class="order">Pri</td-->
                    <!--td class="head">Created</td-->

                    <xsl:if test="order/@title">
                        <td class="head">Title</td>
                    </xsl:if>

                    <xsl:if test="self::order_history">
                        <td class="head">Finished at</td>
                    </xsl:if>

                    <xsl:if test="order/@setback">
                        <td class="head">Start</td>
                    </xsl:if>

                    <td class="head">State</td>

                    <xsl:if test="order/@state_text">
                        <td class="head">State text</td>
                    </xsl:if>

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

                        <td>
                            <xsl:value-of select="@id"/>
                        </td>
                        <!--td class="order"><xsl:value-of select="@priority"/></td-->
                        <!--td class="small"><xsl:value-of select="@created__xslt_date_or_time"        disable-output-escaping="yes"/></td-->

                        <xsl:if test="../order/@title">
                            <td>
                                <xsl:value-of select="@title"/>
                            </td>
                        </xsl:if>

                        <xsl:if test="parent::order_history">
                            <td>
                                <span style="white-space: nowrap">
                                    <xsl:value-of select="@end_time__xslt_date_or_time_with_diff"  disable-output-escaping="yes"/>
                                </span>
                            </td>
                        </xsl:if>

                        <xsl:if test="../order/@setback">
                            <td class="small">
                                <span style="white-space: nowrap">
                                    <xsl:value-of select="@setback__xslt_date_or_time_with_diff"  disable-output-escaping="yes"/>
                                </span>
                            </td>
                        </xsl:if>

                        <td>
                            <xsl:value-of select="@state"/>
                        </td>

                        <xsl:if test="../order/@state_text">
                            <td>
                                <xsl:call-template name="show_text_with_url">
                                    <xsl:with-param name="text" select="@state_text"/>
                                </xsl:call-template>
                            </td>
                        </xsl:if>

                        <td style="text-align: right">
                            <xsl:call-template name="command_menu">
                                <xsl:with-param name="onclick_call">
                                    <xsl:choose>
                                        <xsl:when test="parent::order_history">history_order_menu__onclick</xsl:when>
                                        <xsl:otherwise>order_menu__onclick</xsl:otherwise>
                                    </xsl:choose>
                                </xsl:with-param>
                                <xsl:with-param name="onclick_param1_str" select="@job_chain"/>
                                <xsl:with-param name="onclick_param2_str" select="@id"/>
                                <xsl:with-param name="onclick_param3_str" select="@history_id"/>
                                <xsl:with-param name="onclick_param4"     select="'mouse_x() - 70'"/>
                                <xsl:with-param name="onclick_param5"     select="'mouse_y() - 1'"/>
                            </xsl:call-template>
                        </td>
                    </xsl:element>
                </xsl:for-each>

                <xsl:if test="ERROR">
                    <tr>
                        <td class="job_error" colspan="99">
                            <xsl:apply-templates select="ERROR"/>
                        </td>
                    </tr>
                </xsl:if>
            </tbody>
            <!--/xsl:if-->
        </table>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ERROR-->

    <xsl:template match="ERROR">
        <xsl:if test="@time != ''">
            <xsl:value-of select="@time"/>
            <xsl:text> &#160; </xsl:text>
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

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~file_based_line-->
    
    <xsl:template mode="file_based_line" match="*">
        <xsl:choose>
            <xsl:when test="file_based/ERROR">
                <p style="margin: 0px">
                    <span class="label" style="white-space: nowrap">Error in configuration file: </span>
                    <span class="file_based_error_message">
                        <xsl:apply-templates select="file_based/ERROR"/>
                    </span>
                </p>
            </xsl:when>
            <xsl:when test="replacement/*/file_based/ERROR">
                <p style="margin: 0px">
                    <span class="label" style="white-space: nowrap">Error in changed file</span>
                    <span class="label" style="white-space: nowrap"> (not loaded): </span>
                    <span class="file_based_error_message">
                        <xsl:apply-templates select="replacement/*/file_based/ERROR"/>
                    </span>
                </p>
            </xsl:when>
            <xsl:when test="replacement">
                <p style="margin: 0px">
                    <span class="label">Replacement is standing by</span>
                </p>
            </xsl:when>
            <xsl:when test="file_based/removed/ERROR">
                <p style="margin: 0px">
                    <span class="label" style="white-space: nowrap">Removing delayed: </span>
                    <span class="file_based_error_message">
                        <xsl:apply-templates select="file_based/removed/ERROR"/>
                    </span>
                </p>
            </xsl:when>
        </xsl:choose>

        <xsl:if test="file_based/requisites/requisite [ @is_missing='yes' ]">
            <p class="small" style="margin: 0px; color: red">
                <xsl.text>Missing</xsl.text>
            
                <xsl:for-each select="file_based/requisites/requisite [ @is_missing='yes' ]">
                    <xsl:text> </xsl:text>
                    <xsl:value-of select="@type"/>
                    <xsl:text> '</xsl:text>
                    <xsl:value-of select="@path"/>
                    <xsl:text>'</xsl:text>
                </xsl:for-each>
            </p>
        </xsl:if>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~file_based/ERROR-->

    <xsl:template mode="file_based_error" match="file_based">
        <span class="file_based_error_message">
            <xsl:apply-templates select="ERROR"/>
        </span>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~file_based/@state-->

    <xsl:template match="file_based/@state">

        <span class="small">
            <xsl:choose>
                <xsl:when test=".='active'">
                    <span style="color: green">
                        <xsl:value-of select="."/>
                    </span>
                </xsl:when>

                <xsl:otherwise>
                    <span style="color: red">
                        <xsl:value-of select="."/>
                    </span>
                </xsl:otherwise>
            </xsl:choose>
        </span>

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
            <td>
                <span class="label">log:</span>
            </td>

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
                <td colspan="99" class="scheduler_error">
                    <xsl:value-of select="@last_error"/>
                </td>
            </tr>
        </xsl:if>

        <xsl:if test="@last_warning">
            <tr>
                <td>
                    <span class="label">last warning: </span>
                </td>
                <td colspan="99" class="scheduler_warning">
                    <xsl:value-of select="@last_warning"/>
                </td>
            </tr>
        </xsl:if>

        <xsl:if test="@last_info">
            <tr>
                <td>
                    <span class="label">last info: </span>
                </td>
                <td colspan="99">
                    <xsl:value-of select="@last_info"/>
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

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~time-->

    <xsl:template mode="time" match="*">
        <xsl:param name="show"/>
        <xsl:param name="modified_short_attribute"/>

        <xsl:element name="span">
            <xsl:attribute name="title">
                <xsl:value-of select="."/>
            </xsl:attribute>
            <xsl:value-of select="$modified_short_attribute"  disable-output-escaping="yes"/>
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~date_time-->

    <xsl:template mode="date_time" match="@*">
        <xsl:value-of select="concat( substring( ., 1, 10 ), ' ', substring( ., 12 ) )"/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_url-->
    <!-- Hängt /z/ an die Scheduler-Url, wenn die eigene diesen Suffix hat -->

    <xsl:template name="scheduler_url">
        <xsl:param name="url"/>

        <xsl:choose>
            <xsl:when test="substring( /spooler/@my_url_base, string-length( /spooler/@my_url_base ) - 2 ) = '/z/'">
                <xsl:value-of select="concat( $url, '/z/' )"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$url"/>
            </xsl:otherwise>
        </xsl:choose>

    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~show_text_with_url-->
    <!-- Wickelt http://… in <a href="http://…">http://…</a> ein -->

    <xsl:template name="show_text_with_url">

        <xsl:param name="text"/>

        <xsl:choose>
            <xsl:when test="contains( $text, 'http://' )">
                <xsl:value-of select="substring-before( $text, 'http://' )"/>
                
                <xsl:variable name="tail" select="substring-after( $text, 'http://' )"/>
                
                <xsl:choose>
                    <xsl:when test="contains( $tail, ' ' )">
                        <xsl:variable name="url" select="concat( 'http://', substring-before( $tail, ' ' ) )"/>

                        <xsl:call-template name="show_text_with_url.url_with_punctuation">
                            <xsl:with-param name="url" select="$url"/>
                        </xsl:call-template>

                        <xsl:text> </xsl:text>

                        <xsl:call-template name="show_text_with_url">
                            <xsl:with-param name="text" select="substring-after( $tail, ' ' )"/>
                        </xsl:call-template>
                    </xsl:when>

                    <xsl:otherwise>
                        <xsl:call-template name="show_text_with_url.url_with_punctuation">
                            <xsl:with-param name="url" select="concat( 'http://', $tail )"/>
                        </xsl:call-template>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:when>

            <xsl:otherwise>
                <xsl:value-of select="$text"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~show_text_with_url.url_with_punctuation-->
 
    <xsl:template name="show_text_with_url.url_with_punctuation">
        <xsl:param name="url"/>
        
        <xsl:choose>
            <xsl:when test="not( contains( 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/', substring( $url, string-length( $url ), 1 ) ) )">
                <xsl:call-template name="show_text_with_url.url">
                    <xsl:with-param name="url" select="substring( $url, 1, string-length( $url ) - 1 )"/>
                </xsl:call-template>
                <xsl:value-of select="substring( $url, string-length( $url ), 1 )"/>
            </xsl:when>
            
            <xsl:otherwise>
                <xsl:call-template name="show_text_with_url.url">
                    <xsl:with-param name="url" select="$url"/>
                </xsl:call-template>
            </xsl:otherwise>
        </xsl:choose>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~show_text_with_url.url-->
    
    <xsl:template name="show_text_with_url.url">
        <xsl:param name="url"/>

        <xsl:element name="a">
            <xsl:attribute name="target">_blank</xsl:attribute>
            <xsl:attribute name="href">
                <xsl:value-of select="$url"/>
            </xsl:attribute>

            <xsl:value-of select="$url"/>
        </xsl:element>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~translate_target-->
    <!-- Für <a target="..."/> -->

    <xsl:template name="translate_target">
        <xsl:param name="target"/>
        <xsl:value-of select="translate( $target, ':/.-', '____' )" />
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_menu-->

    <xsl:template name="command_menu">
        <xsl:param name="value">
            <u>Menu</u>
        </xsl:param>
        <xsl:param name="onclick"/>
        <xsl:param name="onclick_call"/>
        <xsl:param name="onclick_param1"/>
        <xsl:param name="onclick_param1_str"/>
        <xsl:param name="onclick_param2"/>
        <xsl:param name="onclick_param2_str"/>
        <xsl:param name="onclick_param3"/>
        <xsl:param name="onclick_param3_str"/>
        <xsl:param name="onclick_param4"/>
        <xsl:param name="onclick_param4_str"/>
        <xsl:param name="onclick_param5"/>
        <xsl:param name="onclick_param5_str"/>

        <xsl:element name="span">
            <xsl:attribute name="class">menu_click</xsl:attribute>

            <xsl:attribute name="onclick">
                <xsl:choose>
                    <xsl:when test="$onclick">
                        <xsl:value-of select="$onclick"/>
                    </xsl:when>

                    <xsl:otherwise>
                        <xsl:value-of select="$onclick_call"/>
                        <xsl:text>( </xsl:text>

                        <xsl:call-template name="command_menu_param">
                            <xsl:with-param name="first"     select="true()"/>
                            <xsl:with-param name="param"     select="$onclick_param1"/>
                            <xsl:with-param name="param_str" select="$onclick_param1_str"/>
                        </xsl:call-template>

                        <xsl:call-template name="command_menu_param">
                            <xsl:with-param name="param"     select="$onclick_param2"/>
                            <xsl:with-param name="param_str" select="$onclick_param2_str"/>
                        </xsl:call-template>

                        <xsl:call-template name="command_menu_param">
                            <xsl:with-param name="param"     select="$onclick_param3"/>
                            <xsl:with-param name="param_str" select="$onclick_param3_str"/>
                        </xsl:call-template>

                        <xsl:call-template name="command_menu_param">
                            <xsl:with-param name="param"     select="$onclick_param4"/>
                            <xsl:with-param name="param_str" select="$onclick_param4_str"/>
                        </xsl:call-template>

                        <xsl:call-template name="command_menu_param">
                            <xsl:with-param name="param"     select="$onclick_param5"/>
                            <xsl:with-param name="param_str" select="$onclick_param5_str"/>
                        </xsl:call-template>

                        <xsl:text> )</xsl:text>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:attribute>

            <xsl:copy-of select="$value"/>

        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_menu_param-->

    <xsl:template name="command_menu_param">
        <xsl:param name="param"/>
        <xsl:param name="param_str"/>
        <xsl:param name="first" select="false()"/>

        <xsl:if test="not( $first ) and ( $param or $param_str )">
            <xsl:text>,</xsl:text>
        </xsl:if>

        <xsl:choose>
            <xsl:when test="$param">
                <xsl:value-of select="$param"/>
            </xsl:when>
            <xsl:when test="$param_str">
                <xsl:text>"</xsl:text>
                <!-- Anführungszeichen in $param_str werden nicht ersetzt -->
                <xsl:call-template name="replace">
                    <xsl:with-param name="string" select="$param_str"/>
                    <xsl:with-param name="old"    select="'\'"/>
                    <xsl:with-param name="new"    select="'\\'"/>
                </xsl:call-template>
                <xsl:text>"</xsl:text>
            </xsl:when>
        </xsl:choose>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~replace-->

    <xsl:template name="replace">
        <xsl:param name="string"/>
        <xsl:param name="old"/>
        <xsl:param name="new"/>

        <xsl:choose>
            <xsl:when test="contains( $string, $old )">
                <xsl:value-of select="concat( substring-before( $string, $old ), $new )"/>

                <xsl:call-template name="replace">
                    <xsl:with-param name="string" select="substring-after( $string, $old)"/>
                    <xsl:with-param name="old" select="$old"/>
                    <xsl:with-param name="new" select="$new"/>
                </xsl:call-template>
            </xsl:when>

            <xsl:otherwise>
                <xsl:value-of select="$string"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

</xsl:stylesheet>
