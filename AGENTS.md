# Repository Guidelines

## Project Structure & Module Organization
- `renderdoc/` contains the capture/replay core, API drivers in `driver/`, platform code in `os/`, and shared code.
- `qrenderdoc/` contains the Qt UI in `Code/`, `Windows/`, and `Widgets/`, with assets in `Resources/` and Python bindings in `Code/pyrenderdoc/`.
- `renderdoccmd/` is the CLI and unit-test entry point. `renderdocshim/` contains the injection shim.
- `docs/` holds Sphinx and contributor docs. `util/` contains build scripts, formatter binaries, installers, and graphics test assets under `util/test/`.

## Build, Test, and Development Commands
- Windows development build: `msbuild.exe renderdoc.sln /m /p:Configuration=Development /p:Platform=x64`
- Linux or macOS debug build: `cmake -DCMAKE_BUILD_TYPE=Debug -B build -H. && make -C build -j2`
- Android build: `cmake -DBUILD_ANDROID=On -DANDROID_ABI=armeabi-v7a -DANDROID_NATIVE_API_LEVEL=23 -B build-android-arm32 -H. && make -C build-android-arm32 -j2`
- Core unit tests: `build/bin/renderdoccmd test unit -o test.log` or `x64/Development/renderdoccmd.exe test unit -o test.log`
- UI unit tests: `build/bin/qrenderdoc --unittest log=test.log` or `x64/Development/qrenderdoc.exe --unittest log=test.log`
- Docs: `cd docs && make html SPHINXOPTS=-W && python3 verify-docstrings.py`

## Coding Style & Naming Conventions
- Format C/C++ with `clang-format` 15.0.7 and the checked-in `.clang-format`: 2-space indentation, no tabs, 100-column limit.
- Match surrounding code. Prefer explicit types over `auto` except for iterators and lambdas, use `NULL` instead of `nullptr`, and keep brace usage consistent across all branches.
- Prefer project containers and strings such as `rdcarray` and `rdcstr` over `std::vector` and `std::string`. Member variables use the `m_` prefix.
- Test and demo filenames usually follow `<api>_<feature>.cpp`, for example `d3d12_shader_debug_zoo.cpp`.

## Testing Guidelines
- Unit tests use Catch `TEST_CASE`s embedded in source files and run through `renderdoccmd test unit` or `qrenderdoc --unittest`.
- Broader regression coverage lives in `util/test/`. Build demos with `cmake -B build -H util/test/demos`, then use `python3 util/test/run_tests.py --list` or `-t <regex>`.
- There is no published coverage quota. CI expects targeted unit tests, doc generation, docstring verification, and manual testing around changed features.

## Commit & Pull Request Guidelines
- Keep commit subjects imperative and under 72 characters. Recent history uses summaries like `Remove ResourceUsage member view`.
- Rebase onto `v1.x`; do not include merge commits, standalone formatting commits, or leftover `WIP`/`fix stuff` commits in a PR.
- PRs should be ready for review, not drafts. Use the description to explain what changed and what you tested; link issues when relevant.
- Follow repository policy in `docs/CONTRIBUTING.md`: submitted code must not be LLM-generated, and copyright notices must not be changed.
