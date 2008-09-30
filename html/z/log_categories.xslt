<?xml version='1.0' encoding="utf-8"?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl   = "http://www.w3.org/1999/XSL/Transform"
                xmlns:msxsl = "urn:schemas-microsoft-com:xslt"
                version     = "1.0">

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~log_categories-->
    
    <xsl:template match="log_categories">

        <h2>Log categories</h2>

        <p style="margin-bottom: 1em">
            Reset after
            <input type="text" size="4" id="delay_input"></input>
            <label class="standard_size" for="delay_input">seconds</label>
        </p>
        
        <table cellspacing="0" cellpadding="0">
            <thead>
                
            </thead>

            <tbody>
                <xsl:for-each select="log_category">
                    <xsl:sort select="@path"/>
                    
                    <tr>
                        <td>
                            <xsl:element name="input">
                                <xsl:attribute name="type">checkbox</xsl:attribute>
                                <xsl:attribute name="id">
                                    <xsl:text>checkbox_log_category__</xsl:text>
                                    <xsl:value-of select="@path"/>
                                </xsl:attribute>
                                <xsl:if test="@value='1'">
                                    <xsl:attribute name="checked">checked</xsl:attribute>
                                </xsl:if>
                                <xsl:attribute name="onclick">execute_input()</xsl:attribute>
                            </xsl:element>
                            
                            <xsl:element name="label">
                                <xsl:attribute name="for">
                                    <xsl:text>checkbox_log_category__</xsl:text>
                                    <xsl:value-of select="@path"/>
                                </xsl:attribute>
                                <xsl:attribute name="class">standard_size</xsl:attribute>
                                <xsl:attribute name="style">
                                    <xsl:if test="@value='1'">
                                        <xsl:text>font-weight: bold; </xsl:text>
                                    </xsl:if>
                                </xsl:attribute>
                                <xsl:value-of select="@path"/>
                            </xsl:element>
                        </td>
                        <td>
                            <xsl:if test="@all_children='yes'">
                                <xsl:text> all children</xsl:text>
                            </xsl:if>
                        </td>
                        <td>
                            <xsl:if test="@is_implicit='yes'">
                                <xsl:text> implicit</xsl:text>
                            </xsl:if>
                        </td>
                        <td>
                            <xsl:if test="@is_explicit='yes'">
                                <xsl:text> explicit</xsl:text>
                            </xsl:if>
                        </td>
                        <td>
                            <xsl:value-of select="@title"/>
                        </td>
                    </tr>
                </xsl:for-each>
            </tbody>
        </table>

        <!--button type="submit" onclick="submit_button_onclick()">Apply</button-->

        
    </xsl:template>
    
</xsl:stylesheet>
