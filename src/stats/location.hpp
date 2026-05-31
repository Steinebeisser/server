#pragma once

#include <array>
#include <cstdint>
#include "generated/subdivisions.hpp"


class LocationMapper {
public:
    static int16_t get_subdivision_id_by_code(const std::string_view& code);
    static int16_t get_subdivision_id_by_name(const std::string_view& name);

    static const char* get_subdivision_code(int16_t id);
    static const char* get_subdivision_name(int16_t id);
    static const char* get_subdivision_code_by_name(const std::string_view& name);


    static int16_t get_country_id_by_code(const std::string_view& code);

    LocationMapper() = delete;

private:
    static std::array<SubdivisionEntry, SUBDIVISION_COUNT> cache_;
    static size_t cache_count_;
};
