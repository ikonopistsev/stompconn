#include "stompconn/handler.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

static void require(bool value)
{
	if (!value) throw std::runtime_error("receipt assertion failed");
}

static stompconn::packet packet()
{
	static stompconn::header_store headers;
	return {headers, "", st_method_receipt, {}};
}

int main()
{
	for (unsigned int order = 0; order < 3; ++order) {
		stompconn::receipt_handler handler;
		std::vector<std::string> ids;
		std::vector<unsigned int> seen(10000);
		const auto begin = std::chrono::steady_clock::now();
		for (std::size_t i = 0; i < seen.size(); ++i) {
			ids.emplace_back(handler.create([&, i](stompconn::packet) {
				++seen[i];
			}));
		}
		if (order == 1) std::reverse(ids.begin(), ids.end());
		if (order == 2) {
			std::mt19937 random(7);
			std::shuffle(ids.begin(), ids.end(), random);
		}
		for (const auto& id : ids) {
			require(handler.call(id, packet()));
			require(!handler.call(id, packet()));
		}
		require(std::all_of(seen.begin(), seen.end(), [](unsigned int value) {
			return value == 1;
		}));
		require(!handler.call("unknown", packet()));
		std::cout << "10000 receipts order=" << order << " us="
				  << std::chrono::duration_cast<std::chrono::microseconds>(
						 std::chrono::steady_clock::now() - begin)
						 .count()
				  << '\n';
	}
	auto handler = std::make_unique<stompconn::receipt_handler>();
	std::string id;
	bool called = false;
	bool recursive_call = true;
	id = handler->create([&](stompconn::packet) {
		called = true;
		recursive_call = handler->call(id, packet());
		handler->clear();
		handler->create([](stompconn::packet) {
		});
		handler.reset();
	});
	require(handler->call(id, packet()));
	require(called && !recursive_call && !handler);
}
