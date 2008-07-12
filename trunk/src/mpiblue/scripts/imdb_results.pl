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
# 2008-03-20: Movie results section was no longer being located; fixed parsing of movies with a /I after the prod. year.
# 2000-03-26: Fixed regular expressions

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

print "Movida IMDb results parser started.\r\n";

$inputFile = $ARGV[0];
($outputDir) = ($inputFile =~ m/^(.+[\\|\/]).+?$/);
$outputFile = $outputDir ? $outputDir."mvdmres.xml" : "mvdmres.xml";

-r  $inputFile or die "Input file does not exist or cannot be accessed.";

print "Parsing movie file: ", $inputFile, "\r\n";
print "Writing matches to: ", $outputFile, "\r\n";

# configuration parameters
#%config = (
#);

# constants
%MovieTypeDescriptions = (
	'tv' => 'Made for video',
	'v' => 'Made for video or direct-to-video release',
	'vg' => 'Video game',
	'mini' => 'Television miniseries',
);

open F_IN, "<", $inputFile or die "Failed to open input file.";
open F_OUT, ">", $outputFile or die "Failed to open output file.";

# print header
print F_OUT "<?xml version='1.0' encoding='UTF-8'?>\n";
print F_OUT "<movida-movie-results version=\"1\">\n";

# regular expressions

# currently seven digits
$rxImdbId = '\d{7}';

# the next line compares in the header of pages:
# NO RESULTS: <link rel="stylesheet" type="text/css" href="none_files/consumersite.css">
# MULTIPLE RESULTS: <link rel="stylesheet" type="text/css" href="multi_files/consumersite.css">
# MULTIPLE RESULTS (alternative): <link rel="stylesheet" type="text/css" href="http://i.imdb.com/images/css2/consumersite.css">
$rxResultsPageType = '.*<link.*href="([^<>]*)/consumersite\.css.*';
$rxResultsPageTypeNoResults = 'none_files';

# the next line is used to detect REDIRECTS after a single match
# <meta name="title" content="IMDb Title  Search">
# <meta name="title" content="The Matrix (1999)"><meta ....
$rxPageType = '<meta name="title" content="([^"]*)">';
$rxPageTypeNoRedirect = '\s*IMDb\s*Title\s*Search\s*';

# this is a good pattern to speed up parsing. there is no useful data before this point.
$rxDataStart = '^\s*</script>\s*$';

# results are grouped. possible groups:
# <p><b>Popular Titles</b> (Displaying 4 Results)<table> ..... </table>
# <p><b>Titles (Exact Matches)</b> (Displaying 1 Result)<table> ..... </table>
# <p><b>Titles (Partial Matches)</b> (Displaying 22 Results)<table> ..... </table>
# <p><b>Titles (Approx Matches)</b> (Displaying 17 Results)<table> ..... </table>
#
# $1 = group name, $s = group size, $3 = group contents
$rxGroup = '<p><b>(?:Titles \()?(.*?)\)?</b>\s*\(Displaying (\d*) Results?\)<table>(.*?)</table>';

# $1 = id, $2 = title, $3 = year, $4 = optional aka
$rxMatch = '<tr>.*?<a href="[^"]*/title/tt('.$rxImdbId.')/">\s*([^<>]*?)\s*</a>\s*\((\d{4})(:?.*?)?\)(.*?)?</td>\s*</tr>';

# The Matrix (1999)
$rxTitle = '\s*(.*?)\s*\((\d{4})(:?.*?)?\)';

# (TV) = made for TV (TV-movies, MOWs, pilots, TV specials)
# (V) = made for video or direct-to-video release
# (VG) = video game
# (mini) = television miniseries 
$rxMovieType = '\s*\(([a-z]*)\)';
$rxAka = '<br>.*?aka\s*(.*?)\s*<br>';

# set to true if we detect a redirect to a movie page
$isMoviePage = 0;

while (<F_IN>) {
	next if /^\s*$/; # skip blank lines
	chomp;
	
	# look for the page type first
	if (!$hasPageType) {
		if (/$rxPageType/i) {
			unless ($1 =~ /$rxPageTypeNoRedirect/i) {
				$isMoviePage = 1;
				($movieTitle, $movieYear) = ($1 =~ /$rxTitle/i);
				$movieTitle = XmlEscape($movieTitle);
				print "Your search produced only one match and you have been redirected to the movie page for '$movieTitle ($movieYear)'.";
			}
			$hasPageType = 1;
		}
		next;
	}
	
	# now look for the results page type
	if (!$isMoviePage and !$hasResultsPageType) {
		if (/$rxResultsPageType/i) {
			if ($1 =~ /$rxResultsPageTypeNoResults/i) {
				print "Sorry, but the query resulted in no matches.";
				last;
			}
			$hasResultsPageType = 1;
		}
		next;
	}

	if ($isMoviePage) {
		# movie data page
		print F_OUT "\t\t<result>\n";
		print F_OUT "\t\t\t<source type=\"cached\"/>\n";
		print F_OUT "\t\t\t<title>$movieTitle</title>\n";
		print F_OUT "\t\t\t<year>$movieYear</year>\n";
		print F_OUT "\t\t</result>\n";
		last;

	} else {
		# results page
		if (/$rxDataStart/i) {
			$_ = <F_IN>;
			$hasMovieData = 1;
		}
		
		if ($hasMovieData) {
			
			while (m/$rxGroup/ig) {
				$name = XmlEscape($1);
				print F_OUT "\t<group name=\"$name\">\n";
				$group = $3;
				while ($group =~ m/$rxMatch/ig) {
					# $1 = id, $2 = title, $3 = year, $4 = optional raw aka
					if ($4) {
						$akaRaw = $4;
						$aka = '';
						if ($akaRaw =~ /$rxMovieType/i) {
							if ($movieType = $MovieTypeDescriptions{lc($1)}) {
								$aka .= $movieType;
							}
						}
						
						$firstAka = 1;
						while ($akaRaw =~ m/$rxAka/ig) {
							if ($firstAka) {
								unless ($aka eq '') {
									$aka .= '<br><br>';
								}
								$aka .= "Alternative titles:";
								$firstAka = 0;
							}
							$aka .= "<br>".$1;
						}
						
						$aka = XmlEscape($aka);
					}
					
					$id = $1;
					$title = XmlEscape($2);
					$year = $3;
					print F_OUT "\t\t<result>\n";
					print F_OUT "\t\t\t<source type=\"remote\"/>\n";
					print F_OUT "\t\t\t<title>$title</title>\n";
					print F_OUT "\t\t\t<year>$year</year>\n";
					if ($aka) {
						print F_OUT "\t\t\t<notes>$aka</notes>\n";
					}
					print F_OUT "\t\t\t<url>http://us.imdb.com/title/tt$id/</url>\n";
					print F_OUT "\t\t</result>\n";
				}
				print F_OUT "\t</group>\n";
			}
			next;
		}
	}
}

# print footer
print F_OUT "</movida-movie-results>";

close F_IN;
close F_OUT;
