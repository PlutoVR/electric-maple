// Copyright 2023, PlutoVR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal header for a data structure used by the ElectricMaple XR streaming solution
 * @author Rylie Pavlik <rpavlik@collabora.com>
 * @ingroup em_client
 */
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>

namespace em {


namespace id_data_accum {

	using IdType = std::int64_t;
	static constexpr IdType kSentinel = 0;
	/*!
	 * How should the currently-visited element be handled?
	 */
	enum class Command
	{
		/// This record should be dropped
		Drop,
		/// Keep this record for additional data
		Keep,
	};

	/*!
	 * Collecting data for increasing key values.
	 *
	 * @tparam ValueType the data structure associated with each key
	 * @tparam MaxSize the fixed max size of our buffer of in-progress data structures, in elements.
	 */
	template <typename ValueType, std::size_t MaxSize> class IdDataAccumulator
	{
	public:
		/// Constructor
		IdDataAccumulator();

		/// Clear all entries.
		void
		clear();

		/// Get a pointer to the value corresponding to that ID, or nullptr if not found.
		ValueType *
		getForId(IdType id);

		/// Get a pointer to the const value corresponding to that ID, or nullptr if not found.
		ValueType const *
		getConstForId(IdType id) const;

		/// Get a pointer to the const value corresponding to that ID, or nullptr if not found.
		ValueType const *
		getForId(IdType id) const;

		/// Get the number of entries in progress
		size_t
		size() const;

		/*!
		 * Add a data structure with the given ID.
		 *
		 * @param id ID of data
		 * @param value The structure you'd like to add
		 * @return true if the data was actually added
		 *
		 * @throws if the ID already exists or is the sentinel
		 */
		bool
		addDataFor(IdType id, ValueType &&value);

		/*!
		 * Look for a data structure with the given ID. If it exists, call the functor on it.
		 *
		 * @param id ID of data
		 * @param dataUpdater A functor taking ValueType& that will update the data for that key, if found
		 * @return true if the ID was found and functor was called.
		 */
		template <typename F>
		bool
		updateDataFor(IdType id, F &&dataUpdater)
		{
			ValueType *ptr = getForId(id);
			if (ptr) {
				dataUpdater(*ptr);
				return true;
			}
			// we didn't find it
			return false;
		}

		/*!
		 * Call your functor on all populated entries, so you can emit them if they're ready to go.
		 *
		 * @param dataHandler A functor taking IdType and ValueType& that will do stuff with the data and return
		 * @ref IdAccumCommand
		 *
		 * @return true if any in-progress structures remain
		 */
		template <typename F>
		bool
		visitAll(F &&dataHandler)
		{
			bool haveAnyKeepers = false;
			for (auto &p : m_data) {
				if (isPairPopulated(p)) {
					Command cmd = dataHandler(p.first, p.second);
					switch (cmd) {
					case Command::Drop: markPairUnpopulated(p); break;
					case Command::Keep: haveAnyKeepers = true; break;
					}
				}
			}
			return haveAnyKeepers;
		}
		/*!
		 * Call your functor on all populated entries, as const.
		 *
		 * @param dataHandler A functor taking IdType and const ValueType& that will do stuff with the data
		 *
		 * @return true if any entries exist and were visited
		 */
		template <typename F>
		bool
		constVisitAll(F &&dataHandler) const
		{
			bool haveAny = false;
			for (const auto &p : m_data) {
				if (isPairPopulated(p)) {
					dataHandler(p.first, p.second);
					haveAny = true;
				}
			}
			return haveAny;
		}


	private:
		using PairType = std::pair<IdType, ValueType>;
		using ArrayType = std::array<PairType, MaxSize>;
		using ArrayIterator = typename ArrayType::iterator;
		using ArrayConstIterator = typename ArrayType::const_iterator;
		void
		minMaxMatch(IdType id, ArrayIterator &minElt, ArrayIterator &maxElt, ArrayIterator &matchingElt)
		{
			const auto b = m_data.begin();
			const auto e = m_data.end();
			bool foundEmpty = false;
			IdType minId = kSentinel;
			IdType maxId = kSentinel;
			minElt = e;
			maxElt = e;
			matchingElt = e;
			for (auto it = b; it != e; ++it) {
				IdType eltId = it->first; // might be the sentinel

				if (isPairPopulated(*it)) {
					if (minId == kSentinel || minId > eltId) {
						if (!foundEmpty) {
							// a "populated" pair is never smaller than an empty pair
							minId = eltId;
							minElt = it;
						}
					}
					if (maxId == kSentinel || eltId > maxId) {
						maxId = eltId;
						maxElt = it;
					}
					if (eltId == id) {
						matchingElt = it;
					}
				} else {
					// OK this is an empty one.
					if (!foundEmpty) {
						minElt = it;
						minId = eltId;
						foundEmpty = true;
					}
				}
			}
		}

		static bool
		isPairPopulated(PairType const &p)
		{
			return p.first != kSentinel;
		}
		static void
		markPairUnpopulated(PairType &p)
		{
			p.first = kSentinel;
			p.second = {};
		}
		ArrayType m_data;
	};

	// - private implementation follows - //

	template <typename ValueType, std::size_t MaxSize>
	inline bool
	IdDataAccumulator<ValueType, MaxSize>::addDataFor(IdType id, ValueType &&value)
	{
		if (id == kSentinel) {
			throw std::logic_error("Sentinel ID passed to addDataFor");
		}
		const auto b = m_data.begin();
		const auto e = m_data.end();
		ArrayIterator minElt{e};
		ArrayIterator maxElt{e};
		ArrayIterator matchElt{e};
		minMaxMatch(id, minElt, maxElt, matchElt);
		if (matchElt != e) {
			// this one is already in there?!
			throw std::logic_error("ID already present in accumulator");
		}

		// min is either empty or the oldest
		if (isPairPopulated(*minElt)) {
			if (id < minElt->first) {
				// Do not insert if it would make the oldest entry older
				return false;
			}
			// TODO do we notify about forgetting this?
		}

		minElt->first = id;
		minElt->second = std::move(value);
		return true;
	}

	template <typename ValueType, std::size_t MaxSize>
	inline IdDataAccumulator<ValueType, MaxSize>::IdDataAccumulator()
	{
		clear();
	}

	template <typename ValueType, std::size_t MaxSize>
	inline void
	IdDataAccumulator<ValueType, MaxSize>::clear()
	{
		for (auto &p : m_data) {
			markPairUnpopulated(p);
		}
	}

	template <typename ValueType, std::size_t MaxSize>
	inline ValueType *
	IdDataAccumulator<ValueType, MaxSize>::getForId(IdType id)
	{
		const auto b = m_data.begin();
		const auto e = m_data.end();
		auto it = std::find_if(b, e, [=](PairType &p) { return p.first == id; });
		if (it == e) {
			return nullptr;
		}
		return &(it->second);
	}

	template <typename ValueType, std::size_t MaxSize>
	inline ValueType const *
	IdDataAccumulator<ValueType, MaxSize>::getConstForId(IdType id) const
	{
		const auto b = m_data.begin();
		const auto e = m_data.end();
		auto it = std::find_if(b, e, [=](PairType const &p) { return p.first == id; });
		if (it == e) {
			return nullptr;
		}
		return &(it->second);
	}

	template <typename ValueType, std::size_t MaxSize>
	inline ValueType const *
	IdDataAccumulator<ValueType, MaxSize>::getForId(IdType id) const
	{
		return getConstForId(id);
	}

	template <typename ValueType, std::size_t MaxSize>
	inline size_t
	IdDataAccumulator<ValueType, MaxSize>::size() const
	{
		return std::count_if(m_data.begin(), m_data.end(), &isPairPopulated);
	}
}; // namespace id_data_accum

using id_data_accum::IdDataAccumulator;

} // namespace em
