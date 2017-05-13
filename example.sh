#!/bin/sh

t() {
	curl --http1.0 -H 'User-Agent:' -s "$2" | tscrape | sed 's@$@	'$1'@g'
}

(
t 'tedunangst' 'https://twitter.com/tedunangst'      &
t 'richfelker' 'https://twitter.com/richfelker'      &
t 'transip'    'https://twitter.com/transip'         &
wait
) | sort -k 1rn -t "	" | awk -F "	" '{
	printf("%-16.16s  %-10.10s  %-20.20s: %s\n",
		$2, $6, $5, $3);
}'
