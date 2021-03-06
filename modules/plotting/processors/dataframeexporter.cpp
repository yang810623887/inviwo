/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2016-2018 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include <modules/plotting/processors/dataframeexporter.h>
#include <modules/plotting/datastructures/dataframeutil.h>

#include <inviwo/core/util/filesystem.h>
#include <inviwo/core/util/ostreamjoiner.h>
#include <inviwo/core/io/serialization/serializer.h>

#include <fstream>

namespace inviwo {

namespace plot {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo DataFrameExporter::processorInfo_{
    "org.inviwo.DataFrameExporter",      // Class identifier
    "DataFrame Exporter",                // Display name
    "Data Output",                       // Category
    CodeState::Stable,                   // Code state
    "CPU, DataFrame, Export, CSV, XML",  // Tags
};

const ProcessorInfo DataFrameExporter::getProcessorInfo() const { return processorInfo_; }

DataFrameExporter::DataFrameExporter()
    : Processor()
    , dataFrame_("dataFrame")
    , exportFile_("exportFile", "Export file name", "", "dataframe")
    , exportButton_("snapshot", "Export DataFrame")
    , overwrite_("overwrite", "Overwrite", false)
    , separateVectorTypesIntoColumns_("separateVectorTypesIntoColumns",
                                      "Separate Vector Types Into Columns", true)
    , export_(false) {
    exportFile_.clearNameFilters();
    exportFile_.addNameFilter("CSV (*.csv)");
    exportFile_.addNameFilter("XML (*.xml)");

    addPort(dataFrame_);
    addProperty(exportFile_);
    addProperty(exportButton_);
    addProperty(overwrite_);
    addProperty(separateVectorTypesIntoColumns_);

    exportFile_.setAcceptMode(AcceptMode::Save);
    exportFile_.onChange([&]() {
        separateVectorTypesIntoColumns_.setReadOnly(exportFile_.getSelectedExtension() !=
                                                    exportFile_.getNameFilters()[0]);
    });
    exportButton_.onChange([&]() { export_ = true; });

    setAllPropertiesCurrentStateAsDefault();
}

void DataFrameExporter::process() {
    if (export_) exportNow();
    export_ = false;
}

void DataFrameExporter::exportNow() {
    if (filesystem::fileExists(exportFile_) && !overwrite_.get()) {
        LogWarn("File already exists: " << exportFile_);
        return;
    }
    if (exportFile_.getSelectedExtension() == exportFile_.getNameFilters()[0]) {
        exportAsCSV(separateVectorTypesIntoColumns_);
    } else if (exportFile_.getSelectedExtension() == exportFile_.getNameFilters()[1]) {
        exportAsXML();
    } else {
        LogWarn("Unsupported file type: " << exportFile_);
    }
}

void DataFrameExporter::exportAsCSV(bool separateVectorTypesIntoColumns) {
    std::ofstream file(exportFile_);
    auto dataFrame = dataFrame_.getData();

    const std::string delimiter = ", ";
    const char lineterminator = '\n';
    const std::array<char, 4> componentNames = {'X', 'Y', 'Z', 'W'};

    // headers
    auto oj = util::make_ostream_joiner(file, delimiter);
    for (const auto& col : *dataFrame) {
        const auto components = col->getBuffer()->getDataFormat()->getComponents();
        if (components > 1 && separateVectorTypesIntoColumns) {
            for (size_t k = 0; k < components; k++) {
                oj = col->getHeader() + ' ' + componentNames[k];
            }
        } else {
            oj = col->getHeader();
        }
    }
    file << lineterminator;

    std::vector<std::function<void(std::ostream&, size_t)>> printers;
    for (const auto& col : *dataFrame) {
        auto df = col->getBuffer()->getDataFormat();
        if (auto cc = dynamic_cast<const CategoricalColumn*>(col.get())) {
            printers.push_back([cc](std::ostream& os, size_t index) {
                os << "\"" << cc->getAsString(index) << "\"";
            });
        } else if (df->getComponents() == 1) {
            col->getBuffer()
                ->getRepresentation<BufferRAM>()
                ->dispatch<void, dispatching::filter::Scalars>([&printers](auto br) {
                    printers.push_back([br](std::ostream& os, size_t index) {
                        os << br->getDataContainer()[index];
                    });
                });
        } else if (df->getComponents() > 1 && separateVectorTypesIntoColumns) {
            col->getBuffer()
                ->getRepresentation<BufferRAM>()
                ->dispatch<void, dispatching::filter::Vecs>([&printers, delimiter](auto br) {
                    using ValueType = util::PrecsionValueType<decltype(br)>;
                    printers.push_back([br, delimiter](std::ostream& os, size_t index) {
                        auto oj = util::make_ostream_joiner(os, delimiter);
                        for (size_t i = 0; i < util::flat_extent<ValueType>::value; ++i) {
                            oj = br->getDataContainer()[index][i];
                        }
                    });
                });
        } else {
            col->getBuffer()
                ->getRepresentation<BufferRAM>()
                ->dispatch<void, dispatching::filter::Vecs>([&printers](auto br) {
                    printers.push_back([br](std::ostream& os, size_t index) {
                        os << "\"" << br->getDataContainer()[index] << "\"";
                    });
                });
        }
    }

    for (size_t j = 0; j < dataFrame->getNumberOfRows(); j++) {
        if (j != 0) {
            file << lineterminator;
        }
        bool firstCol = true;
        for (auto& printer : printers) {
            if (!firstCol) {
                file << delimiter;
            }
            firstCol = false;
            printer(file, j);
        }
    }

    LogInfo("CSV file exported to " << exportFile_);
}

void DataFrameExporter::exportAsXML() {
    auto dataFrame = dataFrame_.getData();

    std::ofstream file(exportFile_);
    Serializer serializer("");

    for (const auto& col : *dataFrame) {
        col->getBuffer()->getRepresentation<BufferRAM>()->dispatch<void>([&](auto br) {
            serializer.serialize(col->getHeader(), br->getDataContainer(), "Item");
        });
    }

    serializer.writeFile(file);
    LogInfo("XML file exported to " << exportFile_);
}

}  // namespace plot

}  // namespace inviwo
