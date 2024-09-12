#
# Configuration file for the Sphinx documentation builder.
#

import datetime
import re

# -- Project information -----------------------------------------------------

project = "exactextract"
copyright = f"2018-{datetime.date.today().year}, ISciences LLC"
title = "exactextract"
author = "Dan Baston"
description = ""

# -- General configuration ---------------------------------------------------

source_suffix = ".rst"
exclude_patterns = []

master_doc = "index"

extensions = [
    "sphinx.ext.autodoc",
    "autoapi.extension",
    "sphinx.ext.coverage",
    "sphinx.ext.viewcode",
    "sphinx.ext.githubpages",
    "sphinx.ext.napoleon",
    "nbsphinx",
]

pygments_style = None

# -- Options for HTML output -------------------------------------------------

html_theme = "sphinx_rtd_theme"

# html_theme_options = {}
# html_static_path = ['_static']
# html_sidebars = {}

# -- Options for HTMLHelp output ---------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = "exactextractdoc"

# -- Napoleon configuration --------------------------------------------------

napoleon_google_docstring = True
napoleon_numpy_docstring = False
napoleon_include_init_with_doc = True
napoleon_include_private_with_doc = False
napoleon_include_special_with_doc = True
napoleon_use_admonition_for_examples = False
napoleon_use_admonition_for_notes = False
napoleon_use_admonition_for_references = False
napoleon_use_ivar = False
napoleon_use_param = True
napoleon_use_rtype = True
napoleon_preprocess_types = False
napoleon_type_aliases = None
napoleon_attr_annotations = True

# -- AutoAPI configuration ----------------------------------------------------

autoapi_dirs = ["../src/exactextract"]
autoapi_keep_files = False
autoapi_add_toctree_entry = False
autoapi_python_class_content = "both"  # include both class and __init__ docstrings

autoapi_options = ["members", "undoc-members", "show-module-summary", "special-members"]


def autoapi_skip_member(app, what, name, obj, skip, options):
    shortname = name.split(".")[-1]

    # Don't emit documentation for anything named beginning with
    # an underscore, except for __init__
    if re.match("^_[a-z]", shortname):
        return True

    # Don't emit documentation for member variables, which are
    # assumed to be private even if not prefixed with an
    # underscore.
    if what == "attribute":
        return True

    return None  # Fallback to default behavior


def setup(sphinx):
    sphinx.connect("autoapi-skip-member", autoapi_skip_member)
