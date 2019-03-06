//
// Created by dbaston on 3/6/19.
//

#ifndef EXACTEXTRACT_GDAL_WRITER_H
#define EXACTEXTRACT_GDAL_WRITER_H

#include "output_writer.h"

#include "gdal.h"
#include "ogr_api.h"

#include "stats_registry.h"

#include "utils.h"


namespace exactextract {

    class GDALWriter : public OutputWriter {

    public:
        GDALWriter(const std::string & filename, const std::string & id_field) :
        m_layer{nullptr},
        m_dataset{nullptr} {
            std::string driver_name;

            if (ends_with(filename, ".csv")) {
                driver_name = "CSV";
            } else if (ends_with(filename, ".dbf")) {
                driver_name = "ESRI Shapefile";
            } else if (ends_with(filename, ".nc")) {
                driver_name = "NetCDF";
            } else if (starts_with(filename, "PG:")) {
                driver_name = "PostgreSQL";
            } else {
                throw std::runtime_error("Unknown output format: " + filename);
            }

            auto driver = GDALGetDriverByName(driver_name.c_str());

            if (driver == nullptr) {
                throw std::runtime_error("Could not load output driver: " + driver_name);
            }

            m_dataset = GDALCreate(driver, filename.c_str(), 0, 0, 0, GDT_Unknown, nullptr);

            m_layer = GDALDatasetCreateLayer(m_dataset, "output", nullptr, wkbNone, nullptr);

            auto id_def = OGR_Fld_Create(id_field.c_str(), OFTString);

            OGR_Fld_SetWidth(id_def, 32); // FIXME

            OGR_L_CreateField(m_layer, id_def, true);
        }

        ~GDALWriter() {
            if (m_dataset != nullptr) {
                GDALClose(m_dataset);
            }
        }

        void add_operation(const Operation & op) override {
            auto field_name = varname(op);

            // TODO set type here?
            auto def = OGR_Fld_Create(field_name.c_str(), OFTReal);
            OGR_L_CreateField(m_layer, def, true);

            m_ops.push_back(&op);
        }

        void set_registry(const StatsRegistry* reg) override {
            m_reg = reg;
        }

        void write(const std::string & fid) override {
            auto feature = OGR_F_Create(OGR_L_GetLayerDefn(m_layer));

            OGR_F_SetFieldString(feature, 0, fid.c_str());

            for (const auto &op : m_ops) {
                if (m_reg->contains(fid, *op)) {
                    const auto field_pos = OGR_F_GetFieldIndex(feature, varname(*op).c_str());
                    const auto &stats = m_reg->stats(fid, *op);

                    if (op->stat == "mean") {
                        OGR_F_SetFieldDouble(feature, field_pos, stats.mean());
                    } else if (op->stat == "sum") {
                        OGR_F_SetFieldDouble(feature, field_pos, stats.sum());
                    } else {
                        // FIXME
                        OGR_F_Destroy(feature);
                        throw std::runtime_error("Not implemented.");
                    }
                }
            }

            // FIXME check return vals
            OGR_L_CreateFeature(m_layer, feature);
            OGR_F_Destroy(feature);
        }

    private:
        GDALDatasetH m_dataset;
        OGRLayerH m_layer;
        const StatsRegistry* m_reg;
    };

}

#endif //EXACTEXTRACT_GDAL_WRITER_H
