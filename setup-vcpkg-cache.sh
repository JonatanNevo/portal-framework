#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ENV_FILE="${SCRIPT_DIR}/.env"

case "$(uname -s)" in
  Darwin) OS=mac ;;
  Linux)  OS=linux ;;
  *)
    echo "Unsupported OS '$(uname -s)'. Use setup-vcpkg-cache.ps1 on Windows." >&2
    exit 1
    ;;
esac

if ! command -v mono >/dev/null 2>&1; then
  echo "'mono' is required to run nuget.exe but was not found in PATH." >&2
  if [ "$OS" = "mac" ]; then
    echo "Install via Homebrew: brew install mono" >&2
  else
    echo "Install via your package manager, e.g. 'sudo apt install mono-complete' or 'sudo pacman -S mono'." >&2
  fi
  exit 1
fi

OWNER=JonatanNevo
FEED_URL=https://nuget.pkg.github.com/${OWNER}/index.json
FEED_NAME="GitHubPackages"

VCPKG_EXPORT_VALUE="default,readwrite;nuget,${FEED_URL},readwrite"
VCPKG_NUGET_REPOSITORY="http://github.com/${OWNER}/portal-framework"

# Source the .env file if it exists
if [ -f "$ENV_FILE" ]; then
  echo "Sourcing $ENV_FILE"
  set -a
  # shellcheck disable=SC1090
  . "$ENV_FILE"
  set +a
fi

if [ -n "${GPR_USERNAME:-}" ] && [ -n "${GPR_KEY:-}" ]; then
  echo "Using GPR_USERNAME / GPR_KEY from environment"
else
  echo "GPR_USERNAME / GPR_KEY are not set."
  echo "Provide a GitHub username and a personal access token with 'read:packages' and 'write:packages' scopes."

  if [ -z "${GPR_USERNAME:-}" ]; then
    read -r -p "GitHub username: " GPR_USERNAME
  fi
  if [ -z "${GPR_KEY:-}" ]; then
    read -r -s -p "GitHub access token: " GPR_KEY
    echo
  fi

  if [ -z "$GPR_USERNAME" ] || [ -z "$GPR_KEY" ]; then
    echo "Both GPR_USERNAME and GPR_KEY are required." >&2
    exit 1
  fi

  # Make sure the file ends with a newline before appending
  if [ -f "$ENV_FILE" ] && [ -n "$(tail -c1 "$ENV_FILE")" ]; then
    printf '\n' >> "$ENV_FILE"
  fi

  if ! grep -q '^GPR_USERNAME=' "$ENV_FILE" 2>/dev/null; then
    printf 'GPR_USERNAME=%s\n' "$GPR_USERNAME" >> "$ENV_FILE"
  fi
  if ! grep -q '^GPR_KEY=' "$ENV_FILE" 2>/dev/null; then
    printf 'GPR_KEY=%s\n' "$GPR_KEY" >> "$ENV_FILE"
  fi

  export GPR_USERNAME GPR_KEY
fi


NUGET="$( "${SCRIPT_DIR}/vcpkg/vcpkg" fetch nuget | tail -n 1)"

mono "$NUGET" sources add \
  -Source "$FEED_URL" \
  -Name "$FEED_NAME" \
  -UserName "$GPR_USERNAME" \
  -Password "$GPR_KEY" \
  -StorePasswordInClearText

mono "$NUGET" setapikey "$GPR_KEY" -Source "$FEED_URL"


# Append VCPKG_BINARY_SOURCES and VCPKG_NUGET_REPOSITORY to .env if missing
touch "$ENV_FILE"
if [ -n "$(tail -c1 "$ENV_FILE")" ]; then
  printf '\n' >> "$ENV_FILE"
fi

if ! grep -q '^VCPKG_BINARY_SOURCES=' "$ENV_FILE"; then
  printf 'VCPKG_BINARY_SOURCES="%s"\n' "$VCPKG_EXPORT_VALUE" >> "$ENV_FILE"
  echo "Wrote VCPKG_BINARY_SOURCES to $ENV_FILE"
fi

if ! grep -q '^VCPKG_NUGET_REPOSITORY=' "$ENV_FILE"; then
  printf 'VCPKG_NUGET_REPOSITORY=%s\n' "$VCPKG_NUGET_REPOSITORY" >> "$ENV_FILE"
  echo "Wrote VCPKG_NUGET_REPOSITORY to $ENV_FILE"
fi

echo "vcpkg cache setup complete."
