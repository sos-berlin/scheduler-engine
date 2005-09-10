<?xml version='1.0'?>
<!-- $Id: scheduler.xsl 3860 2005-09-06 08:38:41Z jz $ -->



<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">
                
<xsl:include href="../scheduler.xsl" />

<xsl:output doctype-public="-//W3C//DTD HTML 4.01//EN" />  <!--"http://www.w3.org/TR/html4/strict.dtd"-->
<!--xsl:output doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"/> <!- - "http://www.w3.org/TR/html4/loose.dtd"-->
    
    

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~api-->

<xsl:template match="api">
    
    <xsl:param name="title"/>
    <xsl:param name="base_dir"/>
    <xsl:param name="author"/>
    <xsl:param name="date"/>

    <html>
        <xsl:call-template name="html_head">
            <xsl:with-param name="title" select="$title"/>
        </xsl:call-template>

        <body>
            <xsl:apply-templates select="description"/>
        </body>
    </html>

</xsl:template>


<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api_classes" mode="description">
    <xsl:apply-templates mode="script" select="document( 'all_classes.xml' )/class_references/class_reference"/>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="class_reference">
    <xsl:element name="a">
        <xsl:attribute name="href"><xsl:value-of select="@name"/>.xml</xsl:attribute>
        <xsl:value-of select="@name"/>
    </xsl:element>
    <br/>
</xsl:template>


<xsl:template match="class_reference" mode="script">
    <xsl:element name="a">
        <xsl:attribute name="href">javascript:void(0)</xsl:attribute>  <!-- Für ie6 -->
        <xsl:attribute name="onclick"><xsl:value-of select="concat( 'api.class_reference__onclick(&quot;', @name, '&quot;)' )"/></xsl:attribute>
        <xsl:value-of select="@name"/>
    </xsl:element>
    <br/>
</xsl:template>






<xsl:template match="/class [ /*/@show_list ]">
    <xsl:apply-templates mode="list" select="."/>
</xsl:template>    




<xsl:template mode="list" match="class">
    <p style="margin-top: 0px; padding-bottom: 3ex; font-weight: bold;">
        <xsl:value-of select="@name"/>
        <xsl:if test="@title">
             - <xsl:value-of select="@title"/>
        </xsl:if>
    </p>

    <table cellpadding="0" cellspacing="0">
        <!--tr><td colspan="4" style="padding-top: 4ex; padding-bottom: 1ex; font-weight: bold;">Eigenschaften</td></tr-->
        <tr>
            <td>Typ</td>
            <td style="padding-left: 1ex">Eigenschaft&#160;</td>
            <td style="padding-left: 1ex">Parameter</td>
            <td style="padding-left: 1ex"></td>
            <td style="padding-left: 1ex"></td>
        </tr>
        <tr>
            <td colspan="5"><hr size="1"/></td>
        </tr>
        
        <xsl:apply-templates mode="list_properties_and_methods" select="property"/>

        
        <tr><td colspan="4" style="padding-top: 6ex; padding-bottom: 1ex; font-weight: bold;"> </td></tr>
        <tr>
            <td>Ergebnistyp</td>
            <td style="padding-left: 1ex">Methode&#160;</td>
            <td style="padding-left: 0ex">Parameter</td>
            <td style="padding-left: 1ex"></td>
            <td style="padding-left: 1ex"></td>
        </tr>
        <tr>
            <td colspan="5"><hr size="1"/></td>
        </tr>
        
        <xsl:apply-templates mode="list_properties_and_methods" select="method"/>
    </table>

</xsl:template>


<xsl:template mode="list_properties_and_methods" match="property | method">
    <xsl:for-each select="com">
        <tr>
            <xsl:apply-templates mode="list" select=".">
                <xsl:with-param name="td" select="'x'"/>
            </xsl:apply-templates>
        </tr>
    </xsl:for-each>
</xsl:template>


<xsl:template mode="list" match="property | method">
    <p>
        <xsl:apply-templates mode="list" select="com"/>
    </p>
</xsl:template>



<xsl:template mode="list" match="com">
    <xsl:param name="td"/>

    <td class="mono">
        <xsl:if test="com.type">
            <xsl:value-of select="com.type/@type"/>
        </xsl:if>
    </td>

    <td style="padding-left: 1ex">
        <xsl:value-of select="parent::*/parent::class/@object_name"/>.<span class="mono" style="font-weight: bold"><xsl:value-of select="../@name"/></span>
    </td>
    
    <xsl:element name="td">
        <xsl:if test="not( parent::property )">
            <xsl:attribute name="colspan">2</xsl:attribute>
        </xsl:if>
        
        <xsl:if test="com.parameter or parent::method">
            <span class="mono"><xsl:text>(</xsl:text></span>

            <xsl:if test="com.parameter">
                <span class="mono"><xsl:text> </xsl:text></span>
                <xsl:for-each select="com.parameter">
                    <xsl:if test="position() &gt; 1">
                        <span class="mono"><xsl:text>, </xsl:text></span>
                    </xsl:if>
                    <xsl:apply-templates select="."/>
                </xsl:for-each>
                <span class="mono"><xsl:text> </xsl:text></span>
            </xsl:if>

            <span class="mono">)</span>
        </xsl:if>
    </xsl:element>
    
    <xsl:if test="parent::property">
        <td style="padding-left: 1ex">
            <xsl:if test="@access">
                <xsl:value-of select="@access"/> only
            </xsl:if>
        </td>
    </xsl:if>
    
    <td style="padding-left: 1ex">
        <xsl:value-of select="parent::property/title"/>
    </td>
    
</xsl:template>


<xsl:template match="com.parameter">
    <span class="mono"><xsl:value-of select="com.type/@type"/></span>
    <span class="mono"><xsl:text> </xsl:text></span>
    <span class="mono"><xsl:value-of select="@name"/></span>
</xsl:template>




<!--

<xsl:template match="class">

    <xsl:variable name="title" select="@title"/>
    
    <html>
        <xsl:call-template name="html_head">
            <xsl:with-param name="title" select="$title"/>
        </xsl:call-template>

        
        <body>
            <h1><xsl:value-of select="@name"/></h1>

            <xsl:apply-templates select="property | method"/>
        </body>
    </html>

</xsl:template>


<xsl:template match="property | method">
    <h2>Eigenschaft <xsl:value-of select="@name"/></h2>
    
    <p>
        <xsl:apply-templates select="com"/>
        <xsl:apply-templates select="java"/>
    </p>
</xsl:template>


<xsl:template match="com">
    
    <xsl:if test="com.type">
        <xsl:value-of select="com.type/@type"/>
    </xsl:if>
    
    <xsl:text> </xsl:text>
    <xsl:value-of select="/*/@object_name"/>.<xsl:value-of select="../@name"/>
    
    <xsl:if test="com.parameter">
        <xsl:text>( </xsl:text>

        <xsl:for-each select="com.parameter">
            <xsl:if test="position() &gt; 1">, </xsl:if>
            <xsl:apply-templates select="."/>
        </xsl:for-each>

        )
    </xsl:if>
    
</xsl:template>


<xsl:template match="com.parameter">
    <xsl:value-of select="com.type/@type"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@name"/>
</xsl:template>

-->

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_commands-->
    
</xsl:stylesheet>

<!-- Das ist ein Gedankenstrich: – -->
<!-- Das ist drei Punkte: … -->
