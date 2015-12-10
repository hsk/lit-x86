#!/bin/bash

function check() {
	if [[ $($1) = $($2) ]] ; then
		printf "$1 ... \e[32mOK\e[00m\n"
	else
		printf "$1 ... \e[31mNO\e[00m\n"
	fi
}

check './lit ./examples/io.rb' 'cat io'
check './lit ./examples/fibo.rb' 'echo 1346269'
check './lit ./examples/pi.rb' 'cat test/pi'
check './lit ./examples/factlong.rb' 'cat test/long_factorial'
check './lit ./examples/primetable.rb' 'echo 664579'
check './lit ./examples/vararg.rb' 'echo 31'
