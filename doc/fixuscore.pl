#!/usr/bin/perl
while (<>) {
  s,\\href{http://perso.club-internet.fr/lclevy/adflib/adf_info.html}{{\\tt http://perso.club-internet.fr/lclevy/adflib/adf_info.html}},\\href{http://perso.club-internet.fr/lclevy/adflib/adf_info.html}{{\\tt http://perso.club-internet.fr/lclevy/adflib/adf\\_info.html}},;
  s,\\href{http://www.win.tue.nl/~aeb/partitions/partition_tables-2.html}{{\\tt http://www.win.tue.nl/~aeb/partitions/partition_tables-2.html}},\\href{http://www.win.tue.nl/~aeb/partitions/partition_tables-2.html}{{\\tt http://www.win.tue.nl/~aeb/partitions/partition\\_tables-2.html}},;
  print $_;
}
exit 0;
