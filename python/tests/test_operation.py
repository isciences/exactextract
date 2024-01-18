#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pytest

from exactextract import Operation


def test_valid_stat(np_raster_source):
    valid_stats = (
        "count",
        "sum",
        "mean",
        "min",
        "max",
        "minority",
        "majority",
        "variety",
        "variance",
        "stdev",
        "coefficient_of_variation",
    )
    for stat in valid_stats:
        op = Operation(stat, "test", np_raster_source)
        assert op.stat == stat


def test_field_name(np_raster_source):
    op = Operation("count", "any_field_name", np_raster_source)
    assert op.name == "any_field_name"


def test_valid_raster(np_raster_source):
    op = Operation("count", "test", np_raster_source)
    assert op.values == np_raster_source


@pytest.mark.parametrize("raster", ("invalid", None))
def test_invalid_raster(raster):
    with pytest.raises(TypeError):
        Operation("count", "test", raster)  # type: ignore


def test_valid_weights(np_raster_source):
    rs1 = np_raster_source
    rs2 = np_raster_source

    op = Operation("count", "test", rs1, None)
    assert op.values == rs2
    assert op.weights is None

    op = Operation("weighted_mean", "test", rs1, rs2)
    assert op.values == rs1
    assert op.weights == rs2


def test_invalid_weights(np_raster_source):
    with pytest.raises(TypeError):
        Operation("count", "test", np_raster_source, "invalid")  # type: ignore


def test_stat_arguments(np_raster_source):

    op = Operation("quantile", "test", np_raster_source, None, {"q": 0.333})
    assert op.values == np_raster_source
