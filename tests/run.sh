#!/bin/sh
# usage: tests/run.sh [path/to/js-pdl]
# pass/*.js must exit 0 with no stderr; fail/**/*.js must exit 1..127 and
# report something on stderr. A signal (rc >= 128, e.g. 139 = SIGSEGV) is
# always wrong, so a crash never counts as a correct rejection.
dir=$(cd "$(dirname "$0")" && pwd)
bin=${1:-"$dir/../build/js-pdl"}
ok=0; bad=0

check() { # file expect(pass|fail)
	err=$("$bin" "$1" 2>&1 >/dev/null); rc=$?
	if [ "$2" = pass ] && [ $rc -eq 0 ] && [ -z "$err" ]; then got=pass
	elif [ "$2" = fail ] && [ $rc -gt 0 ] && [ $rc -lt 128 ] && [ -n "$err" ]; then got=fail
	else got=wrong; fi
	if [ "$got" = wrong ]; then
		bad=$((bad + 1)); [ $rc -ge 128 ] && what="CRASH" || what="FAIL "
		printf '%s %s (expected %s, rc=%d)\n' "$what" "${1#$dir/}" "$2" "$rc"
		[ -n "$err" ] && printf '      %s\n' "$err"
	else
		ok=$((ok + 1))
	fi
}

for f in "$dir"/pass/*.js; do check "$f" pass; done
for f in $(find "$dir/fail" -name '*.js' | sort); do check "$f" fail; done
printf '%d ok, %d wrong\n' "$ok" "$bad"
[ $bad -eq 0 ]
