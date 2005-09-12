<?xml version='1.0'?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property [ java and /*/@programming_language='java' ] | method [ java and /*/@programming_language='java' ]" mode="table_rows">
    <xsl:param name="show_title"  select="true()"/>
    <xsl:param name="is_in_table" select="false()"/>

    <xsl:apply-templates select="java" mode="table_row">
        <xsl:with-param name="show_title"  select="$show_title"/>
        <xsl:with-param name="is_in_table" select="$is_in_table"/>
    </xsl:apply-templates>
    
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property [ /*/@programming_language='java' ] | method [ /*/@programming_language='java' ]" mode="method_name">
    <xsl:param name="access"/>

    <!--    
    <span style="font-size: 8pt"><xsl:value-of select="parent::*/parent::class/@object_name"/></span>
    <xsl:text>.</xsl:text>
    -->
    
    <span class="mono" style="font-weight: bold">
        <xsl:if test="$access='write'">set_</xsl:if>
        <!--xsl:if test="$access='read'">&#160;&#160;&#160;&#160;</xsl:if>-->
        <xsl:value-of select="@name"/>
    </span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ /*/@programming_language='java' and @type='bool' ]">
    <span class="mono">boolean</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='java' and @type='BSTR' ]">
    <span class="mono">java.lang.String</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='java' and @type='VARIANT*' ]">
    <span class="mono">java.lang.String</span>
</xsl:template>

<xsl:template match="com.type [ @class and /*/@programming_language='java' ]">
    <span class="mono"><xsl:value-of select="concat( 'sos.spooler.', @class )"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->


<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
                
    
</xsl:stylesheet>
