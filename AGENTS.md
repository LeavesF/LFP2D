# Repository Editing Rules

- Treat source files as UTF-8 text.
- Do not rewrite source files with shell-based text operations such as `Get-Content` plus `Set-Content`, output redirection, or ad hoc scripts.
- Prefer minimal patch-based edits for `.h`, `.cpp`, `.cs`, `.md`, `.ini`, `.json`, `.uplugin`, and `.uproject` files.
- If a file cannot be parsed as UTF-8 or a diff shows widespread unrelated text changes, stop and recover the file from git before continuing.
- After editing, inspect the diff and confirm changes are limited to the intended logic.
