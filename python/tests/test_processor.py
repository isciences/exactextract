#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pytest

from exactextract import *


@pytest.mark.parametrize("Processor", (FeatureSequentialProcessor,))
def test_process(Processor, np_raster_source, square_features):
    ops = [
        Operation("count", "test", np_raster_source),
        Operation("sum", "my_test", np_raster_source),
    ]
    writer = JSONWriter()

    fs = JSONFeatureSource(square_features)

    processor = Processor(fs, writer, ops)
    processor.add_col("id")
    processor.process()

    results = writer.features()

    assert len(results) == len(square_features)

    f1 = results[0]
    assert f1["properties"].keys() == {"my_test", "test"}
    assert f1["id"] == square_features[0]["id"]
