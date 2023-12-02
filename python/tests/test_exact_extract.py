import math
import pytest
import numpy as np

from exactextract import *


def make_square_raster(n):
    values = np.arange(1, n * n + 1).reshape(n, n)
    return NumPyRasterSource(values, 0, 0, n, n)


def make_rect(xmin, ymin, xmax, ymax, fid=1):
    return {
        "id": fid,
        "geometry": {
            "type": "Polygon",
            "coordinates": [
                [[xmin, ymin], [xmax, ymin], [xmax, ymax], [xmin, ymax], [xmin, ymin]]
            ],
        },
    }


@pytest.mark.parametrize(
    "stat,expected",
    [
        ("count", 4),
        ("mean", 5),
        ("median", 5),
        ("min", 1),
        ("max", 9),
        ("mode", 5),
        ("majority", 5),
        ("minority", 1),
        # TODO: add quantiles
        ("variety", 9),
        ("variance", 5),
        ("stdev", math.sqrt(5)),
        ("coefficient_of_variation", math.sqrt(5)/5)
    ],
)
def test_exact_extract_stats(stat, expected):
    rast = make_square_raster(3)
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5), "id")

    assert exact_extract(rast, square, stat)[0]["properties"][stat] == pytest.approx(
        expected
    )


def test_exact_extract_multiple_stats():
    rast = make_square_raster(3)
    square = JSONFeatureSource(make_rect(0.5, 0.5, 2.5, 2.5), "id")

    assert exact_extract(rast, square, ("min", "max", "mean"))[0]["properties"] == {
        "min": 1,
        "max": 9,
        "mean": 5,
    }
