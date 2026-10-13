import re

import pytest

import exactextract


def test_version_format():

    assert re.match("[0-9]+[.][0-9]+[.][0-9]+", exactextract.__version__)


def test_version_consistency():

    from importlib.metadata import PackageNotFoundError, version

    try:
        mdversion = version("exactextract")
        if "dev" in mdversion:
            pytest.skip()
    except PackageNotFoundError:
        pytest.skip()

    assert mdversion == exactextract.__version__
