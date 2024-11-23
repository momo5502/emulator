#pragma once
#include "../io_device.hpp"

std::unique_ptr<io_device<EmulatorTraits<Emu64>>> create_afd_endpoint64();
