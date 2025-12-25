# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html
import os

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Portal Framework'
copyright = 'Copyright © 2025, Jonatan Nevo'
author = 'Jonatan Nevo'
release = '0.1.6'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'breathe',
    'exhale',
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.viewcode',
    'sphinx.ext.todo',
    'myst_parser',
    "sphinx_inline_tabs",
    'sphinx_copybutton'
]

templates_path = ['_templates']
exclude_patterns = [
    '.venv',
    '_build',
    'Thumbs.db',
    '.DS_Store',
]

source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

# -- Breathe configuration ---------------------------------------------------
# Breathe is the bridge between Doxygen XML and Sphinx

breathe_projects = {
    "PortalFramework": os.environ.get(
        'BREATHE_PROJECTS_PATH',
        os.path.join(os.path.dirname(__file__), '../build/doxygen/xml')
    )
}

breathe_default_project = "PortalFramework"
breathe_default_members = ('members', 'undoc-members')

exhale_args = {
    "containmentFolder": "./api",
    "rootFileName": "library_root.rst",
    "doxygenStripFromPath": "../../",
    # Heavily encouraged optional argument (see docs)
    "rootFileTitle": "Library API",
    # Suggested optional arguments
    "createTreeView": True,
    # TIP: if using the sphinx-bootstrap-theme, you need
    # "treeViewIsBootstrap": True,
    "exhaleExecutesDoxygen": False,
}

primary_domain = 'cpp'
highlight_language = 'cpp'

myst_enable_extensions = [
    "colon_fence",  # ::: fence for directives
    "deflist",  # Definition lists
    "tasklist",  # Task lists with [ ] and [x]
    "fieldlist",  # Field lists
]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'shibuya'
html_static_path = ['_static']
html_logo = '_static/portal_icon_64x64.png'
html_title = "Portal Framework"

html_theme_options = {
    "accent_color": "iris",
    "github_url": "https://github.com/JonatanNevo/portal-framework"
}

todo_include_todos = True
