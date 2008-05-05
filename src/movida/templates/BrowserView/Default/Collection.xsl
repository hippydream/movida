<?xml version="1.0" encoding="UTF-8"?>

<!DOCTYPE xsl:stylesheet [
<!ENTITY nbsp '&#160;'>
]>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/TR/xhtml1/strict">
            
	<xsl:template match="collection-info">
		<html>
			<head>
				<title>movida movie collection<xsl:if test="string(name)"> - <xsl:value-of select="name"/></xsl:if></title>
				<style>
body {
	background-color: #EBEBEB;
	margin: 6px;
	padding: 6px;
	border: 1px solid #848484;
	font-size: 65%;
	font-family: 'Trebuchet MS', Verdana, Georgia, Arial, sans-serif;
}

a, a:hover, a:visited {
	color: #FFAA00;
}

h1, h2, h3, h4, h5, h6 {
	margin: 0 0 10px 0;
	padding: 0;
	font-style: normal;
	font-weight: bold;
	font-variant: normal;
	color: #222;
}

h1 {
	font-size: 1.7em;
	margin-bottom: 0px;
}

h2 {
	font-size: 1.2em;
	margin-bottom: 20px;
	padding-bottom: 5px;
	border-bottom: 2px solid #222;
}

h3 {
	font-size: 1.4em;
	margin-bottom: 10px;
}

table {
	margin-bottom: 20px;
}

table th, table td {
	text-align: left;
	vertical-align: top;
	font-size: 0.9em;
}

table th {
	padding-right: 10px;
	white-space: nowrap;
}

table.details th {
	background-color: #D2D2D2;
	border-bottom: 1px solid #222;
}

table.details .actor,
table.details .role {
	width: 20em;
}

table.details tr.odd td {
	background-color: #dedede;
}

p {
	font-size: 1.3em;
}

				</style>
			</head>
			<body>
				<h1>
					<xsl:choose>
						<xsl:when test="@name"><xsl:value-of select="@name"/></xsl:when>
						<xsl:otherwise>Untitled movie collection</xsl:otherwise>
					</xsl:choose>
				</h1>
				<h2>
					<xsl:choose>
						<xsl:when test="@movies = 0"><span><!-- avoid tag collapsing --></span></xsl:when>
						<xsl:when test="@movies = 1">The collection contains only one movie.</xsl:when>
						<xsl:otherwise>The collection contains <xsl:value-of select="@movies"/> movies.</xsl:otherwise>
					</xsl:choose>
				</h2>
				
				<h3>About this collection</h3>
				<xsl:choose>
					<xsl:when test="count(descendant::*) != 0">
						<table cellspacing="0" border="0" class="details">
							<xsl:if test="string(owner)">
								<tr class="odd"><th>Owner:</th><td><xsl:value-of select="owner"/></td></tr>
							</xsl:if>
							<xsl:if test="string(email)">
								<tr class="even"><th>Email:</th><td><xsl:value-of select="email"/></td></tr>
							</xsl:if>
							<xsl:if test="string(website)">
								<tr class="odd"><th>Website:</th><td><xsl:value-of select="website"/></td></tr>
							</xsl:if>
							<xsl:if test="string(notes)">
								<tr class="even"><th>Notes:</th><td><xsl:value-of select="notes"/></td></tr>
							</xsl:if>
						</table>
					</xsl:when>
					<xsl:otherwise>
						<p>There is no information associated to this collection. You can set the collection owner and other properties
						 using the &quot;About this collection&quot; item in the &quot;Collection&quot; menu.
						</p>
					</xsl:otherwise>
				</xsl:choose>
				
				<h3>Statistics</h3>
				<xsl:choose>
					<xsl:when test="@movies = 0">
						<p>The collection is currently empty. You can either <a href="movida://collection/add">add movies manually</a>
						or import from other sources like the Internet.</p>
					</xsl:when>
					<xsl:when test="@movies = 1">
						<p>The collection contains only one movie. You can either <a href="movida://collection/add">add movies
						manually</a> or import from other sources like the Internet.</p>
					</xsl:when>
					<xsl:otherwise>
						<p>The collection contains <xsl:value-of select="@movies" /> movies. You can either 
						<a href="movida://collection/add">add movies manually</a> or import from other sources like the Internet.</p>
					</xsl:otherwise>
				</xsl:choose>
				
				</body>
		</html>
	</xsl:template>
</xsl:stylesheet>
