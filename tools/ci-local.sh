#!/usr/bin/env bash
# tools/ci-local.sh
# Run the same checks as .github/workflows/{build,static-analysis}.yml locally.
#
# Usage:
#   tools/ci-local.sh                 # run every job
#   tools/ci-local.sh host cppcheck   # run only the named jobs
#
# Jobs: arm host coverage cppcheck clang-tidy clang-format doxygen

set -uo pipefail
cd "$(git rev-parse --show-toplevel)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

declare -A RESULTS

pass() { RESULTS["$1"]="PASS"; }
fail() { RESULTS["$1"]="FAIL"; }
skip() {
    RESULTS["$1"]="SKIP"
    echo -e "${YELLOW}[SKIP]${NC} $1 — $2"
}

job_arm() {
    echo "== build-arm =="
    cmake -B build/arm -DTARGET_PLATFORM=arm -DCMAKE_BUILD_TYPE=Debug -G Ninja \
        && cmake --build build/arm --parallel \
        && pass arm || fail arm
}

job_host() {
    echo "== build-host =="
    cmake -B build/host -DTARGET_PLATFORM=host -DCMAKE_BUILD_TYPE=Debug -G Ninja \
        && cmake --build build/host --parallel \
        && ctest --test-dir build/host --output-on-failure --parallel 4 \
        && pass host || fail host
}

job_coverage() {
    echo "== coverage =="
    if ! command -v gcovr &>/dev/null; then
        skip coverage "gcovr not installed (pip3 install --user gcovr)"
        return
    fi
    if [[ ! -d build/host ]]; then
        skip coverage "run the 'host' job first"
        return
    fi
    gcovr --root . --exclude tests/ --exclude third_party/ --exclude build/ \
        --xml coverage.xml --html coverage.html --html-details coverage-details/ --print-summary \
        && gcovr --root . --exclude tests/ --exclude third_party/ --exclude build/ --fail-under-line 0 \
        && pass coverage || fail coverage
}

job_cppcheck() {
    echo "== cppcheck =="
    if ! command -v cppcheck &>/dev/null; then
        skip cppcheck "cppcheck not installed"
        return
    fi

    local include_flags=() dir
    for dir in bsp/include app/include core/include ml/include; do
        [[ -d "$dir" ]] && include_flags+=("-I$dir")
    done

    local src_dirs=()
    for dir in bsp/src app/src bootloader/src ml/src; do
        if [[ -d "$dir" ]] && find "$dir" -name '*.c' -print -quit | grep -q .; then
            src_dirs+=("$dir")
        fi
    done

    if [[ ${#src_dirs[@]} -eq 0 ]]; then
        skip cppcheck "no source files yet"
        return
    fi

    cppcheck --enable=all --error-exitcode=1 --inline-suppr \
        --suppress=missingInclude --suppress=missingIncludeSystem \
        "${include_flags[@]}" "${src_dirs[@]}" \
        && pass cppcheck || fail cppcheck
}

job_clang_tidy() {
    echo "== clang-tidy =="
    if ! command -v run-clang-tidy &>/dev/null; then
        skip clang-tidy "run-clang-tidy not installed"
        return
    fi
    cmake -B build/tidy -DTARGET_PLATFORM=host -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja \
        && run-clang-tidy -p build/tidy -header-filter='(bsp|app|ml|bootloader)/.*\.h' \
        && pass clang-tidy || fail clang-tidy
}

job_clang_format() {
    echo "== clang-format =="
    if ! command -v clang-format &>/dev/null; then
        skip clang-format "clang-format not installed"
        return
    fi

    local dirs=() d
    for d in bsp app bootloader ml core; do
        [[ -d "$d" ]] && dirs+=("$d")
    done

    local files
    files=$(find "${dirs[@]}" -name "*.c" -o -name "*.h")
    if [[ -z "$files" ]]; then
        skip clang-format "no C sources yet"
        return
    fi

    echo "$files" | xargs clang-format --dry-run --Werror \
        && pass clang-format || fail clang-format
}

job_doxygen() {
    echo "== doxygen =="
    if ! command -v doxygen &>/dev/null; then
        skip doxygen "doxygen not installed"
        return
    fi

    mkdir -p build/docs
    if ! doxygen Doxyfile; then
        fail doxygen
        return
    fi

    if [[ -s build/docs/doxygen_warnings.txt ]]; then
        cat build/docs/doxygen_warnings.txt
        fail doxygen
    else
        pass doxygen
    fi
}

ALL_JOBS=(arm host coverage cppcheck clang-tidy clang-format doxygen)

jobs=("$@")
[[ ${#jobs[@]} -eq 0 ]] && jobs=("${ALL_JOBS[@]}")

for job in "${jobs[@]}"; do
    case "$job" in
        arm) job_arm ;;
        host) job_host ;;
        coverage) job_coverage ;;
        cppcheck) job_cppcheck ;;
        clang-tidy) job_clang_tidy ;;
        clang-format) job_clang_format ;;
        doxygen) job_doxygen ;;
        *)
            echo "Unknown job: $job (valid: ${ALL_JOBS[*]})" >&2
            exit 1
            ;;
    esac
    echo
done

echo "== Summary =="
failed=0
for job in "${jobs[@]}"; do
    status="${RESULTS[$job]:-?}"
    case "$status" in
        PASS)
            echo -e "  ${GREEN}PASS${NC}  $job"
            ;;
        FAIL)
            echo -e "  ${RED}FAIL${NC}  $job"
            failed=1
            ;;
        SKIP)
            echo -e "  ${YELLOW}SKIP${NC}  $job"
            ;;
        *)
            echo "  ????  $job"
            ;;
    esac
done

exit $failed
