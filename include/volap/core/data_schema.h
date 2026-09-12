#pragma once

#include "volap/core/type.h"
#include "volap/core/data_chunk.h"

#include <cstddef>
#include <initializer_list>
#include <vector>

namespace volap::core {

// To avoid repeated type checking during data pipeline we should
// keep track of the data schema
class DataSchema final
{
public:
    explicit DataSchema(std::vector<DataType> column_types);

    DataSchema(std::initializer_list<DataType> column_types);

    static DataSchema from_chunk(const DataChunk &chunk);

    std::size_t column_count() const noexcept;

    DataType column_type(std::size_t column_index) const;

private:
    std::vector<DataType> column_types_;
};

} // namespace volap::core
