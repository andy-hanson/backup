#pragma once

#include "../Option.h"
#include "./Arena.h"
#include "./ArenaArrayBuilders.h"
#include "./KeyValuePair.h"

template <typename K, typename V, typename Hash>
class Map {
	struct Entry {
		KeyValuePair<K, V> pair;
		Option<Ref<Entry>> next_in_chain;
	};
	static uint array_elements_size(uint arr_size) {
		return arr_size * 3 / 4;
	}
	static uint index(const K& key, uint arr_size) {
		// Only use the first 3/4 of the array -- rest is for conflicts. https://en.wikipedia.org/wiki/Coalesced_hashing
		hash_t hash = Hash{}(key);
		return hash % array_elements_size(arr_size);
	}

	template <typename, typename, typename> friend struct build_map;
	Slice<Option<Entry>> arr;
	Map(Slice<Option<Entry>> _arr) : arr(_arr) {}

public:
	Map() : arr{} {}

	bool has(const K& key) const {
		return get(key).has();
	}

	Option<const V&> get(const K& key) const {
		assert(!arr.is_empty());
		const Option<Entry>& op_entry = arr[index(key, arr.size())];
		if (!op_entry.has())
			// Nothing has that hash.
			return Option<const V&> {};

		Ref<const Entry> entry = &op_entry.get();
		while (true) {
			if (entry->pair.key == key) return Option<const V&> { entry->pair.value };
			if (!entry->next_in_chain.has())
				return Option<const V&> {};
			entry = entry->next_in_chain.get();
		}
	}
};

template <typename K, typename V, typename Hash>
class build_map {
	using M = Map<K, V, Hash>;
	using Entry = typename M::Entry;

	// Note that conflicts come from right, so last_conflict is the leftmost conflict slot.
	template <typename Input, typename CbGetKey, typename CbGetValue, typename CbConflict>
	static void insert(
		Slice<Option<Entry>> arr, const Input& input, Option<Entry>* leftmost_conflict_slot, Option<Entry>*& next_conflict_slot,
		CbGetKey& get_key, CbGetValue& get_value, CbConflict& on_conflict) {
		const K& key = get_key(input);
		Option<Entry>& op_entry = arr[M::index(key, arr.size())];
		if (!op_entry.has()) {
			op_entry = Entry { KeyValuePair<K, V> { key, get_value(input) }, {} };
			return;
		}

		Ref<Entry> entry = &op_entry.get();
		while (true) {
			if (entry->pair.key == key) {
				// True conflict
				on_conflict(entry->pair.value, input);
				return;
			}

			if (!entry->next_in_chain.has()) {
				if (next_conflict_slot < leftmost_conflict_slot) todo();
				*next_conflict_slot = Entry { KeyValuePair<K, V> { key, get_value(input) }, {} };
				entry->next_in_chain = Ref<Entry> { &next_conflict_slot->get() };
				--next_conflict_slot;
				return;
			}

			// Else, continue
			entry = entry->next_in_chain.get();
		}
	}

public:
	template <typename Input, typename /*const Input& => K*/ CbGetKey, typename /*const Input& => V*/ CbGetValue, typename /*(const V&, const Input&) => void*/ CbConflict>
	Map<K, V, Hash> operator()(Arena& arena, const Slice<Input>& inputs, CbGetKey get_key, CbGetValue get_value, CbConflict on_conflict) {
		if (inputs.is_empty())
			return {};

		Slice<Option<Entry>> arr = fill_array<Option<Entry>>()(arena, inputs.size() * 3, [](uint i __attribute__((unused))) { return Option<Entry> {}; });

		// Fills from right.
		// This should never reach *begin since we have twice as many array entries as elements in the map.
		Option<Entry>* last_conflict_slot = arr.begin() + M::array_elements_size(arr.size());
		Option<Entry>* next_conflict_slot = arr.end() - 1;
		for (const Input& input : inputs)
			build_map::insert(arr, input, last_conflict_slot, next_conflict_slot, get_key, get_value, on_conflict);
		return { arr };
	}
};
