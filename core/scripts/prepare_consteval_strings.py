"""
Copyright © 2025 Jonatan Nevo.
Distributed under the MIT license (see LICENSE file).
"""
import subprocess
import sys
import pathlib
import re

def find_consteval_strings(base_dir: pathlib.Path) -> list[str]:
    print(f"Looking for consteval strings in: {base_dir}")

    output = []
    for root, dirs, files in base_dir.walk(on_error=print):
        for name in files:
            if not (name.endswith(".cpp") or name.endswith(".h") or name.endswith(".hpp")) or (name.endswith("tests.cpp")):
                continue

            try:
                lines = (root / name).read_text().splitlines()
                for line in lines:
                    match = re.search( r'STRING_ID\("([^"]*)"\)', line )
                    if match:
                        output.append(match.group(1))
            except:
                print(f"Error reading file: {root / name}")

    return output

def make_hash_foreach_string(strings: list[str], executable: pathlib.Path) -> dict[int, str]:
    output = {}
    for string_to_hash in strings:
        result = subprocess.run([executable, string_to_hash], capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Error hashing string: {string_to_hash}")
            print(result.stderr)
        output[int(result.stdout)] = string_to_hash

    print(output)
    return output

def main():
    if len(sys.argv) != 4:
        print("Error: Expected exactly 3 arguments")
        print("Usage: python prepare_consteval_strings.py <base_dir> <output_path> <rapidhash_runner_path>")
        sys.exit(1)

    base_dir = pathlib.Path(sys.argv[1])
    output_path = pathlib.Path(sys.argv[2])
    rapidhash_runner_path = pathlib.Path(sys.argv[3])

    print(f"Rapidhash runner: {rapidhash_runner_path}")
    strings = find_consteval_strings(base_dir)
    hashes = make_hash_foreach_string(strings, rapidhash_runner_path)

if __name__ == '__main__':
    main()