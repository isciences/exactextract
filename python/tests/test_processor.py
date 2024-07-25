import pytest

from exactextract import Operation
from exactextract.feature import JSONFeatureSource
from exactextract.processor import FeatureSequentialProcessor, RasterSequentialProcessor
from exactextract.writer import JSONWriter


@pytest.mark.parametrize(
    "Processor", (FeatureSequentialProcessor, RasterSequentialProcessor)
)
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


def test_progress(np_raster_source, square_features):
    ops = [
        Operation("count", "count", np_raster_source),
    ]
    writer = JSONWriter()

    fs = JSONFeatureSource(square_features)

    fracs = []

    def status(frac, message):
        fracs.append(frac)

    processor = FeatureSequentialProcessor(fs, writer, ops)
    processor.set_progress_fn(status)
    processor.show_progress(True)
    processor.process()

    assert len(fracs) == len(square_features)
    assert fracs == sorted(fracs)
    assert fracs[-1] == 1.0
