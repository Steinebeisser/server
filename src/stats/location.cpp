#include "location.hpp"
#include "generated/subdivisions.hpp"
#include <cstdint>


int16_t LocationMapper::get_subdivision_id_by_code(const std::string_view& code) {
    size_t l = 0;
    size_t r = mappings_by_code.size() - 1;

    while (l <= r) {
        size_t m = l + (r - l) / 2;
        if (mappings_by_code[m].code == code) {
            return mappings_by_code[m].id;
        }

        if (mappings_by_code[m].code < code) {
            l = m + 1;
        }

        else {
            r = m - 1;
        }
    }

    return -1;
}

int16_t LocationMapper::get_subdivision_id_by_name(const std::string_view& name) {
    size_t l = 0;
    size_t r = mappings_by_name.size() - 1;

    while (l <= r) {
        size_t m = l + (r - l) / 2;
        if (mappings_by_name[m].name == name) {
            return mappings_by_name[m].id;
        }

        if (mappings_by_name[m].name < name) {
            l = m + 1;
        }

        else {
            r = m - 1;
        }
    }

    return -1;
}

const char* LocationMapper::get_subdivision_code_by_name(const std::string_view& name) {
    size_t l = 0;
    size_t r = mappings_by_name.size() - 1;

    while (l <= r) {
        size_t m = l + (r - l) / 2;
        if (name_to_code_mappings[m].name == name) {
            return name_to_code_mappings[m].code.data();
        }

        if (name_to_code_mappings[m].name < name) {
            l = m + 1;
        }

        else {
            r = m - 1;
        }
    }

    return "";
}

int16_t LocationMapper::get_country_id_by_code(const std::string_view& code) {
    size_t l = 0;
    size_t r = country_id_mappings_by_code.size() - 1;

    while (l <= r) {
        size_t m = l + (r - l) / 2;
        if (country_id_mappings_by_code[m].code == code) {
            return country_id_mappings_by_code[m].id;
        }

        if (country_id_mappings_by_code[m].code < code) {
            l = m + 1;
        }

        else {
            r = m - 1;
        }
    }

    return -1;
}

const char* LocationMapper::get_subdivision_code(int16_t id) {
    if (id >= 0 && id <= SUBDIVISION_COUNT)
        return code_mappings_by_id[static_cast<size_t>(id)];
    return "";
}

const char* LocationMapper::get_subdivision_name(int16_t id) {
    if (id >= 0 && id <= SUBDIVISION_COUNT)
        return name_mappings_by_id[static_cast<size_t>(id)];
    return "";
}


