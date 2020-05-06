#!/usr/bin/env perl

use strict;
use warnings;

use Readonly;
use Test::More;
use Convert::Bencode qw(bencode bdecode);
use LWP::UserAgent();

Readonly::Scalar my $ocelot_host => 'localhost';
Readonly::Scalar my $ocelot_port => 34000;

sub make_request {
    my ( $endpoint, $passkey, $params ) = @_;
    my $h   = LWP::UserAgent->new();
    my $req = URI->new(
        "http://${ocelot_host}:${ocelot_port}/" . "${passkey}/${endpoint}" );
    $req->query_form( %{$params} );
    my $rsp = $h->get( $req, %{$params} );
    ok( $rsp->is_success, 'http response is ok' );
    return $rsp->decoded_content;
}

sub make_site_request {
    my $params = shift;
    Readonly::Scalar my $site_password => '0' x 32;
    my $content = make_request( 'update', $site_password, $params );
    is( $content, 'success', 'site request successful' );
    return;
}

Readonly::Scalar my $passkey   => 'x' x 32;
Readonly::Scalar my $info_hash => 'a' x 40;
Readonly::Scalar my $peer_id   => 'A' x 20;

sub make_announce {
    my $reply = make_request( 'announce', $passkey,
        { info_hash => $info_hash, peer_id => $peer_id, compact => 1 } );
    return bdecode($reply);
}

make_site_request( { action => 'add_user', passkey => $passkey, id => 9000 } );
make_site_request(
    { action => 'add_torrent', info_hash => $info_hash, id => 42 } );
make_site_request( { action => 'add_whitelist', peer_id => $peer_id } );

my $reply = make_announce();
is( $reply->{downloaded}, 0, 'announce accepted' );

my $new_peer_id = ( $peer_id =~ s/A/B/gr );
make_site_request(
    {
        action      => 'edit_whitelist',
        old_peer_id => $peer_id,
        new_peer_id => $new_peer_id
    }
);

$reply = make_announce();
is(
    $reply->{'failure reason'},
    'Your client is not on the whitelist',
    'announce rejected'
);

make_site_request(
    {
        action  => 'remove_whitelist',
        peer_id => $new_peer_id
    }
);

$reply = make_announce();
is( $reply->{downloaded}, 0, 'announce accepted' );

done_testing();

exit 0;
