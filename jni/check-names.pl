#!/usr/bin/perl -w

######################################################################
# A simple duplicity function checker.
# Checks for duplicit non-static functions (and variables).
#
# It parses the output of the ctags standard utility:
#   $ ctags -x --links=no -R ffmpeg/ faad2/ opencore-aacdec/ > tags.txt
#   $ ./check-names.pl tags.txt 
#
# This utility expects that the input file contains the following information:
# <name>           <type>   <line> <file> <detail>                       
# A                macro      3324 ffmpeg/libavcodec/dsputil.c #define A 0
# get_int          member       30 ffmpeg/libavformat/mvi.c unsigned int (*get_int)(ByteIOContext *);
# get_adif_header  function   2310 faad2/libfaad/syntax.c void get_adif_header(adif_header *adif, bitfile *ld)
# get_adts_header  function    325 opencore-aacdec/src/get_adts_header.c Int get_adts_header(
# get_aiff_header  function     86 ffmpeg/libavformat/aiffdec.c static unsigned int get_aiff_header(ByteIOContext *pb, AVCodecContext *codec,
#
######################################################################

use strict;

my %tags = ();
my @duplicit = ();

while (<>) {
    chomp;
    my ($tag,$type,$lineno,$file, $detail) = split( /\s+/, $_, 5);

    next unless $type eq 'function';
    #next unless $type eq 'function' or $type eq 'variable';
    next if $detail =~ m/^static /;

    my ($module,$dummy) = split( /\//, $file, 2);

    unless (exists $tags{$tag}) {
        $tags{$tag} = $module;
        next;
    }

    my $rec = $tags{$tag};
    if (ref($rec)) {
        my $found = 0;
        foreach (@$rec) {
            if ($_ eq $module) { $found=1; last }
        }

        push @{$tags{$tag}}, $module unless $found;
    }
    elsif ($module ne $rec) {
        $tags{$tag} = [$rec,$module];
        push @duplicit, $tag;
    }
}

print "Found total ", scalar(@duplicit), " duplicit tags:\n";

foreach my $tag (@duplicit) {
    print "Tag '$tag' in ", join( ", ", @{$tags{$tag}}), "\n";
}

