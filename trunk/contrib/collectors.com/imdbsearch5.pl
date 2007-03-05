sub EscapeChars {
  my $text = @_[0];

  $text =~ s/&(\w*?\s)/&amp;$1/g;
  $text =~ s/</&lt;/g;

  return $text;
};

($dir) = ( $0 =~ /(.*[\\\/])Scripts[\\\/].*?\.pl/);

open SearchResultsfile, "<", $dir."searchresult.html";
open ParsedResultsfile, ">", $dir."parsedresults.xml";

print ParsedResultsfile "<queryresults>";

#read the whole HTML file into variable $line
undef $/;
$line = <SearchResultsfile>;

if ($line =~ /IMDb Title  Search/s) {
#search results page
$before_imdb = '<a href="/title/tt';
$between_imdb_title = '/.*?>';
$after_title = '</a> \(';
$after_year = '\)';
$imdbexp = '\d{7}';

while ($line =~ /\G.*?$before_imdb($imdbexp)$between_imdb_title(.*?)$after_title(.*?)$after_year/gs) {

#search and replace high-ASCII chars in $1, $2 and $3 here
$imdb = EscapeChars($1);  
$title = EscapeChars($2);  
$year = EscapeChars($3);  

if ($imdb ne '') {   
  print ParsedResultsfile "<match source='IMDB'><description>$title ($year)</description><id name='imdbnr'>$imdb</id></match>\n";
};
};

} else
{
#details page
$bimdb = 'pro\.imdb\.com/title/tt';
$aimdb = '/">';

$btitle = '<title>';
$atitle = ' \(';

$byear = '<title>.*? \(';
$ayear = '\)';

($imdb) = ($line =~ /$bimdb(\d+?)$aimdb/s);
($title) = ($line =~ /$btitle(.*?)$atitle/s);
($year) = ($line =~ /$byear(\d+?)$ayear/s);

#search and replace high-ASCII chars in $1, $2 and $3 here
$imdb = EscapeChars($imdb);  
$title = EscapeChars($title);  
$year = EscapeChars($year);  

if ($imdb ne '') {  
  print ParsedResultsfile "<match source='IMDB'><description>$title ($year)</description><id name='imdbnr'>$imdb</id></match>\n";
};
};

print ParsedResultsfile "</queryresults>";

close SearchResultsfile;
close ParsedResultsfile;