<?xml version='1.0'?>
<!-- $Id: scheduler.xsl 3860 2005-09-06 08:38:41Z jz $ -->



<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property [ /*/@programming_language='java' ] | method [ /*/@programming_language='java' ]" mode="list_property_or_method">

    <xsl:choose>
        <xsl:when test="java">
            <xsl:apply-templates select="java [ position() = 1 ]" mode="table_row">
                <xsl:with-param name="access" select="@access | parent::*/@access"/>
                <xsl:with-param name="rowspan" select="count(java)"/>
            </xsl:apply-templates>
            <xsl:apply-templates select="java [ position() &gt; 1 ]" mode="table_row">
                <xsl:with-param name="access" select="@access | parent::*/@access"/>
                <xsl:with-param name="rowspan" select="0"/>
            </xsl:apply-templates>
        </xsl:when>
        
        <xsl:when test="com">
            <xsl:for-each select="com">
                <xsl:choose>
                    <xsl:when test="parent::property and @access='write'">
                        <xsl:apply-templates select="." mode="table_row">
                            <xsl:with-param name="access" select="'write'"/>
                        </xsl:apply-templates>
                    </xsl:when>
                        
                    <xsl:when test="parent::property and @access='read'">
                        <xsl:apply-templates select="." mode="table_row">
                            <xsl:with-param name="access" select="'read'"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    
                    <xsl:when test="parent::property and not( @access )">
                        <xsl:apply-templates select="." mode="table_row">
                            <xsl:with-param name="access" select="'write'"/>
                            <xsl:with-param name="rowspan" select="2"/>
                        </xsl:apply-templates>
                    
                        <xsl:apply-templates select="." mode="table_row">
                            <xsl:with-param name="access" select="'read'"/>
                            <xsl:with-param name="rowspan" select="0"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    
                    <xsl:otherwise>
                        <xsl:apply-templates select="." mode="table_row"/>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:for-each>
        </xsl:when>
        
        <xsl:otherwise>
            <!-- Methode ohne Parameter, ohne Ergebnis -->
            <xsl:apply-templates select="." mode="table_row"/>
        </xsl:otherwise>
    </xsl:choose>
    
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="method [ /*/@programming_language='java' ] | property [ /*/@programming_language='java' ] " mode="method_name">
    <xsl:param name="access"/>

    <!--    
    <span style="font-size: 8pt"><xsl:value-of select="parent::*/parent::class/@object_name"/></span>
    <xsl:text>.</xsl:text>
    -->
    
    <span class="mono" style="font-weight: bold">
        <xsl:if test="$access='write'">set_</xsl:if>
        <xsl:if test="$access='read'">&#160;&#160;&#160;&#160;</xsl:if>
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
