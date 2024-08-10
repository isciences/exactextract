# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#

import datetime

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


def autoapi_skip_member(app, what, name, obj, skip, options):
    if name.startswith("exactextract.processor"):
        return True
    if name.startswith("exactextract.feature."):
        return True
    if name.startswith("_"):
        return name != "__init__"

    return False


def setup(sphinx):
    sphinx.connect("autoapi-skip-member", autoapi_skip_member)
