# movida blue plugin script
#
# Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
# 
# This file is part of the Movida project (http://movida.sourceforge.net/).
# For requests or issue reports please refer to http://movida.sourceforge.net/contact
#
# This file may be distributed and/or modified under the terms of the
# GNU General Public License version 2 as published by the Free Software
# Foundation and appearing in the file LICENSE.GPL included in the
# packaging of this file.
#
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# See the file LICENSE.GPL that came with this software distribution or
# visit http://www.gnu.org/copyleft/gpl.html for GPL licensing information.

# CHANGES:
# 2007-04-27: Strip HTML from plot
# 2007-11-07: Titles with special chars would not be parsed (i.e. ".45" by Gary Lennon)
# 2007-11-07: Entries with one single director/writer were not being recognized
# 2008-03-20: Fixed parsing of cast members, movie poster and IMDB id
# 2000-03-26: Fixed regular expressions

# TODO trivia, notes, keywords?
# TODO localized title

# subroutines
%XmlEntities = (
	'>' => '&gt;',
	'<' => '&lt;',
	'&' => '&amp;',
	'"' => '&quot;',
	"'" => '&apos;'
);
$RxXmlEntities = join('', keys %XmlEntities);
sub XmlEscape {
	if ($_[0]) {
		$s = $_[0];
		$s =~ s/([$RxXmlEntities])/$XmlEntities{$1}/og;
		return $s;
	}
}


print "Movida IMDb movie parser started.\r\n";

$inputFile = $ARGV[0];
($outputDir) = ($inputFile =~ m/^(.+[\\|\/]).+?$/);
$outputFile = $outputDir ? $outputDir."mvdmdata.xml" : "mvdmdata.xml";

-r  $inputFile or die "Input file does not exist or cannot be accessed.";

print "Parsing movie file: ", $inputFile, "\r\n";
print "Writing matches to: ", $outputFile, "\r\n";

# configuration parameters
#%config = (
#	'import-rating' => 1,
#	'import-languages' => 0,
#	'import-poster' => 1,
#	'import-plot' => 1,
#);

# constants
$imdbMaxRating = 10;

open F_IN, "<", $inputFile or die "Failed to open input file.";
open F_OUT, ">", $outputFile or die "Failed to open output file.";

# print header
print F_OUT "<?xml version='1.0' encoding='UTF-8'?>\n";
print F_OUT "<movida-movie-data version=\"1\">\n";
print F_OUT "\t<movie>\n";


# regular expressions

# currently seven digits
$rxImdbId = '\d{7}';
# <title>The Matrix (1999)</title>
$rxTitle = '<title>([^<>]*) \((\d{4})\)</title>';
# <div class="photo">
$rxPoster = '<div class="photo">.*';
# <a name="poster" href="photogallery" title="The Matrix"><img border="0" alt="The Matrix" title="The Matrix" src="http://ia.imdb.com/media/imdb/01/I/38/48/31m.jpg" height="140" width="99"></a>
$rxPosterEx = '<a name="poster".*?src="(.*?)".*>';
$rxPosterSizeFind = '_S[XY]\d+_S[XY]\d+_'; # X and Y can be inverted
$rxPosterSizeReplace = '_SX600_SY600_';
# <a href="/rg/title-lhs/mymovies/mymovies/list?pending&amp;add=0133093">
$rxId = '<a href=".*?/mymovies.*?('.$rxImdbId.').*>';
# <b>User Rating:</b>
$rxRating = '<b>User Rating:</b>';
# <b>8.6/10</b>
$rxRatingEx = '<b>([0-9.]{1,3})/10</b>';
# <h3>Overview</h3>
$rxDataStart = '<h3>Overview</h3>';
# <h5>Directors:</h5>
$rxDirectors = '<h5>Directors?:</h5>';
# <a href="/name/nm0905152/">Andy Wachowski</a><br/><a href="/name/nm0905154/">Larry Wachowski</a><br/>
# <a href="/name/nm0001466/" onclick="(new Image()).src='/rg/directorlist/position-1/images/b.gif?link=name/nm0001466/';">Sergio Leone</a><br/>
$rxDirectorsEx = '<a href="/name/nm('.$rxImdbId.')/"[^>]*>(.*?)</a>';
# <h5>Writers <a href="/wga">(WGA)</a>:</h5>
$rxWriters = '<h5>Writers?.*</h5>';
# same as for directors
$rxWritersEx = $rxDirectorsEx;
# <h5>Genre:</h5>
$rxGenres = '<h5>Genre:</h5>';
# <a href="/Sections/Genres/Action/">Action</a> / <a href="/Sections/Genres/Thriller/">Thriller</a> ...
$rxGenresEx = '<a href="/Sections/Genres/.*?/"[^>]*>(.*?)</a>';
# <h5>Plot Outline:</h5> 
$rxPlot = '<h5>Plot[^<]*:</h5>';
# PLOT PLOT PLOT <a href=...>
$rxPlotEx = '\s*(.*?)\s*<.*';
# 
$rxCast = '^\s*<div class="headerinline"><h3>Cast</h3>';
# a single cast member has the following table snipped (but all on the same line!):
# <td class="hs">
# 	<a href="/rg/title-tease/tinyhead/name/nm0000206/">
# 		<img src="http://ia.imdb.com/media/imdb/01/I/65/85/38t.jpg" width="23" height="32" border="0">
# 	</a> [THIS ONE IS MISSING SOMETIMES!]
# 	<br>
# </td>
# <td class="nm">
# 	<a href="/name/nm0000206/">Keanu Reeves</a>
# </td>
# <td class="ddd"> ... </td>
# <td class="char">
# 	<a href="/character/ch0000741/">Neo</a> [OR simply "Neo"!!!]
# 	(as Some Other Name) [this is optional!]
# </td>
#
# $1 = image, $2 = id, $3 = name, $4 = role
# <tr class="odd">***
# <td class="nm"><a href="/name/nm0000020/"***>Henry Fonda</a></td><td class="ddd"> ... </td>
# <td class="char"><a href="/character/ch0006031/">Frank</a></td></tr>
$rxCastEx  = '<tr[^>]*>.*?';
$rxCastEx .= '<img src="([^"]*)".*?';
$rxCastEx .= '<td class="nm"[^>]*>.*?/name/nm('.$rxImdbId.')/[^>]*>([^<]*)</a></td>.*?'; # id + name
$rxCastEx .= '<td class="char"[^>]*>(.*?)</td>'; # role
$rxCastEx .= '</tr>';
$rxCastInvalidImage = 'addtiny.gif$';
# <h5>Runtime:</h5>
$rxRunningTime = '<h5>Runtime:</h5>';
# 136 min
$rxRunningTimeEx = '(\d*) min';
# <h5>Country:</h5>
$rxCountries = '<h5>Country:</h5>';
$rxCountriesStop = '</div>';
# <a href="/Sections/Countries/Italy/">\nItaly</a>\n | \n<a href="/Sections/Countries/USA/">\nUSA</a>
$rxCountriesEx = '<a href="/Sections/Countries/[^>]*>\s*(.*?)\s*</a>';
# <h5>Language:</h5>
$rxLanguages = '<h5>Language:</h5>';
$rxLanguagesStop = $rxCountriesStop;
# <a href="/Sections/Languages/English/">English</a> / <a href="/Sections/Languages/Italian/">Italian</a>
$rxLanguagesEx = '<a href="/Sections/Languages/[^>]*>\s*(.*?)\s*</a>';
# <h5>Color:</h5>
$rxColor = '<h5>Color:</h5>';
$rxColorStop = $rxCountriesStop;
# <a href="/List?color-info=Color&&heading=13;Color">Color</a> <i>(Technicolor)</i>
# <a href="/List?color-info=Color&&heading=13;Color">Color</a> <i>(Technicolor)</i>
$rxColorEx = '<a href=[^>]*>\s*([^>]*)\s*</a>';

# this is a good pattern to stop parsing. there is no useful info beyond this point.
$rxStop = '<div class="comment">';


# GO!
while (<F_IN>) {
	next if /^(\s)*$/; # skip blank lines
	chomp;
	
	if (!$hasTitle and /$rxTitle/i) {
		print F_OUT "\t\t<original-title>$1</original-title>\n";
		print F_OUT "\t\t<year>$2</year>\n";
		$hasTitle = 1;
		
	} elsif (!$hasPosters and /$rxPoster/i) {
		$_ = <F_IN>;
		if (/$rxPosterEx/i) {
			$myPoster = $1;
			$myPoster =~ s/$rxPosterSizeFind/$rxPosterSizeReplace/; # Larger poster URL
			print F_OUT "\t\t<poster>$myPoster</poster>\n";
		}
		$hasPosters = 1;
		
	} elsif (!$hasId and /$rxId/i) {
		print F_OUT "\t\t<imdb-id>$1</imdb-id>\n";
		print F_OUT "\t\t<urls><url description=\"IMDb\">http://www.imdb.com/title/tt$1/</url></urls>\n";
		$hasId = 1;
		
	} elsif (!$hasRating and /$rxRating/i) {
		$_ = <F_IN>;
		if (/$rxRatingEx/i) {
			print F_OUT "\t\t<rating maximum=\"$imdbMaxRating\">$1</rating>\n";
		}
		$hasRating = 1;
		
	} elsif (!$hasDataStart and /$rxDataStart/i) {
		# look for this convenience pattern :-)
		$hasDataStart = 1;
	}
	
	next unless $hasDataStart;
	
	if (!$hasDirectors and /$rxDirectors/i) {
		$_ = <F_IN>;
		while (/^(\s)*$/) { $_ = <F_IN>; } # skip blank lines
		print F_OUT "\t\t<directors>\n";
		while (m/$rxDirectorsEx/ig) {
			$id = $1;
			$name = XmlEscape($2);
			print F_OUT "\t\t\t<person>\n\t\t\t\t<name>$name</name>\n\t\t\t\t<imdb-id>$id</imdb-id>\n\t\t\t</person>\n";
		}
		print F_OUT "\t\t</directors>\n";
		$hasDirectors = 1;
		
	} elsif (!$hasWriters and /$rxWriters/i) {
		$_ = <F_IN>;
		while (/^(\s)*$/) { $_ = <F_IN>; } # skip blank lines
		print F_OUT "\t\t<crew>\n";
		while (m/$rxWritersEx/ig) {
			$id = $1;
			$name = XmlEscape($2);
			print F_OUT "\t\t\t<person>\n\t\t\t\t<name>$name</name>\n\t\t\t\t<imdb-id>$id</imdb-id>\n\t\t\t\t<roles><role>writer</role></roles>\n\t\t\t</person>\n";
		}
		print F_OUT "\t\t</crew>\n";
		$hasWriters = 1;
		
	} elsif (!$hasGenres and /$rxGenres/i) {
		$_ = <F_IN>;
		while (/^(\s)*$/) { $_ = <F_IN>; } # skip blank lines
		print F_OUT "\t\t<genres>\n";
		while (m/$rxGenresEx/ig) {
			$name = XmlEscape($1);
			print F_OUT "\t\t\t<genre>$name</genre>\n";
		}
		print F_OUT "\t\t</genres>\n";
		$hasGenres = 1;
		
	} elsif (!$hasPlot and /$rxPlot/i) {
		$_ = <F_IN>;
		while (/^(\s)*$/) { $_ = <F_IN>; } # skip blank lines
		if (/$rxPlotEx/i) {
			($plot = $1) =~ s/<.*?>//g;
			print F_OUT "\t\t<plot><![CDATA[$plot]]></plot>\n";
		}
		$hasPlot = 1;
		
	} elsif (!$hasCast and /$rxCast/i) {
		print F_OUT "\t\t<cast>\n";
		while (m/$rxCastEx/ig) {
			$img = $1;
			$id = $2;
			$name = XmlEscape($3);
			# TODO split roles using "/" as delimiter and avoid writing roles if there is none!
			$role = $4;
			$role =~ s/\<[^\<]+\>//g; # Strip HTML tags from role
			$role = XmlEscape($role);
			if ($5) {
				$aka = ' aka="'.$5.'"';
			}
			# Do not output placeholder image
			if ($img =~ /$rxCastInvalidImage/) { $img = ''; }
			else { $img =~ s/$rxPosterSizeFind/$rxPosterSizeReplace/;  } # Larger image URL
			print F_OUT "\t\t\t<person>\n\t\t\t\t<name>$name</name>\n\t\t\t\t<imdb-id>$id</imdb-id>\n\t\t\t\t<image>$img</image>\n\t\t\t\t<roles><role$aka>$role</role></roles>\n\t\t\t</person>\n";
		}
		print F_OUT "\t\t</cast>\n";
		$hasCast = 1;
		
	} elsif (!$hasRunningTime and /$rxRunningTime/i) {
		$_ = <F_IN>;
		while (/^(\s)*$/) { $_ = <F_IN>; } # skip blank lines
		if (/$rxRunningTimeEx/i) {
			print F_OUT "\t\t<running-time>$1</running-time>\n";
		}
		$hasRunningTime = 1;
		
	}  elsif (!$hasCountries and /$rxCountries/i) {
		$_ = <F_IN>;
		while (/^(\s)*$/) { $_ = <F_IN>; } # skip blank lines
		print F_OUT "\t\t<countries>\n";
		$line = $_;
		while (!(m/$rxCountriesStop/i)) { $_ = <F_IN>; $line .= $_; } # Join lines
		while ($line =~ m/$rxCountriesEx/ig) {
			$name = XmlEscape($1);
			print F_OUT "\t\t\t<country>$name</country>\n";
		}
		print F_OUT "\t\t</countries>\n";
		$hasCountries = 1;
		
	} elsif (!$hasLanguages and /$rxLanguages/i) {
		$_ = <F_IN>;
		while (/^(\s)*$/) { $_ = <F_IN>; } # skip blank lines
		print F_OUT "\t\t<languages>\n";
		$line = $_;
		while (!(m/$rxLanguagesStop/i)) { $_ = <F_IN>; $line .= $_; } # Join lines
		while ($line =~ m/$rxLanguagesEx/ig) {
			$name = XmlEscape($1);
			print F_OUT "\t\t\t<language>$name</language>\n";
		}
		print F_OUT "\t\t</languages>\n";
		$hasLanguages = 1;
		
	} elsif (!$hasColor and /$rxColor/i) {
		$_ = <F_IN>;
		while (/^(\s)*$/) { $_ = <F_IN>; } # skip blank lines
		$line = $_;
		while (!(m/$rxColorStop/i)) { $_ = <F_IN>; $line .= $_; } # Join lines
		if ($line =~ /$rxColorEx/i) {
			if ($1 =~ /color/i) {
				$color = "color";
			} elsif ($1 =~ /black and white/i) {
				$color = "bw";
			}
			if ($color) {
				print F_OUT "\t\t<color-mode>$color</color-mode>\n";
			}
		}
		$hasColor = 1;
		
	} elsif (/$rxStop/i) {
		last;
	}
}

# print footer
print F_OUT "\t</movie>\n";
print F_OUT "</movida-movie-data>\n";

close F_IN;
close F_OUT;
