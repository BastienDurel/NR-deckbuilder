#!/usr/bin/perl -w
# -*- mode: cperl; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-
use WWW::Mechanize;
use HTML::TreeBuilder;
use Data::Dumper;

my @cards;
#my @sets = ('limited', 'proteus', 'classic');
my @sets = ('classic');

sub enlarge_card {
  my ($mech, $card) = @_;
  #JS: ref = window.open(("/popup/card/?id="+cardId), "netrunneronline", "status=0,toolbar=0,resizable=0,location=0,width=385,height=535");
  my $url = 'http://www.netrunneronline.com/popup/card/?id=' . $card;
  $mech->get($url);
  my @img = $mech->images();
  foreach my $img (@img) {
	if ($img->url() =~ /^\/images\/cards\//) {
	  print $img->url_abs(), "\n";
	  #TODO: get
	}
  }
}

sub get_raw_content {
  my ($p) = @_;

  my @pile = $p->content_list();
  my $tag;
  my $text = '';
  while (@pile) {
	if ( !defined( $pile[0] ) ) {    # undef!
	  # no-op
	}
	elsif ( !ref( $pile[0] ) ) {     # text bit!  save it!
	  $text .= shift @pile;
	}
	else {
	  $tag = $pile[0]->tag();
	  if ($tag eq 'img') {
		my $img = shift @pile;
		#$img->dump();
		my $alt = $img->attr('alt');
		if ($alt eq 'Subroutine') {
		  $text .= '@';
		}
		elsif ($alt eq 'Action') {
		  $text .= '[A]';
		}
		elsif ($alt eq 'Trash') {
		  $text .= '[T]';
		}
		else {
		  $text .= '[' . $alt . ']';
		}
	  }
	  else {
		$text .= get_raw_content(shift @pile);
	  }
	  if ($tag eq 'p') { $text .= "\n"; }
	  #shift @pile;
	}
  }
  return $text;
}

sub next_raw_content {
  my ($elem) = @_;
  my $p = $elem->parent();
  my @n = $p->content_list();
  #return $n[1]->as_text();
  my $txt = get_raw_content($n[1]);
  chop $txt if substr $txt, -1 eq "\n";
  return $txt;
}

sub get_td_containing {
  my ($match, $root) = @_;
  return $root->look_down("_tag", "td",
						  sub {
							my @c = $_[0]->content_list();
							foreach my $cc (@c) {
							  if ($cc =~ /^$match/i) { return 1; }
							}
							return 0;
						  } );
}

sub parse_content {
  my ($root) = @_;
  my %parsed;
  #warn Dumper($root);
  #$root->dump();

  # content is in div.smaller -> table -> table ->
  # Card Name: X
  # Rarity: Y
  # ...

  @div = $root->look_down("_tag", "div", "class", "smaller");
  foreach my $pdiv (@div) {
	# seek first td with a 'Card Name:' content
	my $name = get_td_containing('Card Name:', $pdiv);
	next if !defined $name;
	$parsed{name} = next_raw_content($name);
	#$name->dump();
	#print "name: ", next_raw_content($name), "\n";
	my $table = $name->look_up("_tag", "table");
	#$table->dump();

	my $text = get_td_containing('Rules Text:', $pdiv);
	$parsed{text} = next_raw_content($text) if (defined $text);

	my $tflavor = get_td_containing('Flavor Text:', $pdiv);
	$parsed{flavor} = next_raw_content($tflavor) if (defined $tflavor);

	my $key = get_td_containing('Card Type:', $pdiv);
	$parsed{keywords} = next_raw_content($key) if (defined $key);

	my $str = get_td_containing('Strength:', $pdiv);
	$parsed{points} = next_raw_content($str) if (defined $str);
	$str = get_td_containing('Ice strength:', $pdiv);
	$parsed{points} = next_raw_content($str) if (defined $str);
	$str = get_td_containing('Trash cost:', $pdiv);
	$parsed{points} = next_raw_content($str) if (defined $str);
	my $diff = get_td_containing('Agenda points:', $pdiv);
	$parsed{points} = next_raw_content($diff) if (defined $diff);

	my $cost = get_td_containing('Installation Cost:', $pdiv);
	$parsed{cost} = next_raw_content($cost) if (defined $cost);
	$cost = get_td_containing('Rez cost:', $pdiv);
	$parsed{cost} = next_raw_content($cost) if (defined $cost);
	$cost = get_td_containing('Cost:', $pdiv);
	$parsed{cost} = next_raw_content($cost) if (defined $cost);
	$cost = get_td_containing('Difficulty:', $pdiv);
	$parsed{cost} = next_raw_content($cost) if (defined $cost);

	my $rarity = get_td_containing('Rarity:', $pdiv);
	$parsed{rarity} = next_raw_content($rarity) if (defined $rarity);

	my $player = get_td_containing('Player:', $pdiv);
	$parsed{player} = next_raw_content($player) if (defined $player);

	my $set = get_td_containing('Set:', $pdiv);
	$parsed{set} = next_raw_content($set) if (defined $set);

	#my $ = get_td_containing(':', $pdiv);
	#$parsed{} = next_raw_content($) if (defined $);


	last;
  }
  return %parsed;
}

## test
my $troot = HTML::TreeBuilder->new();
$troot->parse_file('sample.html');
my %p = parse_content($troot);
warn Dumper(\%p);
my $troot2 = HTML::TreeBuilder->new();
$troot2->parse_file('sample2.html');
%p = parse_content($troot2);
warn Dumper(\%p);
my $troot3 = HTML::TreeBuilder->new();
$troot3->parse_file('sample3.html');
%p = parse_content($troot3);
warn Dumper(\%p);
my $troot4 = HTML::TreeBuilder->new();
$troot4->parse_file('sample4.html');
%p = parse_content($troot4);
warn Dumper(\%p);
my $troot5 = HTML::TreeBuilder->new();
$troot5->parse_file('sample5.html');
%p = parse_content($troot5);
warn Dumper(\%p);
exit 0;

my $mech = WWW::Mechanize->new();
foreach my $set (@sets) {
  $mech->get( "http://www.netrunneronline.com/set/$set/cards/" );

  @links = $mech->links();
  foreach my $link (@links) {
	if ($link->url =~ /^\/cards\/.*\//) {
	  #print $link->url_abs(), "\n";
	  push @cards, $link->url_abs();
	}
  }
}

foreach my $card (@cards) {
  $mech->get($card);
  #warn Dumper($mech->content());
  my $root = HTML::TreeBuilder->new();
  $root->parse_content($mech->content());

  parse_content($root);

  @links = $mech->links();
  foreach my $link (@links) {
	#print $link->url_abs(), "\n";
	if ($link->url =~ /javascript:enlargeCard\('(.*?)'\);/) {
	  enlarge_card($mech, $1);
	}
  }
  last;
}
