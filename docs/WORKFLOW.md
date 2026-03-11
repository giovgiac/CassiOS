# Project Workflow

This document describes the tools used in the project and the workflow to follow when working with them.

## Tools

- **GitHub**: Issue tracking, version control, and code review (repo: `giovgiac/CassiOS`)

GitHub is accessed via the `gh` CLI.

## Labels

Use the default GitHub labels:

- **bug**: Something isn't working
- **enhancement**: New feature or request
- **duplicate**: This issue or pull request already exists
- **wontfix**: This will not be worked on

## Branching

Create branches from `main` using the naming format:

```
<type>/<issue-number>-<short-description>
```

Where `<type>` is one of `feature`, `bugfix`, `hotfix`, `refactor`, `docs`, or `chore`, and `<issue-number>` is the GitHub issue number.

Example: `feature/42-add-main-menu`, `bugfix/15-fix-keyboard-driver`

## Pull Requests

- **Title format**: `#<issue-number>: Short description`
- **Description**: Brief summary and a link to the GitHub issue (use `Closes #<issue-number>` to auto-close the issue on merge)
- **Review**: Every PR must be reviewed and explicitly approved by the user before merging. After opening a PR, stop and wait for the user to test and approve. Never merge without approval.
- **Merge strategy**: Squash merge (each PR becomes a single commit on main)
- **Commit title on main**: Must follow `#<issue-number>: Short description (#<PR>)` so every commit on main is traceable to its issue
- **After merge**: Delete the feature branch

## Issue Lifecycle

1. Create an issue in the repo with appropriate labels. Everything testable must be tested within the same issue -- only skip when something genuinely can't be tested or the effort is disproportionate. Never create separate issues for testing.
2. Create branch `<type>/<issue-number>-<description>` from `main`
3. Work on the branch, commit freely
4. Push the branch and open a PR with title `#<issue-number>: Description`, linking the issue with `Closes #<issue-number>`
5. **Stop and wait for the user to review, test, and approve the PR**
6. Address any feedback from the review
7. Once approved, squash merge to main, delete the branch (issue closes automatically)
