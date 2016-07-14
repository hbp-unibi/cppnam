/*
 *  CppNAM -- C++ Neural Associative Memory Simulator
 *  Copyright (C) 2016  Andreas Stöckel
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

#pragma once

#ifndef CPPNAM_UTIL_DATA_HPP
#define CPPNAM_UTIL_DATA_HPP

#include <cstdint>
#include <functional>
#include <random>

#include <cppnam/binam/binary_matrix.hpp>

namespace nam {
/**
 * The DataGenerator class allows to generate random data vectors for storage in
 * the associative memories. The user can select between multiple data
 * generation methods by setting the corresponding flags in the constructor or
 * via the designated setters.
 */
class DataGenerator {
public:
	using ProgressCallback = std::function<bool(float)>;

private:
	size_t m_seed;
	bool m_random;
	bool m_balance;
	bool m_unique;

	template <typename RandomEngine>
	BinaryMatrix generate_random(RandomEngine &re, size_t n_bits, size_t n_ones,
	                             size_t n_samples,
	                             const ProgressCallback &progress);

	template <typename RandomEngine>
	BinaryMatrix generate_balanced(RandomEngine &re, uint32_t n_bits,
	                               uint32_t n_ones, uint32_t n_samples,
	                               bool random, bool balance, bool unique,
	                               const ProgressCallback &progress);

public:
	/**
	 * Constructor of the DataGenerator class. Note that after construction,
	 * a DataGenerator instance will always produce exactly the same output.
	 *
	 * @param random if false, the data is not generated randomly but according
	 * to a fixed pattern.
	 * @param balanced if true, bit balancing is performed.
	 * @param unique if true, the generated bit vectors are unique.
	 * @param seed is the random seed used for the data generation.
	 */
	DataGenerator(bool random = true, bool balance = true, bool unique = true)
	    : m_seed(std::random_device()()),
	      m_random(random),
	      m_balance(balance),
	      m_unique(unique)
	{
	}

	/**
	 * Generates sparse matrix containing n_samples rows of data vectors of
	 * length n_bits with n_ones entries set to one.
	 *
	 * @param n_bits is the length of each individual data vector.
	 * @param n_ones is the number of bits set to one in each vector.
	 * @param n_samples is the number of vectors that should be generatred.
	 * @return a sparse matrix containing the generated data.
	 */
	BinaryMatrix generate(uint32_t n_bits, uint32_t n_ones, uint32_t n_samples,
	                      const ProgressCallback &progress = [](float) {
		                      return true;
		                  });

	/**
	 * Setter of the "random" flag.
	 *
	 * @param random if false, the data is not generated randomly but according
	 * to a fixed pattern.
	 * @return a reference at this instance of the DataGenerator to allow for
	 * chaining of setters.
	 */
	DataGenerator &random(bool random)
	{
		m_random = random;
		return *this;
	}

	/**
	 * Getter of the "random" flag.
	 *
	 * @return the current state of the "random" flag.
	 */
	bool random() const { return m_random; }

	/**
	 * Setter of the "balance" flag.
	 *
	 * @param balanced if true, bit balancing is performed.
	 * @return a reference at this instance of the DataGenerator to allow for
	 * chaining of setters.
	 */
	DataGenerator &balance(bool balance)
	{
		m_balance = balance;
		return *this;
	}

	/**
	 * Getter of the "balance" flag.
	 *
	 * @return the current state of the "balance" flag.
	 */
	bool balance() const { return m_balance; }


	/**
	 * Setter of the "unique" flag.
	 *
	 * @param unique if true, the generated bit vectors are unique.
	 * @return a reference at this instance of the DataGenerator to allow for
	 * chaining of setters.
	 */
	DataGenerator &unique(bool unique)
	{
		m_unique = unique;
		return *this;
	}

	/**
	 * Getter of the "unique" flag.
	 *
	 * @return the current state of the "unique" flag.
	 */
	bool unique() const { return m_unique; }

	/**
	 * Setter of seed. Overrides the randomly selected seed chosen in the
	 * constructor.
	 *
	 * @param seed the seed that should be used to initialize the random number
	 * generator.
	 * @return a reference at this instance of the DataGenerator to allow for
	 * chaining of setters.
	 */
	DataGenerator &seed(size_t seed)
	{
		m_seed = seed;
		return *this;
	}

	/**
	 * Getter of the data generator seed.
	 *
	 * @return the seed that is used to initialize the random number generator.
	 */
	size_t seed() const { return m_unique; }
};
}

#endif /* CPPNAM_UTIL_DATA_HPP */