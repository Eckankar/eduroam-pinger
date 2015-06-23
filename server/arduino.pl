#!/usr/bin/env perl
use 5.012;
use warnings;

use Mojolicious::Lite;

use CHI;
use DBI;
use JSON qw/from_json to_json/;
use LWP::UserAgent;

sub get_db {
    my $db = DBI->connect('dbi:Pg:dbname=arduino', 'arduino', 'password');
    return $db;
}

get '/eduroam/ping' => sub {
    my $c = shift;

    my $db = get_db();
    $db->do("INSERT INTO eduroam (time) VALUES (current_timestamp);");
    $db->commit;

    $c->render(text => 'OK');
};

get '/eduroam/data' => sub {
    my $c = shift;

    my $db = get_db();
    my $stm = $db->prepare(<<SQL);
SELECT date_trunc('hour', time) + date_part('minute', time)::int / 5 * interval '5 min' as t_interval,
       COUNT(*) as count
FROM eduroam
WHERE time > date_trunc('day', current_timestamp - INTERVAL '7 days')
GROUP BY t_interval
ORDER BY t_interval ASC
SQL

    $stm->execute();
    my %res;

    $res{pings} = [];
    while (my $row = $stm->fetchrow_hashref) {
        push($res{pings}, $row);
    }

    $c->res->headers->add('Access-Control-Allow-Origin' => '*');
    $c->render(json => \%res);
};

app->start;
