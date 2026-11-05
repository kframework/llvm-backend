## LLVM Backend for KORE

This project implements a fast concrete rewriting engine for KORE, the internal
representation of the [K Framework][K], along with a set of related tools and
utilities. Direct usage of this repository is intended for advanced K users and
core developers; most use cases are supported by the user-facing K tools.

Internal documentation for core developers and advanced K users can be found in
the [`docs/`](docs) directory.

For developer setup instructions, refer to [INSTALL.md](INSTALL.md).

## Git Workflow

PRs to this repository should target the `develop` branch, rather than `master`.

[K]: https://github.com/runtimeverification/k
