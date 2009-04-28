<?xml version='1.0'?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:variable name="selected_programming_language" select="'perl'"/>
<xsl:variable name="language_has_properties"       select="false()"/>

<xsl:include href="api.xsl" />

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<!-- Für Perl wird auch die öffnende Klammer der Parameterliste erzeugt (wegen LetProperty) -->

<!--xsl:template match="property [ /*/@programming_language='perl' ] | method [ /*/@programming_language='perl' ]" mode="method_name"-->
<xsl:template match="property | method" mode="method_name">
    <xsl:param name="access"/>

    <xsl:if test="parent::api.class/@object_name">
        <span class="api_object_name">
            $<xsl:value-of select="parent::api.class/@object_name"/>
            <xsl:text>-></xsl:text>
        </span>
    </xsl:if>

    <span class="mono">
        <xsl:choose>
            <xsl:when test="$access='write'">
                <xsl:text>LetProperty(</xsl:text>
                '<span style="font-weight: bold">
                    <xsl:value-of select="@name"/>
                </span>',
            </xsl:when>

            <xsl:when test="$access='read'">
                <span style="font-weight: bold">
                    <xsl:value-of select="@name"/>
                </span>
                <xsl:if test="com/com.parameter">(</xsl:if>
            </xsl:when>

            <xsl:otherwise>
                <span style="font-weight: bold">
                    <xsl:value-of select="@name"/>
                </span>
                <xsl:text>(</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
    </span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @type='bool' ]" mode="no_array">
    <span class="mono">Boolean</span>
</xsl:template>

<xsl:template match="com.type [ @type='DATE' ]" mode="no_array">
    <span class="mono">Datestring</span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.empty" mode="description">
    <code>undefined</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.null" mode="description">
    <code>undefined</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="method [ com/com.parameter//com.type [ @class or @array ] ] | property [ com/com.parameter//com.type [ @class or @array ] ]" mode="comment">
    <br/>
    <span class="not_for_unix_perl">
        –
        <xsl:choose>
            <xsl:when test="com/com.parameter[ not( @optional ) ]/com.type [ @class or @array ]">
                <span class="not_for_unix_perl">
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'api.method.not_for_unix_perl'"/>
                    </xsl:call-template>
                </span>
            </xsl:when>
            <xsl:otherwise>
                <span class="not_for_unix_perl">
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'api.method.restricted_for_unix_perl'"/>
                    </xsl:call-template>
                </span>
            </xsl:otherwise>
        </xsl:choose>
    </span>
</xsl:template>
-->    
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="method [ com/com.parameter//com.type [ @class or @array ] ] | property [ com/com.parameter//com.type [ @class or @array ] ]" mode="detailed_comment">
    <p class="not_for_unix_perl">
        <xsl:choose>
            <xsl:when test="com/com.parameter[ not( @optional ) ]/com.type [ @class or @array ]">
                <span class="not_for_unix_perl">
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'api.method.not_for_unix_perl.detailed'"/>
                    </xsl:call-template>
                </span>
            </xsl:when>
            <xsl:otherwise>
                <span class="not_for_unix_perl">
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'api.method.restricted_for_unix_perl.detailed'"/>
                    </xsl:call-template>
                </span>
            </xsl:otherwise>
        </xsl:choose>
    </p>
</xsl:template>
-->    
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->


</xsl:stylesheet>
