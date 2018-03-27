#!/usr/bin/env perl

use strict;

while (<STDIN>) {
    s/RasterImageView::AlphaBackgroundSolid/AbstractImageView::AlphaBackgroundSolid/;
    print $_;
}
