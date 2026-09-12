
#include "volap/core/data_schema.h"
#include "volap/core/data_chunk.h"

#include <utility>
#include <vector>

namespace volap::core 
{

DataSchema::DataSchema(std::vector<DataType> column_types)
    : column_types_(std::move(column_types))
{}

DataSchema::DataSchema(std::initializer_list<DataType> column_types)
    : column_types_(column_types)
{}

DataSchema DataSchema::from_chunk(const DataChunk &chunk)
{
    std::vector<DataType> column_types;
    column_types.reserve(chunk.column_count());

    for (std::size_t i = 0; i < chunk.column_count(); ++i) {
        column_types.push_back(chunk.column(i).data_type());
    }

    return DataSchema(std::move(column_types));
}

std::size_t DataSchema::column_count() const noexcept
{
    return column_types_.size();
}

DataType DataSchema::column_type(std::size_t column_index) const
{
    return column_types_.at(column_index);
}

} // namespace volap::core
