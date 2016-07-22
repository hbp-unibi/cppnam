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

#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include <cypress/cypress.hpp>
#include "experiment.hpp"
#include "spiking_binam.hpp"
#include "util/read_json.hpp"

namespace nam {
namespace {
/**
 * Splits a string @param s into parts devided by @param delim and stores the
 * result in @param elems and returns it
 */
std::vector<std::string> &split(const std::string &s, char delim,
                                std::vector<std::string> &elems)
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

/**
 * The same as above, but only returning the vector.
 */
std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

/**
 * Adds a sweep parameter to already existing structures.
 * @param key : string containing the name of the parameter
 * @param values : Vector of values which should be swept over
 * @param sweep_params : Vector of names of sweep parameters. @key will be
 * appended
 * @param sweep_elems : two dimensional vector containing all sweep values: The
 * first dimension is the run index, while the second addresses the sweep
 * parameters
 */
static void add_sweep_parameter(const std::string &key,
                                const std::vector<float> &values,
                                std::vector<std::string> &sweep_params,
                                std::vector<std::vector<float>> &sweep_elems,
                                size_t repeat = 1)
{
	// Add the sweep key
	sweep_params.insert(sweep_params.begin(), key);

	// Copy the old sweep elements
	const std::vector<std::vector<float>> old_sweep_elems = sweep_elems;

	// Fetch some constants
	const size_t n_elems_old = old_sweep_elems.size();
	const size_t n_elems = n_elems_old * values.size();
	const size_t n_dim = sweep_params.size();

	// Special case if n_elems_old is zero, repeat values
	if (n_elems_old == 0) {
		sweep_elems.resize(repeat * values.size());
		std::fill(sweep_elems.begin(), sweep_elems.end(),
		          std::vector<float>(n_dim));
		for (size_t i = 0; i < values.size(); i++) {
			for (size_t j = 0; j < repeat; j++) {
				sweep_elems[i * repeat + j][0] = values[i];
			}
		}
	}
	else {
		// Resize the elements matrix
		sweep_elems.resize(n_elems);
		std::fill(sweep_elems.begin(), sweep_elems.end(),
		          std::vector<float>(n_dim));

		for (size_t i = 0; i < values.size(); i++) {
			// Copy old values, leave first entry empty
			for (size_t k = 0; k < n_elems_old; k++) {
				std::copy(old_sweep_elems[k].begin(), old_sweep_elems[k].end(),
				          sweep_elems[i * n_elems_old + k].begin() + 1);
			}
			// Now put in new values at the beginning of eacht entry
			for (size_t k = 0; k < n_elems_old; k++) {
				sweep_elems[i * n_elems_old + k][0] = values[i];
			}
		}
	}
}

/**
 * Sets a parameter in the given @param binam, while @param names should contain
 * the entries 'name of structure' and 'parameter name'. The appropriate
 * value willl be set to @param value
 */
void set_parameter(SpikingBinam &binam, std::vector<std::string> names,
                   float value)
{
	if (names[0] == "params") {
		auto params = binam.NeuronParams();
		binam.NeuronParams(params.set(names[1], value));
	}
	else if (names[0] == "network") {
		auto params = binam.NetParams();
		binam.NetParams(params.set(names[1], value));
	}
	else {
		throw std::invalid_argument("Unknown parameter \"" + names[0] + "\"");
	}
}

/**
 * This function does a normal run when no parameter is swept over. Booleans can
 * be set to vary the form of the output
 */
void run_standard_neat_output(SpikingBinam &SpBinam, std::ostream &ofs,
                              std::string backend, bool print_params = false,
                              bool neat = true, bool times = true)
{
	using namespace std::chrono;
	system_clock::time_point t1, t2, t3, t4, t5, t6;
	auto time = std::time(NULL);

	if (print_params) {
		ofs << "#"
		    << " ________________________________________________________"
		    << std::endl
		    << "# "
		    << "Spiking Binam from " << std::ctime(&time) << std::endl
		    << "# Simulator : " << backend << std::endl;
		ofs << std::endl;
		auto params = SpBinam.DataParams();
		params.print(ofs);
		ofs << std::endl;
		auto params2 = SpBinam.NetParams();
		params2.print(ofs);
		ofs << std::endl;
		auto params3 = SpBinam.NeuronParams();
		params3.print(ofs);
		ofs << std::endl;
	}
	cypress::Network netw;
	t1 = system_clock::now();
	SpBinam.build(netw);
	t2 = system_clock::now();
	std::thread spiking_network([&netw, backend, &t3, &t4]() mutable {
		t3 = system_clock::now();
		netw.run(cypress::PyNN(backend));
		t4 = system_clock::now();
	});
	std::thread recall([&SpBinam, &t5, &t6]() mutable {
		t5 = system_clock::now();
		SpBinam.recall();
		t6 = system_clock::now();
	});
	recall.join();
	spiking_network.join();
	auto runtime = netw.runtime();
	if (neat) {
		SpBinam.evaluate_neat(ofs);
	}
	else {
		if (print_params) {
			ofs << "info, info_th,info_n, fp, fp_th, fn, fn_th" << std::endl;
		}
		SpBinam.evaluate_csv(ofs);
		ofs << std::endl;
	}
	if (times) {
		ofs << std::endl << "Time in milliseconds:" << std::endl;
		auto time_span = duration_cast<milliseconds>(t2 - t1);
		ofs << "Building spiking neural network took:\t" << time_span.count()
		    << std::endl;
		ofs << "Building in PyNN took:\t\t\t\t" << runtime.initialize * 1e3
		    << std::endl;
		time_span = duration_cast<milliseconds>(t4 - t3);
		ofs << "Cypress run took:\t\t\t\t\t" << time_span.count() << std::endl;
		ofs << "Simulation took:\t\t\t\t\t" << runtime.sim * 1e3 << std::endl;
		time_span = duration_cast<milliseconds>(t6 - t5);
		ofs << "Classical recall took:\t\t\t\t\t" << time_span.count()
		    << std::endl;
	}
}

/**
 * Perepares data parameters if it was manually set with a single value in the
 * JSON object
 */
DataParameters prepare_data_params(
    cypress::Json json, std::vector<std::vector<std::string>> &params_names,
    std::vector<size_t> &params_indices,
    std::vector<std::pair<std::string, float>> &parameters)
{
	DataParameters params(json["data"]);
	for (size_t k = 0; k < parameters.size(); k++) {
		params_names.emplace_back(split(parameters[k].first, '.'));
		if (params_names[k][0] == "data") {
			params.set(params_names[k][1], parameters[k].second);
		}
		else {
			params_indices.emplace_back(k);
		}
	}
	return params;
}

/**
 * Hard coded numbers of maximal neuron count for every plattform. Should be
 * improved -> TODO
 */
static const std::map<std::string, size_t> neuron_numbers{
    {"spikey", 0}, {"spinnaker", 1000}, {"nmmc1", 1e5}, {"nest", 1e3}};

/**
 * Checks, wether an additional parallel run will be to big, and if that is the
 * case, perform the simulation now
 * @param sp_binam_vec: vector of spiking_binam
 * @param sweep_values for this experiment
 * @param netw: the currently build network. Will be resetted after simulation
 * @param backend: simulation platform
 * @param results: vector containing all results for this experiment
 * @param next_neuron_count: number of output neurons needed in the next run
 */
void check_run(std::vector<SpikingBinam> &sp_binam_vec,
               const std::vector<std::vector<float>> &sweep_values,
               cypress::Network &netw, size_t j, std::vector<size_t> &counter,
               const std::string &backend,
               std::vector<std::pair<ExpResults, ExpResults>> &results,
               size_t next_neuron_count)
{
	size_t max_neurons = neuron_numbers.find(backend)->second;
	// Check wether the next run is too big or if we are in the last run of the
	// experiment
	if (netw.neuron_count() + next_neuron_count > max_neurons ||
	    j == sweep_values.size() - 1) {

		netw.run(cypress::PyNN(backend));

		// Generate results
		for (size_t k = 0; k < sp_binam_vec.size(); k++) {
			results[counter[k]] = sp_binam_vec[k].evaluate_res();
		}

		// Reset variables
		// counter.erase(counter.begin(), counter.end());
		counter = std::vector<size_t>();
		sp_binam_vec.erase(sp_binam_vec.begin(), sp_binam_vec.end());
		netw = cypress::Network();
		std::cout << size_t(100 * float(j + 1) / sweep_values.size())
		          << "% done" << std::endl;
	}
}

/**
 * Prints out the results, sweep version
 */
void output(const std::vector<std::vector<float>> &sweep_values,
            const std::vector<std::pair<ExpResults, ExpResults>> &results,
            std::ostream &ofs, std::vector<std::string> &names)
{
	for (size_t j = 0; j < results.size(); j++) {              // all values
		for (size_t k = 0; k < sweep_values[j].size(); k++) {  // all parameter
			if (names[k] == "data") {
				ofs << size_t(sweep_values[j][k]) << ",";
			}
			else {
				ofs << sweep_values[j][k] << ",";
			}
		}
		ofs << results[j].second.Info << "," << results[j].first.Info << ","
		    << results[j].second.Info / results[j].first.Info << ","
		    << results[j].second.fp << "," << results[j].first.fp << ","
		    << results[j].second.fn << "," << results[j].first.fn;
		ofs << std::endl;
	}
}
}

Experiment::Experiment(cypress::Json &json, std::string backend)
    : m_backend(backend), json(json)
{
	if (json.find("experiments") == json.end()) {
		standard = true;
	}
	else {
		std::vector<std::string> names = {"min", "max", "count"};
		for (auto i = json["experiments"].begin();
		     i != json["experiments"].end(); i++) {
			// For every experiment read in all parameters and values, then
			// append to member vectors
			std::vector<std::pair<std::string, float>> params;
			std::vector<std::string> sweep_params;
			std::vector<std::vector<float>> sweep_values;
			std::string name = i.key();
			if (i.value().find("repeat") != i.value().end()) {
				m_repetitions.emplace_back(i.value()["repeat"]);
			}
			else {
				m_repetitions.emplace_back(1);
			}
			if (i.value().find("optimal_sample_count") != i.value().end()) {
				m_optimal_sample.emplace_back(i.value()["optimal_sample_count"]);
			}
			else {
				m_optimal_sample.emplace_back(false);
			}

			for (auto j = json["experiments"][name].begin();
			     j != json["experiments"][name].end(); j++) {
				const cypress::Json val = j.value();

				// See if val is a number (no sweep), an array or an object
				if (val.is_number()) {
					if (j.key() == "repeat") {
						continue;
					}
					else {
						params.emplace_back(
						    std::pair<std::string, float>(j.key(), val));
					}
				}
				else if (val.is_array()) {
					if (val.size() == 1) {
						params.emplace_back(
						    std::pair<std::string, float>(j.key(), val));
					}
					else {
						add_sweep_parameter(j.key(), val, sweep_params,
						                    sweep_values, m_repetitions.back());
					}
				}
				else if (val.is_object()) {
					auto map = json_to_map<float>(val);
					auto range = read_check<float>(map, names,
					                               std::vector<float>{0, 0, 0});
					std::vector<float> values;
					double step = (range[1] - range[0]) / (range[2] - 1.0);
					for (size_t k = 0; k < range[2]; k++) {
						values.emplace_back<float>(range[0] + float(k) * step);
					}
					add_sweep_parameter(j.key(), values, sweep_params,
					                    sweep_values, m_repetitions.back());
				}
				else {
					throw std::invalid_argument("Unknown Json value!");
				}
			}
			m_params.emplace_back(params);
			m_sweep_params.emplace_back(sweep_params);
			m_sweep_values.emplace_back(sweep_values);
			experiment_names.emplace_back(name);
		}
	}
}

void Experiment::run_standard(std::string file_name)
{

	std::ofstream ofs, null;
	ofs =
	    std::ofstream(file_name + "_" + m_backend + ".txt", std::ofstream::app);

	SpikingBinam SpBinam(json, null, true);
	run_standard_neat_output(SpBinam, ofs, m_backend, true);
}

void Experiment::run_no_data(size_t exp,
                             std::vector<std::vector<std::string>> &names,
                             std::ostream &ofs)
{
	std::vector<std::pair<ExpResults, ExpResults>> results(
	    m_sweep_values[exp].size(), std::pair<ExpResults, ExpResults>());
	std::vector<std::vector<std::string>> params_names;
	std::vector<size_t> params_indices;
	DataParameters data_params =
	    prepare_data_params(json, params_names, params_indices, m_params[exp]);
	if (m_optimal_sample[exp]) {
		data_params.optimal_sample_count();
	}
	std::vector<size_t> counter;  // for the number of parallel networks
	std::ofstream out;            // suppress output
	SpikingBinam sp_binam(json, data_params, out, true, false);  // Standard binam
	std::vector<SpikingBinam>
	    sp_binam_vec;       // Emplace binam network for every parameter run
	cypress::Network netw;  // shared network
	size_t neuron_count =
	    data_params.bits_out();  // number of neurons in next run

	// Single parameter settings
	for (size_t k : params_indices) {
		set_parameter(sp_binam, params_names[k], m_params[exp][k].second);
	}

	// If there are no sweep values
	if (m_sweep_values[exp].size() == 0) {
		if (m_repetitions[exp] == 1) {
			run_standard_neat_output(sp_binam, ofs, m_backend, true);
		}
		else {
			run_standard_neat_output(sp_binam, ofs, m_backend, true, false,
			                         false);
			for (size_t repeat_counter = 0;
			     repeat_counter < m_repetitions[exp] - 2;
			     repeat_counter++) {  // do all repetitions
				run_standard_neat_output(sp_binam, ofs, m_backend, false, false,
				                         false);
			}
			run_standard_neat_output(sp_binam, ofs, m_backend, false, false,
			                         true);
		}
	}

	else {
		ofs << "# ";
		for (size_t j = 0; j < names.size(); j++) {
			ofs << names[j][1] << ", ";
		}
		ofs << "info, info_th,info_n, fp, fp_th, fn, fn_th" << std::endl;

		std::default_random_engine generator(1010);
		std::vector<size_t> indices(m_sweep_values[exp].size());
		for (size_t j = 0; j < m_sweep_values[exp].size(); j++) {
			indices[j] = j;
		}
		std::shuffle(indices.begin(), indices.end(), generator);
		for (size_t j = 0; j < m_sweep_values[exp].size(); j++) {  // all values
			// std::uniform_int_distribution<int> distribution(0,
			//                                                indices.size() -
			//                                                1);
			// size_t indices_index = distribution(generator);
			size_t index = indices[j];
			// indices.erase(indices.begin() + indices_index);

			sp_binam_vec.push_back(sp_binam);
			for (size_t k = 0; k < m_sweep_values[exp][index].size(); k++) {
				set_parameter(sp_binam_vec[counter.size()], names[k],
				              m_sweep_values[exp][index][k]);
			}
			sp_binam_vec[counter.size()].build(netw);
			counter.emplace_back(index);
			check_run(sp_binam_vec, m_sweep_values[exp], netw, j, counter,
			          m_backend, results, neuron_count);
		}
		output(m_sweep_values[exp], results, ofs, names[0]);
	}
}

void Experiment::run_data(size_t exp,
                          std::vector<std::vector<std::string>> &names,
                          std::ostream &ofs)
{
	std::vector<std::pair<ExpResults, ExpResults>> results(
	    m_sweep_values[exp].size(), std::pair<ExpResults, ExpResults>());
	std::vector<std::vector<std::string>> params_names;
	std::vector<size_t> params_indices;
	DataParameters data_params =
	    prepare_data_params(json, params_names, params_indices, m_params[exp]);
	std::vector<size_t> counter;  // for the number of parallel networks
	std::ofstream out;            // suppress output
	std::vector<SpikingBinam>
	    sp_binam_vec;         // Emplace binam network for every parameter run
	cypress::Network netw;    // shared network
	int bits_out_index = -1;  // if bits_out are changed, this is the index

	std::vector<size_t> data_indices, other_indices;
	for (size_t k = 0; k < names.size(); k++) {
		if (names[k][0] != "data") {
			other_indices.emplace_back(k);
		}
		else {
			data_indices.emplace_back(k);
		}
		if (names[k][1] == "n_bits_out") {
			bits_out_index = k;
		}
	}

	ofs << "# ";
	for (size_t j = 0; j < names.size(); j++) {
		ofs << names[j][1] << ", ";
	}
	ofs << "info, info_th,info_n, fp, fp_th, fn, fn_th" << std::endl;

	std::default_random_engine generator(1010);
	std::vector<size_t> indices(m_sweep_values[exp].size());
	for (size_t j = 0; j < m_sweep_values[exp].size(); j++) {
		indices[j] = j;
	}
	std::shuffle(indices.begin(), indices.end(), generator);

	for (size_t j = 0; j < m_sweep_values[exp].size(); j++) {  // all values
		size_t index = indices[j];
		for (auto k : data_indices) {
			data_params.set(names[k][1], m_sweep_values[exp][index][k]);
		}
		if (m_optimal_sample[exp]) {
			data_params.optimal_sample_count();
		}
		sp_binam_vec.emplace_back(SpikingBinam(json, data_params, out));
		for (size_t k : params_indices) {
			set_parameter(sp_binam_vec.back(), params_names[k],
			              m_params[exp][k].second);
		}
		for (auto k : other_indices) {
			set_parameter(sp_binam_vec.back(), names[k],
			              m_sweep_values[exp][index][k]);
		}

		// sp_binam_vec.emplace_back(sp_binam);
		sp_binam_vec.back().build(netw);
		counter.emplace_back(index);
		size_t neuron_count = 0;

		if (j + 1 < m_sweep_values.size() && bits_out_index >= 0) {
			neuron_count = m_sweep_values[exp][indices[j + 1]][bits_out_index];
		}
		else {
			neuron_count = data_params.ones_out();
		}

		check_run(sp_binam_vec, m_sweep_values[exp], netw, j, counter,
		          m_backend, results, neuron_count);
	}
	output(m_sweep_values[exp], results, ofs, names[0]);
}

int Experiment::run(std::string file_name)
{
	if (standard) {
		run_standard(file_name);
		return 0;
	}
	for (size_t i = 0; i < m_sweep_params.size(); i++) {  // for every
		                                                  // experiment

		// Splitting names for usage
		std::vector<std::vector<std::string>> names;
		for (auto j : m_sweep_params[i]) {
			names.emplace_back(split(j, '.'));
		}

		// Check, wether DataParameters was changed. This is important
		// for later computation
		bool data_changed = false;
		for (size_t k = 0; k < names.size(); k++) {
			if (names[k][0] == "data") {
				data_changed = true;
			}
		}

		// Open file and write first line
		std::ofstream ofs(experiment_names[i] + "_" + m_backend + ".csv",
		                  std::ofstream::out);

		if (!data_changed) {
			run_no_data(i, names, ofs);
		}
		else {
			run_data(i, names, ofs);
		}
	}
	return 0;
}
}
