#include "./unit_tests.h"

#include "../util/store/collection_util.h"
#include "../util/store/Map.h"

namespace {
	struct uint_hash {
		hash_t operator()(uint u) {
			return u;
		}
	};

	void unit_test_map() {
		Arena temp;
		Map<uint, uint, uint_hash> fast { 64, temp };

		MaxSizeVector<3, Pair<uint, uint>> pairs;
		pairs.push({ 1, 1 });
		pairs.push({ 2, 2 });
		pairs.push({ 3, 3 });

		for (const Pair<uint, uint>& p : pairs) {
			//slow.must_insert(p.first, p.second);
			KeyValuePair<uint, uint>& f = fast.must_insert(p.first, p.second);
			assert(f.key == p.first && f.value == p.second);
			assert(fast.has(p.first));
		}
		for (const Pair<uint, uint>& p : pairs)
			assert(!fast.try_insert(p.first, 0).was_added);

		fast.each([&](uint key, uint value) {
			const Pair<uint, uint>& pair = find(pairs, [&](const Pair<uint, uint>& p) { return p.first == key; }).get();
			assert(pair.second == value);
		});
	}
}

void unit_tests() {
	unit_test_map();
}
