# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html
import os

import exhale
import exhale.configs
import exhale.utils
import exhale.deploy

import os
import os.path
from pprint import pprint


def exhale_environment_ready(app, *args):
    default_project = app.config.breathe_default_project
    default_exhale_args = dict(app.config.exhale_args)

    exhale_projects_args = dict(app.config._raw_config['exhale_projects_args'])
    breathe_projects = dict(app.config._raw_config['breathe_projects'])

    for project in breathe_projects:
        app.config.breathe_default_project = project
        os.makedirs(breathe_projects[project], exist_ok=True)

        project_exhale_args = exhale_projects_args.get(project, {})

        app.config.exhale_args = dict(default_exhale_args)
        app.config.exhale_args.update(project_exhale_args)
        app.config.exhale_args["containmentFolder"] = os.path.realpath(app.config.exhale_args["containmentFolder"])
        print("=" * 75)
        print(project)
        print("-" * 50)
        pprint(app.config.exhale_args)
        print("=" * 75)

        # First, setup the extension and verify all of the configurations.
        exhale.configs.apply_sphinx_configurations(app)
        ####### Next, perform any cleanup

        # Generate the full API!
        try:
            exhale.deploy.explode()
        except:
            exhale.utils.fancyError("Exhale: could not generate reStructuredText documents :/")

    app.config.breathe_default_project = default_project


exhale.environment_ready = exhale_environment_ready

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Portal Framework'
copyright = 'Copyright © 2026, Jonatan Nevo'
author = 'Jonatan Nevo'
release = '1.0.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    # 'breathe',
    # 'exhale',
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.viewcode',
    'sphinx.ext.todo',
    'myst_parser',
    'sphinx_design',
    "sphinx_inline_tabs",
    'sphinx_copybutton',
    'sphinxcontrib.mermaid'
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
    "portal application": "../build/doxygen/application/xml",
    "portal core": "../build/doxygen/core/xml",
    "portal engine": "../build/doxygen/engine/xml",
    "portal input": "../build/doxygen/input/xml",
    "portal networking": "../build/doxygen/networking/xml",
    "portal serialization": "../build/doxygen/serialization/xml",
}

breathe_default_project = "portal engine"
breathe_default_members = tuple()

exhale_args = {
    # These arguments are required
    "rootFileName": "root.rst",
    "doxygenStripFromPath": "../../",

    "rootFileTitle": "Unknown",
    "containmentFolder": "source/api",
    "createTreeView": True,
    "minifyTreeView": False,
    "contentsDirectives": False,
    "exhaleExecutesDoxygen": False,
}

exhale_projects_args = {
    "portal application": {
        "containmentFolder": "source/api/application",
        "rootFileTitle": "Portal Application Api",
    },
    "portal core": {
        "containmentFolder": "source/api/core",
        "rootFileTitle": "Portal Core Api",
    },
    "portal engine": {
        "containmentFolder": "source/api/engine",
        "rootFileTitle": "Portal Engine Api",
    },
    "portal input": {
        "containmentFolder": "source/api/input",
        "rootFileTitle": "Portal Input Api",
    },
    "portal networking": {
        "containmentFolder": "source/api/networking",
        "rootFileTitle": "Portal Networking Api",
    },
    "portal serialization": {
        "containmentFolder": "source/api/serialization",
        "rootFileTitle": "Portal Serialization Api",
    }
}

primary_domain = 'cpp'
highlight_language = 'cpp'

myst_enable_extensions = [
    "colon_fence",  # ::: fence for directives
    "deflist",  # Definition lists
    "tasklist",  # Task lists with [ ] and [x]
    "fieldlist",  # Field lists
]

mermaid_version = ""
mermaid_use_local = "https://example.com"
mermaid_init_js = """import mermaid from 'https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.esm.min.mjs'

const make_config = () => {
  let prefersDark = localStorage.getItem('theme') === 'dark' || (localStorage.getItem('theme') === 'auto' && window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches)
  return({
    startOnLoad:false,
    darkMode: prefersDark,
    theme: prefersDark ? "dark" : "default"
  })
}

const init_mermaid = () => {
    let graphs = document.querySelectorAll(".mermaid");
    [...graphs].forEach((element) => {
        if (!element.hasAttribute("data-source")) {
            element.setAttribute("data-source", element.innerText);
        }
        if (element.hasAttribute("data-processed")) {
            let new_elt = document.createElement("pre");
            let graph_source = element.getAttribute("data-source");
            new_elt.appendChild(document.createTextNode(graph_source));
            new_elt.classList.add("mermaid");
            new_elt.setAttribute("data-source", graph_source);
            element.replaceWith(new_elt);
        }
    });

    let config = make_config()
    mermaid.initialize(config);
    mermaid.run();
}

init_mermaid();

let theme_observer = new MutationObserver(init_mermaid);
let body = document.getElementsByTagName("body")[0];
theme_observer.observe(body, {attributes: true});
window.theme_observer = theme_observer;"""

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'furo'
html_static_path = ['_static']
html_title = "Portal Framework"
html_favicon = "_static/portal_icon_64x64.png"


html_css_files = [
    "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/fontawesome.min.css",
    "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/solid.min.css",
    "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/brands.min.css",
]

html_theme_options = {
    "source_repository": "https://github.com/JonatanNevo/portal-framework",
    "source_branch": "main",
    "source_directory": "docs/source",
    "light_logo": "portal_banner_light.png",
    "dark_logo": "portal_banner_dark.png",
    "sidebar_hide_name": True,
    "navigation_with_keys": True,
    "footer_icons": [
        {
            "name": "GitHub",
            "url": "https://github.com/JonatanNevo/portal-framework",
            "html": "",
            "class": "fa-brands fa-solid fa-github fa-2x",
        },
    ],
}

# html_theme = 'shibuya'
# html_static_path = ['_static']
# html_title = "Portal Framework"
# html_favicon = "_static/portal_icon_64x64.png"
#
# html_theme_options = {
#     "light_logo": "_static/portal_banner_light_cropped.png",
#     "dark_logo": "_static/portal_banner_dark_cropped.png",
#     "og_image_url": "portal_splash_dark.png",
#     "accent_color": "purple",
#
#     "github_url": "https://github.com/JonatanNevo/portal-framework",
#     "linkedin_url": "https://www.linkedin.com/in/jonatan-nevo-488105217/"
# }

todo_include_todos = True
