/*
 *  CppNAM -- C++ Neural Associative Memory Simulator
 *  Copyright (C) 2016  Christoph Jenzen, Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtest/gtest.h"

#include <cstdint>
#include <limits>

#include <cppnam/binam/ncr.hpp>

namespace nam {

TEST(ncr, ncr)
{
	EXPECT_EQ(0, ncr(0, -1));
	EXPECT_EQ(10, ncr(5, 2));
}

TEST(ncr, ncr_clamped32)
{
	EXPECT_EQ(0, ncr_clamped32(0, -1));
	EXPECT_EQ(10, ncr_clamped32(5, 2));
	EXPECT_EQ(49995000, ncr_clamped32(10000, 2));
	EXPECT_EQ(std::numeric_limits<uint32_t>::max(), ncr_clamped32(10000, 3));
}

TEST(ncr, ncr_clamped64)
{
	EXPECT_EQ(0, ncr_clamped64(0, -1));
	EXPECT_EQ(10, ncr_clamped64(5, 2));
	EXPECT_EQ(49995000, ncr_clamped64(10000, 2));
	EXPECT_EQ(166616670001, ncr_clamped64(10000, 3));
	EXPECT_EQ(std::numeric_limits<uint64_t>::max(), ncr_clamped64(10000, 6));
}

TEST(ncr, lnncrr)
{
	EXPECT_NEAR(2.302585093, lnncrr(5, 2), 1e-6);
	EXPECT_NEAR(17.727433558, lnncrr(10000, 2), 1e-6);
	EXPECT_NEAR(25.838961622, lnncrr(10000, 3), 1e-6);
}
}