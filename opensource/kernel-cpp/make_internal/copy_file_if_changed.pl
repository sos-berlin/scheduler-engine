use File::Compare;
use File::Copy;

my $file1 = shift;
my $file2 = shift;
if (compare($file1,$file2) != 0) {
  copy($file1,$file2) or die "Copy failed: $!";
  print "file $file1 succesfully copied to $file2"
} else {
  print "file is up to date"
}
