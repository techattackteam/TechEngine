# Script-mode (-P) half of the `coverage` target. Runs after ctest has produced the
# .profraw files. Kept out of the custom target because globbing needs CMake, not a shell.
#
# Passed in: TE_COV_DIR · TE_LLVM_PROFDATA · TE_LLVM_COV · TE_DIFF_COVER · TE_SOURCE_DIR ·
# TE_OBJECTS ('|'-joined test binaries). Threshold, base branch and the bypass are read
# from the environment, so CI and a local run issue the same command.

file(GLOB _te_profraw "${TE_COV_DIR}/*.profraw")
if(NOT _te_profraw)
  message(FATAL_ERROR "No .profraw in ${TE_COV_DIR}. Did ctest run under LLVM_PROFILE_FILE?")
endif()

set(_te_profdata "${TE_COV_DIR}/merged.profdata")
execute_process(COMMAND "${TE_LLVM_PROFDATA}" merge -sparse ${_te_profraw} -o "${_te_profdata}"
                RESULT_VARIABLE _te_result)
if(NOT _te_result EQUAL 0)
  message(FATAL_ERROR "llvm-profdata merge failed (${_te_result}). A version skew between "
                      "clang and llvm-profdata is the usual cause.")
endif()

string(REPLACE "|" ";" _te_objects "${TE_OBJECTS}")
list(POP_FRONT _te_objects _te_first)
set(_te_object_args "")
foreach(_te_object IN LISTS _te_objects)
  list(APPEND _te_object_args -object "${_te_object}")
endforeach()

# Deps are never instrumented (the flags ride te_warnings), but our own TUs still carry
# mapping records for dependency headers they inline.
#
# GOTCHA: this was `-ignore-filename-regex=/_deps/`, which is FetchContent's DEFAULT directory
# and not the one CI uses — ci.yml passes FETCHCONTENT_BASE_DIR=<workspace>/.deps. So it
# matched every local run and nothing in CI, and inlined catch2 headers were scored against
# the merge threshold. The positional root list is the fix: llvm-cov keeps only files under
# these paths, whatever the deps directory is called.
#
# Test sources are dropped too. A test file is executed by definition, so counting it lets a
# large test diff carry an untested feature over the threshold.
set(_te_roots "${TE_SOURCE_DIR}/engine" "${TE_SOURCE_DIR}/apps" "${TE_SOURCE_DIR}/sdk")
set(_te_common "-instr-profile=${_te_profdata}" "${_te_first}" ${_te_object_args}
               "-ignore-filename-regex=(/tests/|/build/)" ${_te_roots})

set(_te_lcov "${TE_COV_DIR}/coverage.lcov")
execute_process(COMMAND "${TE_LLVM_COV}" export -format=lcov ${_te_common}
                OUTPUT_FILE "${_te_lcov}" RESULT_VARIABLE _te_result)
if(NOT _te_result EQUAL 0)
  message(FATAL_ERROR "llvm-cov export failed (${_te_result}).")
endif()

execute_process(COMMAND "${TE_LLVM_COV}" show -format=html
                        -output-dir=${TE_COV_DIR}/html ${_te_common}
                RESULT_VARIABLE _te_result)
if(NOT _te_result EQUAL 0)
  message(FATAL_ERROR "llvm-cov show failed (${_te_result}).")
endif()

message(STATUS "Browsable report: ${TE_COV_DIR}/html/index.html")

# diff-cover compares against the merge base by default ('...' range notation), which is
# the same thing CI wants and the same thing a local pre-push check wants.
set(_te_base "$ENV{TE_COVERAGE_BASE}")
if(NOT _te_base)
  set(_te_base "origin/master")
endif()

set(_te_threshold "$ENV{TE_COVERAGE_THRESHOLD}")
if(NOT _te_threshold)
  set(_te_threshold "85")
endif()

set(_te_markdown "${TE_COV_DIR}/diff-coverage.md")

# --exclude repeats the lcov's test filter rather than trusting it. A changed file absent
# from the report is diff-cover's decision to make, not ours, and this takes the decision
# away: the paths are excluded by name whatever the report holds.
#
# App.cpp is the composition root, and no CI job runs the runtime exe. Its demo blocks are
# therefore uncoverable by construction, and a card that ships one would fail the floor on
# lines nobody can test. Both spellings, same reason as the tests pair above.
execute_process(COMMAND "${TE_DIFF_COVER}" "${_te_lcov}"
                        "--compare-branch=${_te_base}"
                        "--fail-under=${_te_threshold}"
                        "--exclude" "*/tests/*" "tests/*"
                                    "engine/app/src/App.cpp" "*/engine/app/src/App.cpp"
                        "--format" "markdown:${_te_markdown}"
                WORKING_DIRECTORY "${TE_SOURCE_DIR}"
                OUTPUT_VARIABLE _te_output ECHO_OUTPUT_VARIABLE
                ERROR_VARIABLE _te_output ECHO_ERROR_VARIABLE
                RESULT_VARIABLE _te_result)

# GitHub appends step summaries, and diff-cover truncates whatever it writes to, so the
# report is written to its own file first and appended here.
if(EXISTS "${_te_markdown}")
  file(READ "${_te_markdown}" _te_report)
  if(DEFINED ENV{GITHUB_STEP_SUMMARY})
    file(APPEND "$ENV{GITHUB_STEP_SUMMARY}" "${_te_report}")
  endif()
endif()

if(_te_result EQUAL 0)
  return()
endif()

# diff-cover exits 1 for a real under-threshold verdict, and writes its report on the way
# out. No report means it never got that far, so this is a tool failure wearing a
# threshold failure's exit code. Saying so beats sending someone to write tests they do
# not need.
if(NOT _te_result EQUAL 1 OR NOT EXISTS "${_te_markdown}")
  message(FATAL_ERROR "diff-cover did not complete (result: ${_te_result}). "
                      "This is not a coverage verdict.
${_te_output}")
endif()

# The explicit bypass is checked BEFORE the floor, and the order is the point. Both let the
# run pass, so which one fires changes nothing about the exit code — it changes what the log
# says happened. A human wrote [skip-coverage] on purpose; the floor is an automatic fallback.
# Reporting the fallback while a deliberate signal sat unread hides whether the signal was
# even received, which is exactly how the frozen-payload bug stayed invisible.
if(NOT "$ENV{TE_COVERAGE_BYPASS}" STREQUAL "")
  message(STATUS "Diff coverage is below ${_te_threshold}%, bypassed by [skip-coverage].")
  return()
endif()

# A floor, not a second threshold. Under a handful of lines the percentage stops measuring
# anything: at four measurable lines one line is worth 25 points, so a rename plus one
# unreachable error branch reads as a failure with nothing to fix.
#
# The count is parsed from diff-cover rather than recomputed. diff-cover already intersected
# the diff with the report, and a second implementation of that would eventually disagree
# with the number the percentage was built from. Parsing its output is only safe because
# coverage.cmake pins the version and checks it at configure time. A parse miss falls through
# to the verdict below, never to a pass.
set(_te_floor "$ENV{TE_COVERAGE_MIN_LINES}")
if(NOT _te_floor)
  set(_te_floor "10")
endif()

if(_te_report MATCHES "Total[^0-9]+([0-9]+) lines")
  set(_te_total "${CMAKE_MATCH_1}")
  if(_te_total LESS _te_floor)
    message(STATUS "Diff coverage is below ${_te_threshold}%, but only ${_te_total} "
                   "measurable lines changed (floor: ${_te_floor}). Too small to score.")
    return()
  endif()
endif()

message(FATAL_ERROR "Diff coverage is below ${_te_threshold}%. Add [skip-coverage] to the "
                    "pull request description to merge anyway.")
