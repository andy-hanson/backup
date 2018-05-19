#pragma once

#include "../Option.h"
#include "./Arena.h"
#include "./ArenaArrayBuilders.h"
#include "./KeyValuePair.h"

template <typename K, typename V>
struct InsertResult {
	bool was_added;
	KeyValuePair<K, V>& pair;
};

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
	Option<Entry>* leftmost_conflict_slot;
	Option<Entry>* next_conflict_slot;

	Map() : arr{}, leftmost_conflict_slot{nullptr}, next_conflict_slot{nullptr} {}

public:
	Map(uint capacity, Arena& arena)
		: arr{fill_array<Option<Entry>>{}(arena, capacity, [](uint i __attribute__((unused))) { return Option<Entry> {}; })},
		leftmost_conflict_slot{arr.begin() + array_elements_size(capacity)},
		next_conflict_slot{arr.begin() + capacity - 1}
		{}

	inline static Map empty() { return {}; }

	inline bool has(const K& key) const {
		return get(key).has();
	}

	Option<const KeyValuePair<K, V>&> get_pair(const K& key) const {
		assert(!arr.is_empty());
		const Option<Entry>& op_entry = arr[index(key, arr.size())];
		if (!op_entry.has())
			return Option<const KeyValuePair<K, V>&> {};
		Ref<const Entry> entry = &op_entry.get();
		while (true) {
			if (entry->pair.key == key)
				return Option<const KeyValuePair<K, V>&> { entry->pair };
			if (!entry->next_in_chain.has())
				return Option<const KeyValuePair<K, V>&> {};
			entry = entry->next_in_chain.get();
		}
	}
	Option<KeyValuePair<K, V>&> get_pair(const K& key) {
		assert(!arr.is_empty());
		Option<Entry>& op_entry = arr[index(key, arr.size())];
		if (!op_entry.has())
			return Option<KeyValuePair<K, V>&> {};
		Ref<Entry> entry = &op_entry.get();
		while (true) {
			if (entry->pair.key == key)
				return Option<KeyValuePair<K, V>&> { entry->pair };
			if (!entry->next_in_chain.has())
				return Option<KeyValuePair<K, V>&> {};
			entry = entry->next_in_chain.get();
		}
	}

	// Note that conflicts come from right, so last_conflict is the leftmost conflict slot.
	InsertResult<K, V> try_insert(K key, V value) {
		Option<Entry>& op_entry = arr[index(key, arr.size())];
		if (!op_entry.has()) {
			op_entry = Entry { KeyValuePair<K, V> { key, value }, {} };
			return { true, op_entry.get().pair };
		}

		Ref<Entry> entry = &op_entry.get();
		while (true) {
			if (entry->pair.key == key) {
				return { false, entry->pair };
			}

			if (!entry->next_in_chain.has()) {
				if (next_conflict_slot < leftmost_conflict_slot) todo();
				*next_conflict_slot = Entry { KeyValuePair<K, V> { key, value }, {} };
				KeyValuePair<K, V>& res = next_conflict_slot->get().pair;
				entry->next_in_chain = Ref<Entry> { &next_conflict_slot->get() };
				--next_conflict_slot;
				return { true, res };
			}

			// Else, continue
			entry = entry->next_in_chain.get();
		}
	}

	inline KeyValuePair<K, V>& must_insert(const K& key, V value) {
		InsertResult<K, V> insert_result = try_insert(key, value);
		assert(insert_result.was_added);
		return insert_result.pair;
	}

	inline Option<const V&> get(const K& key) const {
		Option<const KeyValuePair<K, V>&> pair = get_pair(key);
		return pair.has() ? Option<const V&> { pair.get().value } : Option<const V&> {};
	}
	inline Option<V&> get(const K& key) {
		Option<KeyValuePair<K, V>&> pair = get_pair(key);
		return pair.has() ? Option<V&> { pair.get().value } : Option<V&> {};
	}

	inline const V& must_get(const K& key) const {
		return get(key).get();
	}
	inline V& must_get(const K& key) {
		return get(key).get();
	}

	inline Option<const K&> get_key_in_map(const K& key) const {
		Option<const KeyValuePair<K, V>&> pair = get_pair(key);
		return pair.has() ? Option<const K&> { pair.get().key } : Option<const K&> {};
	}

	template <typename /*const K&, const V& => void*/ Cb>
	void each(Cb cb) const {
		for (const Option<Entry>& op_e : arr) {
			if (op_e.has()) {
				const KeyValuePair<K, V>& pair = op_e.get().pair;
				cb(pair.key, pair.value);
			}
		}
	}
};

template <typename K, typename V, typename Hash>
class build_map {
	using M = Map<K, V, Hash>;
	using Entry = typename M::Entry;


public:
	template <typename Input, typename /*const Input& => K*/ CbGetKey, typename /*const Input& => V*/ CbGetValue, typename /*(const V&, const Input&) => void*/ CbConflict>
	Map<K, V, Hash> operator()(Arena& arena, const Slice<Input>& inputs, CbGetKey get_key, CbGetValue get_value, CbConflict on_conflict) {
		if (inputs.is_empty())
			return Map<K, V, Hash>::empty();

		Map<K, V, Hash> res { inputs.size() * 2, arena };

		for (const Input& input : inputs) {
			InsertResult<K, V> insert_result = res.try_insert(get_key(input), get_value(input));
			if (!insert_result.was_added)
				on_conflict(insert_result.pair.value, input);
		}
		return res;
	}
};
