"""Build each Qt Test project in a separate shadow directory and run its check target."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    test_root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", type=Path, help="parent directory for shadow builds")
    parser.add_argument("--qmake", default="qmake", help="qmake from the selected Qt 6 Kit")
    parser.add_argument("--make", default="mingw32-make" if os.name == "nt" else "make")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--qmake-arg", action="append", default=[], help="optional Kit-specific qmake argument")
    args = parser.parse_args()
    qmake = shutil.which(args.qmake)
    make = shutil.which(args.make)
    if not (qmake and make and args.jobs > 0):
        parser.error("select a Qt Kit, its make tool, and a positive job count")
    build_root = args.build_dir.resolve()
    if test_root == build_root or test_root in build_root.parents:
        parser.error("choose a build directory outside the test sources")
    env = os.environ.copy()
    env["PATH"] = str(Path(qmake).resolve().parent) + os.pathsep + env.get("PATH", "")
    env.setdefault("QT_QPA_PLATFORM", "offscreen")
    projects = sorted(test_root.rglob("*.pro"))
    for project in projects:
        relative = project.relative_to(test_root)
        directory = build_root / relative.with_suffix("")
        directory.mkdir(parents=True, exist_ok=True)
        log_path = directory / "test-run.log"
        print(f"Building and running {relative}", flush=True)
        commands = ([qmake, str(project), "CONFIG+=release", "CONFIG-=debug_and_release", *args.qmake_arg],
                    [make, "-j" + str(args.jobs), "check", "TESTARGS=-txt"])
        with log_path.open("wb") as log:
            for command in commands:
                result = subprocess.run(command, cwd=directory, env=env, stdout=log, stderr=subprocess.STDOUT)
                if result.returncode:
                    log.flush()
                    lines = log_path.read_text(encoding="utf-8", errors="replace").splitlines()
                    print("\n".join(lines[-35:]), file=sys.stderr)
                    print(f"Full log: {log_path}", file=sys.stderr)
                    return result.returncode
        lines = log_path.read_text(encoding="utf-8", errors="replace").splitlines()
        print(next((line for line in reversed(lines) if line.startswith("Totals:")), "check target passed"), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
