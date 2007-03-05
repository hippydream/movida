sub EscapeChars {
  my $text = @_[0];

  $text =~ s/&\s/&amp;\s/g;
  $text =~ s/</&lt;/g;
  $text =~ s/%20/ /g;
  $text =~ s/%(\w{2})/&\#x$1;/g;

  return $text;
};

($dir) = ( $0 =~ /(.*\\)Scripts\\.*?\.pl/);

open SearchDetailsfile, "<", $dir."searchdetails.html";
open ParsedDetailsfile, ">", $dir."parseddetails.xml";

#read the whole HTML file into variable $line
undef $/;
$line = <SearchDetailsfile>;

$bt = '<title>';
$at = ' (?:\(\d{4}(/.*?)?\))?(?: \(.*?\))?</title>';

$btitlemore = '<a href="/TitleBrowse\?';
$atitlemore = '%20\(\d{4}\)">title index</a>';

$bimdb = '/title/tt';
$aimdb = '/">';

$bimdbrating = '<b>';
$aimdbrating = '/10</b>';

$byear = '<title>.*? \(';
$ayear = '(/.*?)?\)(?: \(.*?\))?</title>';

$brt = 'Runtime:</b>\n.*?';
$art = ' min';

$bcountry = '<a href="/Sections/Countries/.*?/">';
$acountry = '</a>';

$blang = '<a href="/Sections/Languages/.*?/">';
$alang = '</a>';

$bdirectors = '<a name="directors"';
$adirectors = '</table>';

$bdirector = '<a href="/name/nm\d*/">';
$adirector = '</a>';

$bwriters = '<a name="writers"';
$awriters = '</table>';

$bwriter = '<a href="/name/nm\d*/">';
$awriter = '</a>';

$bproducers = '<a name="producers"';
$aproducers = '</table>';

$bproducer = '<a href="/name/nm\d*/">';
$aproducer = '</a></td><td valign="top" nowrap="1"> \.\.\.\. </td><td valign="top">(?:.*?)producer">';

$bgenres = 'Genre:</b>';
$agenres = '<br>\n';

$bgenre = '<a href="/Sections/Genres/';
$agenre = '/">';

$bcast1 = '<b class="blackcatheader">Cast</b>';
$bcast2 = '<b class="blackcatheader">Series Cast Summary\:</b>';
$acast = '</table>';

$bactor = '<a href="/name/nm\d*/">';
$aactor = '</a>';

$brole = '<td valign="middle">';
$arole = '</td></tr>';
# $arole = '(\(.*?\))?(\(.*?\))?</td></tr>';

$bim = '<img border="0" alt=".*?" title=".*?" src="';
$aim = '" ';

$bplot1 = 'Plot Outline:</b>';
$bplot2 = 'Plot Summary:</b>';
$aplot1 = '<a href="/rg/title-tease/plotsummary';
$aplot2 = '<br><br>\n\n';

$brating = '<a href="/List\?certificates=USA:';
$arating = '&&';

#producer
#musician

print ParsedDetailsfile "<?xml version='1.0' encoding='UTF-8'?>\n";
print ParsedDetailsfile "<movieinfo>\n";
print ParsedDetailsfile "<movielist>\n";
print ParsedDetailsfile "<movie>\n";

($title) = ($line =~ /.*?$bt(.*?)$at/s);
$title = EscapeChars($title);
($titlemore) = ($line =~ /$btitlemore(.*?)$atitlemore/s);
$titlemore = EscapeChars($titlemore);
($titlesort, $titlethe) = ($titlemore =~ /(.*?), (\w{1,4})$/);
if ($titlesort ne '') {
  print ParsedDetailsfile "<titlesort>$titlesort</titlesort>\n"; 
  print ParsedDetailsfile "<titlethe>$titlethe</titlethe>\n";
  $title = $titlethe.' '.$titlesort;
};
print ParsedDetailsfile "<title>$title</title>\n";
 
($imdbnr) = ($line =~ /.*?$bimdb(\d+?)$aimdb/s);
$imdbnr = EscapeChars($imdbnr);
print ParsedDetailsfile "<imdbnum>$imdbnr</imdbnum>\n";
print ParsedDetailsfile "<queryid>$imdbnr</queryid>\n";

($imdbrating) = ($line =~ /.*?$bimdbrating(\d\.\d)$aimdbrating/s);
$imdbrating = EscapeChars($imdbrating);
print ParsedDetailsfile "<imdbrating>$imdbrating</imdbrating>\n";

($year) = ($line =~ /.*?$byear(\d{4})$ayear/s);
$year = EscapeChars($year);
print ParsedDetailsfile "<releasedate><year>$year</year></releasedate>\n";

($rt) = ($line =~ /.*?$brt(\d+)$art/s);
$rt = EscapeChars($rt);
print ParsedDetailsfile "<runtime>$rt</runtime>\n";

($country) = ($line =~ /.*?$bcountry(.*?)$acountry/s);
$country = EscapeChars($country);
print ParsedDetailsfile "<country><displayname>$country</displayname></country>\n";

($lang) = ($line =~ /.*?$blang(.*?)$alang/s);
$lang = EscapeChars($lang);
print ParsedDetailsfile "<language><displayname>$lang</displayname></language>\n";

($im) = ($line =~ /.*?$bim(.*?)$aim/s);
$im = EscapeChars($im);
$im =~ s/MZZZ/LZZZ/;
# $im =~ s/\.gif/\.jpg/;
print ParsedDetailsfile "<coverfront>$im</coverfront>\n";

($plot) = ($line =~ /.*?(?:$bplot1|$bplot2)(.*?)(?:$aplot1|$aplot2)/s);
$plot = EscapeChars($plot);
print ParsedDetailsfile "<plot>$plot</plot>\n";

print ParsedDetailsfile "<crew>\n";

($directors) = ($line =~ /.*?$bdirectors(.*?)$adirectors/s);
while  ($directors =~ /\G.*?$bdirector(.*?)$adirector/gs) {
  $director = EscapeChars($1);
  ($fn, $ln) = ($director =~ /(.*?) (.*)/);
  if ($ln eq '') { $ln = $director; };
  $ln =~ s/\(.*\)//;
  print ParsedDetailsfile "<crewmember><person><displayname>$fn $ln</displayname> <lastname>$ln</lastname><firstname>$fn</firstname></person><role id='dfDirector'>Director</role></crewmember>\n";
};

($writers) = ($line =~ /.*?$bwriters(.*?)$awriters/s);
while  ($writers =~ /\G.*?$bwriter(.*?)$awriter/gs) {
  $writer = EscapeChars($1);
  ($fn, $ln) = ($writer =~ /(.*?) (.*)/);
  if ($ln eq '') { $ln = $writer; };
  $ln =~ s/\(.*\)//;
  print ParsedDetailsfile "<crewmember><person><displayname>$fn $ln</displayname> <lastname>$ln</lastname><firstname>$fn</firstname></person><role id='dfWriter'>Writer</role></crewmember>\n";
};

($producers) = ($line =~ /.*?$bproducers(.*?)$aproducers/s);
while  ($producers =~ /\G.*?$bproducer(.*?)$aproducer/gs) {
  $producer = EscapeChars($1);
  ($fn, $ln) = ($producer =~ /(.*?) (.*)/);
  if ($ln eq '') { $ln = $producer; };
  $ln =~ s/\(.*\)//;
  print ParsedDetailsfile "<crewmember><person><displayname>$fn $ln</displayname> <lastname>$ln</lastname><firstname>$fn</firstname></person><role id='dfProducer'>Producer</role></crewmember>\n";
};
print ParsedDetailsfile "</crew>\n";

print ParsedDetailsfile "<genres>\n";

($genres) = ($line =~ /.*?$bgenres(.*?)$agenres/s);
while  ($genres =~ /\G.*?$bgenre(.*?)$agenre/gs) {
  $genre = EscapeChars($1);
  print ParsedDetailsfile "<genre><displayname>$genre</displayname></genre>\n";
};

print ParsedDetailsfile "</genres>\n";

print ParsedDetailsfile "<cast>\n";

($cast) = ($line =~ /.*?(?:$bcast1|$bcast2)(.*?)$acast/s);
while  ($cast =~ /\G.*?$bactor(.*?)$aactor.*?$brole(.*?)$arole/gs) {
  $actor = EscapeChars($1);
  $role = $2;
  $role =~ s/<.*?>/ /g;
  $role =~ s/\(.*\)//g;
  $role = EscapeChars($role);
  ($fn, $ln) = ($actor =~ /(.*?(?: \w\.)?) (.*)/);
  if ($ln eq '') { $ln = $actor; };
  $ln =~ s/\(.*\)//;
  print ParsedDetailsfile "<star><person><displayname>$fn $ln</displayname> <lastname>$ln</lastname><firstname>$fn</firstname></person><role id='dfActor'>Actor</role><character>$role</character></star>\n";
};

print ParsedDetailsfile "</cast>\n";

($rating) = ($line =~ /.*?$brating(.*?)$arating/s);
$rating = EscapeChars($rating);
print ParsedDetailsfile "<mpaarating><displayname>$rating</displayname></mpaarating>\n";

if ($imdbnr ne '') {
  print ParsedDetailsfile "<links><link>\n";
  print ParsedDetailsfile "<description>IMDB</description>\n";
  print ParsedDetailsfile "<url>http://www.imdb.com/title/tt".$imdbnr."</url>\n";
  print ParsedDetailsfile "</link></links>\n";
};

print ParsedDetailsfile "</movie>\n";
print ParsedDetailsfile "</movielist>\n";
print ParsedDetailsfile "</movieinfo>\n";

close SearchDetailsfile;
close ParsedDetailsfile;