#pragma once

#include <cstdint>

namespace volap::execution 
{

enum class ProjectionType : std::uint8_t
{
    ColumnRef,
    Multiply
};

class Projection
{
public:

    static Projection column(std::size_t column_index) noexcept
    {
        return Projection(ProjectionType::ColumnRef, column_index, 0);
    }

    static Projection multiply(std::size_t left_column_index,
                               std::size_t right_column_index) noexcept
    {
        return Projection(ProjectionType::Multiply, 
                          left_column_index, 
                          right_column_index);
    }

    ProjectionType type() const noexcept
    {
        return type_;
    }

    std::size_t left_column_index() const noexcept
    {
        return left_column_index_;
    }

    std::size_t right_column_index() const noexcept
    {
        return right_column_index_;
    }

private:


    Projection(ProjectionType type,
               std::size_t left_column_index,
               std::size_t right_column_index) noexcept
        : type_(type),
          left_column_index_(left_column_index),
          right_column_index_(right_column_index)
    {}

    ProjectionType type_;
    std::size_t left_column_index_;
    std::size_t right_column_index_;

};

} // volap::execution
