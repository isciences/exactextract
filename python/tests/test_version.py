import re

import exactextract
import pytest


def test_version_format():
    assert re.match("[0-9]+[.][0-9]+[.][0-9]+", exactextract.__version__)


def test_version_consistency():
    from importlib.metadata import PackageNotFoundError, version

    try:
        mdversion = version("exactextract")
        if mdversion.endswith("dev0"):
            pytest.skip()
    except PackageNotFoundError:
        pytest.skip()

    assert mdversion == exactextract.__version__
