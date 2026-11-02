#!/usr/bin/env python3
"""Verify both committed library tarballs and lockfile before tests/builds."""
from pathlib import Path
from pinned_dependencies import DEPENDENCIES, verify_dependency


def main():
    root = Path(__file__).resolve().parents[1]
    for specification in DEPENDENCIES:
        pin, _files = verify_dependency(root, specification)
        print("Pinned " + pin["package"] + " " + pin["version"] + ": verified")


if __name__ == "__main__":
    main()
