# Agents

This repository uses or may interact with automated agents. This document documents the agents we expect to use during the Java migration and guidelines for interacting with them.

## Purpose

Agents are automated actors that can open branches, create changes, run tasks, or otherwise assist the maintainers. During the incremental migration to Java we expect to use agents for repetitive tasks such as:

- Creating branch scaffolding (example: `java` branch)
- Adding Bazel build files and language toolchains
- Adding or updating dependency declarations (e.g. rules_jvm_external / maven_install)
- Running automated formatting, linting or code generation (protobuf/codegen)
- Running CI tasks and test runners

## Known / Recommended Agents

- GitHub Copilot (Copilot for code and Copilot Batches/Tasks)
  - Can create branches and propose commits via PRs or direct pushes when configured.
  - Agent session/task logs can be found via Copilot Tasks URLs: /copilot/tasks/{task_id}

- Dependabot / Renovate
  - For automated dependency updates (Maven or Bazel deps). Configure as required.

- CI bots (GitHub Actions runners)
  - Run Bazel builds and tests. Keep CI configuration small while the java branch is experimental.

## Conventions

- Branches created by agents should use a predictable prefix (eg: `agent/` or `autogen/`) unless the change is explicitly reviewed.
- Agent changes that touch build files, toolchains, or dependency versions should always open a pull request for review unless explicitly authorized to push directly.
- Add a clear commit message including the agent name and a brief description, e.g. `copilot: add bazel java scaffold`.

## Security and Review

- Treat changes that add new binaries, toolchains, or external dependencies as security-sensitive. Require at least one human review before merging.
- Avoid giving agents broad write permissions across the repository unless absolutely necessary.

## Troubleshooting

- If an agent-created CI or build change fails, examine the workflow logs in GitHub Actions and the Copilot agent session logs where applicable.
- For Copilot agent sessions or task logs, use the Copilot Tasks URL pattern: https://github.com/copilot/tasks/{task_id}

## Local developer notes

- If you need to reproduce or fix agent-created commits locally, fetch the branch and inspect the changes:

  git fetch origin
  git checkout <agent-branch>

- When working on the `java` experimental branch, prefer opening PRs back to the default branch only after the incremental migration pieces are reviewed.


