# Portal Framework Documentation

This directory contains the documentation build infrastructure for the Portal Framework using Sphinx + Breathe + Doxygen.

## Prerequisites

### Required Tools

1. **UV** - Fast Python package installer and resolver
   - **Installation (Windows)**:
     ```powershell
     powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
     ```
   - **Installation (Linux/macOS)**:
     ```bash
     curl -LsSf https://astral.sh/uv/install.sh | sh
     ```
   - **Verification**:
     ```bash
     uv --version
     ```

2. **Doxygen** - API documentation generator
   - Download from: https://www.doxygen.nl/download.html
   - Ensure `doxygen` is in your PATH

3. **CMake** - Build system (already required for the main project)

**Installation**:
```bash
# From the docs directory
cd docs
uv sync
```

This will create a virtual environment at `docs/.venv` and install all dependencies specified in `pyproject.toml`.

## Building Documentation

1. **Configure CMake with documentation enabled**:
   ```bash
   cmake -B build -DPORTAL_BUILD_DOCS=ON
   ```

2. **Build the documentation**:
   ```bash
   cmake --build build --target docs
   ```

3. **View the documentation**:
   Open `build/<build-dir>/docs/html/index.html` in your browser (e.g., `build/ninja-multi/docs/html/index.html`).

## Development Workflow

### Live Reload During Documentation Writing

Use `sphinx-autobuild` for automatic rebuilds during documentation editing:

```bash
cd docs
uv sync --extra dev
sphinx-autobuild source api/html --watch ../core --watch ../engine
```

This will:
- Start a local web server (default: http://127.0.0.1:8000)
- Automatically rebuild on file changes
- Auto-refresh your browser