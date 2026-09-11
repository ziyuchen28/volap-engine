#pragma once

#include "volap/core/type.h"
#include "volap/core/data_chunk.h"

#include <cstddef>
#include <initializer_list>
#include <vector>

namespace volap::core {

class DataSchema final
{
public:
    explicit DataSchema(std::vector<Type> column_types);

    DataSchema(std::initializer_list<Type> column_types);

    static DataSchema from_chunk(const DataChunk &chunk);

    std::size_t column_count() const noexcept;

    Type column_type(std::size_t column_index) const;

private:
    std::vector<Type> column_types_;
};

} // namespace volap::core
