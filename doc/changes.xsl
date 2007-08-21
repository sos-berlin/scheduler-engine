<?xml version='1.0'?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

    <xsl:include href="scheduler.xsl" />
    <!--xsl:output doctype-public="-//W3C//DTD HTML 4.01//EN" />  <!- -"http://www.w3.org/TR/html4/strict.dtd"-->


    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~changes-->

    <xsl:template match="changes">

        <xsl:variable name="title" select="@title"/>

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>


            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title"       select="$title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>

                <xsl:apply-templates select="change">
                    <xsl:sort select="@subversion_revision" data-type="number" order="descending"/>
                </xsl:apply-templates>

                <xsl:call-template name="bottom">
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
            </body>
        </html>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~change-->

    <xsl:template match="change">

        <table width="100%" style="border-top: 1px solid #f0f0f0; margin-top: 0ex">
            <tbody>
                <tr>
                    <td valign="top" style="width: 50px">
                        <span style="font-weight: normal; white-space: nowrap">
                            <xsl:value-of select="@version"/>.<xsl:value-of select="@subversion_revision"/>&#160;
                        </span>
                    </td>
                    <td valign="top" style="padding-left: 1ex; width: 80px; white-space: nowrap">
                        <span style="font-weight: normal">
                            <xsl:value-of select="@date"/>
                        </span>
                    </td>
                    <td valign="top" align="left" style="padding-left: 2ex">
                        <xsl:apply-templates select="revision"/>
                    </td>
                </tr>
            </tbody>
        </table>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~revision-->

    <xsl:template match="revision">

        <xsl:if test="position() > 1">
            <p style="margin-top: 0em">&#160;</p>
        </xsl:if>

        <xsl:element name="p">
            <xsl:attribute name="style">
                margin-top: 0px;
                <xsl:choose>
                    <xsl:when test="@type='correction'">
                        <xsl:text>color: green;</xsl:text>
                    </xsl:when>
                    <xsl:when test="@type='error'">
                        <xsl:text>color: #d00000;</xsl:text>
                    </xsl:when>
                </xsl:choose>
            </xsl:attribute>

            <b>
                <xsl:if test="@jira">
                    <xsl:element name="a">
                        <xsl:attribute name="href">
                            <xsl:text>http://www.sos-berlin.com/jira/browse/</xsl:text>
                            <xsl:value-of select="@jira"/>
                        </xsl:attribute>
                        <xsl:attribute name="title">Jira issue</xsl:attribute>
                        
                        <xsl:value-of select="@jira"/>
                    </xsl:element>
                    <xsl:text>: </xsl:text>
                </xsl:if>
                <xsl:value-of select="@title"/>
            </b>
        </xsl:element>

        <xsl:if test="description">
            <p style="margin-top: 0ex">&#160;</p>
            <xsl:apply-templates select="description"/>
            <p style="margin-top: 0ex">&#160;</p>
        </xsl:if>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

</xsl:stylesheet>
