# Repository Guidelines

## Project Structure & Module Organization
`renderdoc/` contains the core capture, replay, driver, and platform code. `qrenderdoc/` holds the Qt UI, widgets, styles, and packaged resources. `renderdoccmd/` contains the command-line frontend, while `renderdocshim/` contains the injection shim. Contributor docs live under `docs/CONTRIBUTING/`, and automated test assets and harnesses live in `util/test/` (`tests/`, `rdtest/`, `data/`, and demo apps under `demos/`).

## Build, Test, and Development Commands
- `cmake -DCMAKE_BUILD_TYPE=Debug -B build -H .` — configure an out-of-source desktop build.
- `cmake --build build --parallel` — build the configured CMake tree.
- `msbuild.exe renderdoc.sln /m /p:Configuration=Development /p:Platform=x64` — build the main Windows solution.
- `python util/test/run_tests.py -l` — list available automated tests.
- `python util/test/run_tests.py --renderdoc <build> --pyrenderdoc <build>/pymodules -t VK_Custom_Resolve` — run a focused test.
- `git clang-format` or `bash util/clang_format_all.sh` — apply repository formatting rules.

## Coding Style & Naming Conventions
Use the repository `.clang-format` with `clang-format-15.0.7`; CI enforces formatting. C++ uses 2-space indentation, no tabs, 100-column lines, and C++14. Match nearby code rather than introducing new patterns. Prefer explicit types over `auto` except where the project already allows it, use `NULL` instead of `nullptr`, and prefer `rdcarray`/`rdcstr` over `std::vector`/`std::string`. Avoid Hungarian notation except `m_` for members.

## Testing Guidelines
Testing is still partly ad hoc, so validate the area you changed and note what you covered in the PR. Add new API tests under `util/test/tests/<API>/` and keep names aligned with existing patterns such as `VK_Custom_Resolve.py` or `D3D12_Shader_Editing.py`. When adding demo coverage, extend `util/test/demos/` with a small focused case instead of an all-in-one sample.

## Commit & Pull Request Guidelines
Recent history uses short, imperative subjects such as `Fix detection of semantic arrays...` and `Add correct return type...`. Keep the first line under 72 characters, add a body when useful, and squash formatting-only or compile-fix commits into the relevant change. Rebase onto `v1.x`; do not include merge commits and do not open draft PRs. Keep PRs reviewable, describe the change clearly, link related issues, and include validation steps; attach screenshots when changing `qrenderdoc/` UI.

## Contributor Notes
Read `docs/CONTRIBUTING.md` before large changes. The project explicitly forbids LLM-generated code contributions and asks contributors not to modify copyright headers.
