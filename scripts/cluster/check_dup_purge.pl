#!/usr/bin/perl -w

use strict;
use warnings;
use File::Basename;
use Cwd qw(abs_path);

my $wdir = abs_path(dirname(dirname($0)));
my $name;
opendir DIR, "$wdir/merge_rels/"
    or die "Cannot open directory `$wdir/merge_rels/': $!\n";
my @files = grep /\.rels\.\d+-\d+\.gz$/,
                         readdir DIR;
close DIR;
basename($files[0]) =~ /^(.*)\.rels\.\d+-\d+\.gz$/;
$name = $1;


##### CHECK RELS #####
my $rels;
my $rels_errors;
my $nrels = 0;
my $nrels_errors = 0;
my $newfilelist = "$wdir/$name.newfilelist";
open FILE, "> $newfilelist" or die "$newfilelist: $!";
for (@files) {
    next if (-e "$wdir/$_");
    system "$wdir/bin/check_rels -poly $wdir/$name.poly ".
           "$wdir/merge_rels/$_ 2> $wdir/$_.log > /dev/null";
    $rels_errors = `grep BUG "$wdir/$_.log"`;
    if ($rels_errors) {
        system "$wdir/bin/check_rels -poly $wdir/$name.poly -f ".
               "$wdir/merge_rels/$_ 2> $wdir/$_.log | gzip > $wdir/$_";
        $rels_errors = $1 if (`tail -1 "$wdir/$_.log"` =~ /([0-9]*) ERRORS/);
        chomp $rels_errors;
    } else {
        symlink "$wdir/merge_rels/$_", "$wdir/$_";
        $rels_errors = 0;
    }
    $rels = $1 if (`tail -1 "$wdir/$_.log"` =~ /([0-9]*) ok/);
    chomp $rels;
        
    $nrels += $rels;
    $nrels_errors += $rels_errors;
    print "Imported $rels relations ($rels_errors wrong relations) ".
         "from `".basename($_)."'.\n";
    print FILE "$_\n";
}
close FILE;
print "TOTAL : Imported $nrels relations ($nrels_errors wrong relations).\n";

##### PREP DUP #####
my $total_rels = $nrels;
my $nslices_log = 2;
my $nslices = 2**$nslices_log;

if (-e "$wdir/$name.nrels") {
    open FILE, "< $wdir/$name.nrels"
        or die "Cannot open `$wdir/$name.nrels' for reading: $!.\n";
    my $first = <FILE>;
    chomp $first;
    $total_rels += $first;
    close FILE;
} else {
    die "file `$wdir/$name.freerels.gz' not exist.\n" 
        unless (-e "$wdir/$name.freerels.gz");
    my $nfreerels = `gzip -dc "$wdir/$name.freerels.gz" | grep -v '#' | wc -l`;
    chomp $nfreerels;
    $total_rels += $nfreerels;
    open FILE, ">> $newfilelist" or die "$newfilelist: $!";
    print FILE "$name.freerels.gz";
    close FILE;
}

open FILE, "> $wdir/$name.nrels"
    or die "Cannot open `$wdir/$name.nrels' for writing: $!.\n";
print FILE "$total_rels\n";
close FILE;
print "Running total: $total_rels relations.\n";

opendir DIR, "$wdir/"
    or die "Cannot open directory `$wdir': $!\n";

my $pat=qr/^$name\.(rels\.[\de.]+-[\de.]+|freerels)\.gz$/;

my @all_files = grep /$pat/, readdir DIR;
closedir DIR;
mkdir "$wdir/$name.nodup"
        unless (-d "$wdir/$name.nodup");
for (my $i=0; $i < $nslices; $i++) {
    mkdir "$wdir/$name.nodup/$i"
        unless (-d "$wdir/$name.nodup/$i");
}
open FILE, "> $wdir/$name.filelist"
    or die "Cannot open `$wdir/$name.filelist' for writing: $!.\n";
for (@all_files) {
    m{([^/]+)$};
    print FILE "$_\n";
}
close FILE;

##### DUP AND PURGE #####
if ($nrels) {
    print "Removing duplicates...\n";
    print "  split new files in $nslices slices...\n";
    system "$wdir/bin/dup1 ".
           "-n $nslices_log ".
           "-out $wdir/$name.nodup ".
           "-filelist $wdir/$name.newfilelist ".
           "-basepath $wdir ".
           "2> $wdir/$name.dup1.log";
    
    open FILE, "> $wdir/$name.subdirlist"
        or die "Cannot open `$wdir/$name.subdirlist' for writing: $!.\n";
    print FILE join("\n", map { "$name.nodup/$_"; } (0..$nslices-1));
    close FILE;

    my $rels_after_dup = 0;
    my $last = 0;
    my $K = int ( 100 + (1.2 * $total_rels / $nslices) );
    for (my $i=0; $i < $nslices; $i++) {
        print "  removing duplicates on slice $i...\n";
        system "$wdir/bin/dup2 ".
               "-K $K -out $wdir/$name.nodup/$i ".
               "-filelist $wdir/$name.filelist ".
               "-basepath $wdir/$name.nodup/$i ".
               "2> $wdir/$name.dup2_$i.log";

        open FILE, "< $wdir/$name.dup2_$i.log"
            or die "Cannot open `$wdir/$name.dup2_$i.log' for reading: $!.\n";
        while (<FILE>) {
            $last = $1 if ( /^\s+(\d+) remaining relations/ );
        }
        close FILE;
        $rels_after_dup += $last;
    }

    print "  Number of relations left: $rels_after_dup.\n";
    print "Removing singletons...\n";
    system "$wdir/bin/purge ".
           "-poly $wdir/$name.poly -keep 160 -excess 1 ".
           "-nrels $rels_after_dup -out $wdir/$name.purged ".
           "-basepath $wdir " .
           "-subdirlist $wdir/$name.subdirlist ".
           "-filelist $wdir/$name.filelist ".
           "2> $wdir/$name.purge.log";
}

