#!/bin/sh

# Exit with error message
# $1 exit value
# $2 error message
_exit_err() {
	printf "%s: %s\n" "$0" "$2" >&2
	exit "$1"
}

# Set terminal foreground color
# $1 color
_term_color() {
	tput setaf "$1" 2>/dev/null || printf "%b[3%sm" "\033" "$1"
}


# Execute file read/write tests
# $1 path to executable file
# $2 base path of all tests
# $total_count
# $passed_count
# $failed_count
# $failed_names
_test_file() {
	pos_path="${2}/file/positive"
	neg_path="${2}/file/negative"
	data_ext='.src'
	addr_ext='.addr'
	in_ext='.in'
	err_ext='.err'

	# Execute positive encode + decode tests
	pos_data_files="$(find "$pos_path" -type f -name "*${data_ext}")"
	for data_file in $pos_data_files; do
		test_name="$(basename "$data_file" | sed "s/${data_ext}\$//")"

		# Find files containing encoded results
		addr_files="$(find "$pos_path" -type f -name "${test_name}.*${addr_ext}")"
		[ -z "$addr_files" ] && _exit_err 3 "${data_file}: No accompanying ${addr_ext} files found"

		# Arrange - Get expected stdout of read
		read_expected="$(cat "$data_file")"

		for addr_file in $addr_files; do
			total_count=$((total_count + 2))

			# Arrange - Get exe arguments from name of file containing Library of Babel page address - expected to be "{test_name}.{encoding}.addr"
			encoding="$(basename "$addr_file" | sed "s/${addr_ext}\$//;s/^${test_name}\.//")"

			# Arrange - Get expected stdout of write
			write_expected="$(cat "$addr_file")"

			# Act - Execute and concat both stdout and stderr
			write_result="$("$exe_path" write -e "$encoding" "$data_file" 2>&1)"
			read_result="$("$exe_path" read -e "$encoding" "$addr_file" 2>&1)"

			# Assert write
			# - Execution should return expected stdout
			# - Execution should return no stderr - any error should cause assertion to fail
			if [ "$write_result" = "$write_expected" ]; then
				passed_count=$((passed_count + 1))
			else
				failed_count=$((failed_count + 1))
				failed_names="${failed_names}${pos_path}/${test_name}: write ${encoding}\n"
			fi

			# Assert read
			# - Execution should return expected stdout
			# - Execution should return no stderr - any error should cause assertion to fail
			if [ "$read_result" = "$read_expected" ]; then
				passed_count=$((passed_count + 1))
			else
				failed_count=$((failed_count + 1))
				failed_names="${failed_names}${pos_path}/${test_name}: decode ${encoding}\n"
			fi
		done
	done

	# Execute negative page search + get tests
	neg_in_files="$(find "$neg_path" -type f -name "*${in_ext}")"
	for in_file in $neg_in_files; do
		total_count=$((total_count + 1))

		# Arrange - Find file containing expected stderr
		err_file="$(printf "%s" "$in_file" | sed "s/${in_ext}\$/${err_ext}/")"
		! [ -f "$err_file" ] && _exit_err 3 "${in_file}: No accompanying ${err_ext} file found"

		# Arrange - Get exe arguments from path of file containing input - expected to be "{operation}/{test_name}.in"
		operation="$(basename "$(dirname "$in_file")")"

		# Arrange - Get expected stderr
		err_expected="$(cat "$err_file")"

		# Act - Execute and concat both stdout and stderr
		exe_result="$("$exe_path" "$operation" "$in_file" 2>&1)"

		# Assert
		# - Execution should return expected stderr
		# - Execution should return no stdout - any output should cause assertion to fail
		if [ "$exe_result" = "$err_expected" ]; then
			passed_count=$((passed_count + 1))
		else
			failed_count=$((failed_count + 1))
			failed_names="${failed_names}${neg_path}/${operation}/$(basename "$in_file" | sed "s/${in_ext}\$//")\n"
		fi
	done
}

# Execute encoding tests
# $1 path to executable file
# $2 base path of all tests
# $total_count
# $passed_count
# $failed_count
# $failed_names
_test_encoding() {
	pos_path="${2}/encoding/positive"
	neg_path="${2}/encoding/negative"
	decoded_ext='.src'
	encoded_ext='.enc'
	in_ext='.in'
	err_ext='.err'

	# Execute positive encode + decode tests
	pos_decoded_files="$(find "$pos_path" -type f -name "*${decoded_ext}")"
	for decoded_file in $pos_decoded_files; do
		test_name="$(basename "$decoded_file" | sed "s/${decoded_ext}\$//")"

		# Find files containing encoded results
		encoded_files="$(find "$pos_path" -type f -name "${test_name}.*${encoded_ext}")"
		[ -z "$encoded_files" ] && _exit_err 3 "${decoded_file}: No accompanying ${encoded_ext} files found"

		# Arrange - Get expected stdout of decode
		decode_expected="$(cat "$decoded_file")"

		for encoded_file in $encoded_files; do
			total_count=$((total_count + 2))

			# Arrange - Get exe arguments from name of file containing encoded result - expected to be "{test_name}.{encoding}.enc"
			encoding="$(basename "$encoded_file" | sed "s/${encoded_ext}\$//;s/^${test_name}\.//")"

			# Arrange - Get expected stdout of encode
			encode_expected="$(cat "$encoded_file")"

			# Act - Execute and concat both stdout and stderr
			encode_result="$("$exe_path" encode -e "$encoding" "$decoded_file" 2>&1)"
			decode_result="$("$exe_path" decode -e "$encoding" "$encoded_file" 2>&1)"

			# Assert encode
			# - Execution should return expected stdout
			# - Execution should return no stderr - any error should cause assertion to fail
			if [ "$encode_result" = "$encode_expected" ]; then
				passed_count=$((passed_count + 1))
			else
				failed_count=$((failed_count + 1))
				failed_names="${failed_names}${pos_path}/${test_name}: ${encoding} encode\n"
			fi

			# Assert decode
			# - Execution should return expected stdout
			# - Execution should return no stderr - any error should cause assertion to fail
			if [ "$decode_result" = "$decode_expected" ]; then
				passed_count=$((passed_count + 1))
			else
				failed_count=$((failed_count + 1))
				failed_names="${failed_names}${pos_path}/${test_name}: ${encoding} decode\n"
			fi
		done
	done

	# Execute negative encode + decode tests
	neg_in_files="$(find "$neg_path" -type f -name "*${in_ext}")"
	for in_file in $neg_in_files; do
		total_count=$((total_count + 1))

		# Arrange - Find file containing expected stderr
		err_file="$(printf "%s" "$in_file" | sed "s/${in_ext}\$/${err_ext}/")"
		! [ -f "$err_file" ] && _exit_err 3 "${in_file}: No accompanying ${err_ext} file found"

		# Arrange - Get exe arguments from path of file containing input - expected to be "{encoding}/{operation}/{test_name}.in"
		in_path="$(dirname "$in_file")"
		operation="$(basename "$in_path")"
		encoding="$(basename "$(dirname "$in_path")")"

		# Arrange - Get expected stderr
		err_expected="$(cat "$err_file")"

		# Act - Execute and concat both stdout and stderr
		exe_result="$("$exe_path" "$operation" -e "$encoding" "$in_file" 2>&1)"

		# Assert
		# - Execution should return expected stderr
		# - Execution should return no stdout - any output should cause assertion to fail
		if [ "$exe_result" = "$err_expected" ]; then
			passed_count=$((passed_count + 1))
		else
			failed_count=$((failed_count + 1))
			failed_names="${failed_names}${neg_path}/${encoding}/${operation}/$(basename "$in_file" | sed "s/${in_ext}\$//")\n"
		fi
	done
}

# Execute Library of Babel page get/search tests
# $1 path to executable file
# $2 base path of all tests
# $total_count
# $passed_count
# $failed_count
# $failed_names
_test_page() {
	pos_path="${2}/page/positive"
	neg_path="${2}/page/negative"
	text_ext='.txt'
	addr_ext='.addr'
	in_ext='.in'
	err_ext='.err'

	# Execute positive page search + get tests
	pos_text_files="$(find "$pos_path" -type f -name "*${text_ext}")"
	for text_file in $pos_text_files; do
		total_count=$((total_count + 2))

		# Arrange - Get test name
		test_name="$(basename "$text_file" | sed "s/${text_ext}\$//")"

		# Arrange - Find file containing expected page adddress
		addr_file="$(printf "%s" "$text_file" | sed "s/${text_ext}\$/${addr_ext}/")"
		! [ -f "$addr_file" ] && _exit_err 3 "${text_file}: No accompanying ${addr_ext} file found"

		# Arrange - Get expected stdouts
		text_expected="$(sed 's/ \+$//g' < "$text_file")" # Trim suffixed space chars
		addr_expected="$(cat "$addr_file")"

		# Act - Execute and concat both stdout and stderr
		text_result="$("$exe_path" page-get "$addr_file" 2>&1 | sed 's/ \+$//g')" # Trim suffixed space chars
		addr_result="$("$exe_path" page-search "$text_file" 2>&1)"

		# Assert page content
		# - Execution should return expected stdout
		# - Execution should return no stderr - any error should cause assertion to fail
		if [ "$text_result" = "$text_expected" ]; then
			passed_count=$((passed_count + 1))
		else
			failed_count=$((failed_count + 1))
			failed_names="${failed_names}${pos_path}/${test_name}: page-get\n"
		fi

		# Assert page address
		# - Execution should return expected stdout
		# - Execution should return no stderr - any error should cause assertion to fail
		if [ "$addr_result" = "$addr_expected" ]; then
			passed_count=$((passed_count + 1))
		else
			failed_count=$((failed_count + 1))
			failed_names="${failed_names}${pos_path}/${test_name}: page-search\n"
		fi
	done

	# Execute negative page search + get tests
	neg_in_files="$(find "$neg_path" -type f -name "*${in_ext}")"
	for in_file in $neg_in_files; do
		total_count=$((total_count + 1))

		# Arrange - Find file containing expected stderr
		err_file="$(printf "%s" "$in_file" | sed "s/${in_ext}\$/${err_ext}/")"
		! [ -f "$err_file" ] && _exit_err 3 "${in_file}: No accompanying ${err_ext} file found"

		# Arrange - Get exe arguments from path of file containing input - expected to be "{operation}/{test_name}.in"
		operation="$(basename "$(dirname "$in_file")")"

		# Arrange - Get expected stderr
		err_expected="$(cat "$err_file")"

		# Act - Execute and concat both stdout and stderr
		exe_result="$("$exe_path" "page-${operation}" "$in_file" 2>&1)"

		# Assert
		# - Execution should return expected stderr
		# - Execution should return no stdout - any output should cause assertion to fail
		if [ "$exe_result" = "$err_expected" ]; then
			passed_count=$((passed_count + 1))
		else
			failed_count=$((failed_count + 1))
			failed_names="${failed_names}${neg_path}/${operation}/$(basename "$in_file" | sed "s/${in_ext}\$//")\n"
		fi
	done
}

# Set config based on environment variables
[ -z "$NO_COLOR" ] && term_color=1 || term_color=0
[ "${LANG#*UTF-8}" != "$LANG" ] && term_unicode=1 || term_unicode=0

# Convert long options to short options
for arg in "$@"; do
	shift
	case "$arg" in
		"--ascii")    set -- "$@" "-a" ;;
		"--no-color") set -- "$@" "-p" ;;
		*)            set -- "$@" "$arg" ;;
	esac
done
OPTIND=1

# Parse options
while getopts ":ap" opt; do
	case "$opt" in
		a) term_unicode=0 ;;
		p) term_color=0 ;;
		\?) _exit_err 2 "-${OPTARG}: Option invalid" ;;
		:) _exit_err 2 "-${OPTARG}: Option requires an argument" ;;
	esac
done
shift $((OPTIND - 1))

readonly term_unicode
readonly term_color

# Get + validate path to executable file
exe_path="$1" && readonly exe_path
[ -z "$exe_path" ] && _exit_err 2 "Executable file not given"
! [ -f "$exe_path" ] && _exit_err 2 "${exe_path}: File not found"
! [ -x "$exe_path" ] && _exit_err 2 "${exe_path}: File not executable"

# Get base path of all tests
base_path="$(dirname "$0")" && readonly base_path

# Init test results
total_count=0
passed_count=0
failed_count=0
failed_names=

# Execute tests
_test_file "$exe_path" "$base_path"
_test_encoding "$exe_path" "$base_path"
_test_page "$exe_path" "$base_path"

# Init test result output
passed_prefix=
failed_prefix=
suffix=

if [ "$term_color" -eq 1 ]; then
	if [ "$failed_count" -gt 0 ]; then
		passed_prefix="${passed_prefix}$(_term_color 3)"
		failed_prefix="${failed_prefix}$(_term_color 1)"
	else
		passed_prefix="${passed_prefix}$(_term_color 2)"
	fi

	suffix="$(tput sgr0 2>/dev/null || printf "%b[m" "\033")"
fi

if [ "$term_unicode" -eq 1 ]; then
	passed_prefix="${passed_prefix}\0342\0234\0224 "
	failed_prefix="${failed_prefix}\0342\0234\0230 "
fi

# Output test results
echo "${passed_prefix}Passed: ${passed_count}/${total_count}${suffix}"

if [ "$failed_count" -gt 0 ]; then
	echo "${failed_prefix}Failed: ${failed_count}/${total_count}${suffix}"
	printf "%b" "$failed_names" | while read -r failed_name; do
		echo "${failed_name}"
	done
	exit 1
else
	exit 0
fi
