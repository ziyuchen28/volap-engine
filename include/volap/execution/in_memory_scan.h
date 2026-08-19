
#pragma once

#include "volap/core/data_chunk.h"

namespace volap::execution 
{

using namespace volap::core;

class InMemoryScan final {
public:
    InMemoryScan(DataChunk source, std::size_t chunk_size);

    bool next(DataChunk& output);

    std::size_t position() const noexcept
    {
        return position_;
    }

    void reset() noexcept
    {
        position_ = 0;
    }

private:
    void prepare_output(DataChunk &output) const;

    DataChunk source_;
    std::size_t chunk_size_;
    std::size_t position_ = 0;
};

} // volap::execution
