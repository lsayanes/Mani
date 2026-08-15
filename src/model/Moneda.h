#pragma once

#include <cstdint>

enum class Moneda : std::uint8_t
{
    USD = 1,
    ARS = 2
};

inline const char *monedaLabel(Moneda moneda)
{
    switch (moneda) {
    case Moneda::USD:
        return "USD";
    case Moneda::ARS:
        return "ARS";
    }
    return "?";
}
