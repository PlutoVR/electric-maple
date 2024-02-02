// Copyright 2023, Pluto VR, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 */

#include "catch2/catch_message.hpp"
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_quantifiers.hpp"
#include "catch2/matchers/catch_matchers_range_equals.hpp"

#include "em/em_id_data_accumulator.hpp"
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <vector>

static constexpr std::size_t MaxElts = 3;

using em::id_data_accum::IdType;

namespace {

struct MyData {
  bool a;
  bool b;
};

constexpr IdType kGoodId[] = {11, 12, 13, 14};
constexpr IdType kOldId = 5;
static_assert(kOldId < kGoodId[0],
              "the old ID must be less than the smallest good ID");

std::vector<IdType>
visitIds(em::IdDataAccumulator<MyData, MaxElts> const &accum) {
  std::vector<IdType> ids;
  accum.constVisitAll([&](IdType id, MyData const &) { ids.emplace_back(id); });
  return ids;
}

} // namespace

TEST_CASE("IdData") {

  em::IdDataAccumulator<MyData, MaxElts> accum{};

  CHECK(accum.size() == 0);
  SECTION("Filling the structure") {
    CHECK(accum.size() == 0);

    INFO("Adding the first element: id " << kGoodId[0]);
    CHECK(accum.addDataFor(kGoodId[0], {}));
    {
      INFO("Checking state after adding the first one");
      CHECK(accum.size() == 1);
      CHECK(accum.getConstForId(kGoodId[0]));
      CHECK_FALSE(accum.getConstForId(kGoodId[1]));
      CHECK_FALSE(accum.getConstForId(kGoodId[2]));
      CHECK_FALSE(accum.getConstForId(kGoodId[3]));
      CHECK_FALSE(accum.getConstForId(kOldId));
      CHECK_THAT(visitIds(accum),
                 Catch::Matchers::UnorderedRangeEquals(
                     std::initializer_list<IdType>{kGoodId[0]}));
    }

    INFO("Adding the second element: id " << kGoodId[1]);
    CHECK(accum.addDataFor(kGoodId[1], {}));
    {
      INFO("Checking state after adding the second one");
      CHECK(accum.size() == 2);
      CHECK(accum.getConstForId(kGoodId[0]));
      CHECK(accum.getConstForId(kGoodId[1]));
      CHECK_FALSE(accum.getConstForId(kGoodId[2]));
      CHECK_FALSE(accum.getConstForId(kGoodId[3]));
      CHECK_FALSE(accum.getConstForId(kOldId));
      CHECK_THAT(visitIds(accum),
                 Catch::Matchers::UnorderedRangeEquals(
                     std::initializer_list<IdType>{kGoodId[0], kGoodId[1]}));
    }

    SECTION("Add old ID before container is full") {
      CHECK(accum.addDataFor(kOldId, {}));
      CHECK(accum.size() == 3);

      CHECK(accum.getConstForId(kGoodId[0]));
      CHECK(accum.getConstForId(kGoodId[1]));
      CHECK(accum.getConstForId(kOldId));
      CHECK_FALSE(accum.getConstForId(kGoodId[2]));
      CHECK_FALSE(accum.getConstForId(kGoodId[3]));

      CHECK_THAT(visitIds(accum), Catch::Matchers::UnorderedRangeEquals(
                                      std::initializer_list<IdType>{
                                          kGoodId[0], kGoodId[1], kOldId}));
    }

    INFO("Adding the third element: id " << kGoodId[2]);
    CHECK(accum.addDataFor(kGoodId[2], {}));
    {
      INFO("Checking state after adding the third one");
      CHECK(accum.size() == 3);

      CHECK(accum.getConstForId(kGoodId[0]));
      CHECK(accum.getConstForId(kGoodId[1]));
      CHECK(accum.getConstForId(kGoodId[2]));
      CHECK_FALSE(accum.getConstForId(kGoodId[3]));
      CHECK_FALSE(accum.getConstForId(kOldId));

      CHECK_THAT(visitIds(accum), Catch::Matchers::UnorderedRangeEquals(
                                      std::initializer_list<IdType>{
                                          kGoodId[0], kGoodId[1], kGoodId[2]}));
    }

    SECTION("Reject duplicate IDs") {
      CHECK_THROWS(accum.addDataFor(kGoodId[0], {}));
      CHECK_THROWS(accum.addDataFor(kGoodId[1], {}));
      CHECK_THROWS(accum.addDataFor(kGoodId[2], {}));
    }

    SECTION("Reject sentinel ID") {
      CHECK_THROWS(accum.addDataFor(em::id_data_accum::kSentinel, {}));
    }

    SECTION("Reject too old") {
      CHECK_FALSE(accum.addDataFor(kOldId, {}));
      CHECK(accum.size() == 3);
      {
        INFO("Verify we have all the items that we expect, and none we don't");
        CHECK(accum.getConstForId(kGoodId[0]));
        CHECK(accum.getConstForId(kGoodId[1]));
        CHECK(accum.getConstForId(kGoodId[2]));
        CHECK(nullptr == accum.getConstForId(kOldId));
      }

      CHECK_THAT(visitIds(accum), Catch::Matchers::UnorderedRangeEquals(
                                      std::initializer_list<IdType>{
                                          kGoodId[0], kGoodId[1], kGoodId[2]}));
    }

    SECTION("Newer additional value") {
      CHECK(accum.addDataFor(kGoodId[3], {}));
      CHECK(accum.size() == 3);
      {
        INFO("Verify we have all the items that we expect, and none we don't");
        CHECK(nullptr == accum.getConstForId(kGoodId[0]));
        CHECK(accum.getConstForId(kGoodId[1]));
        CHECK(accum.getConstForId(kGoodId[2]));
        CHECK(accum.getConstForId(kGoodId[3]));
      }

      CHECK_THAT(visitIds(accum), Catch::Matchers::UnorderedRangeEquals(
                                      std::initializer_list<IdType>{
                                          kGoodId[3], kGoodId[1], kGoodId[2]}));
    }

    SECTION("Modify non-present value") {
      bool functorCalled = false;
      accum.updateDataFor(kOldId, [&](MyData &data) {
        functorCalled = true;
        data.b = true;
      });
      CHECK_FALSE(functorCalled);
      accum.constVisitAll([](IdType id, MyData const &data) {
        CAPTURE(id);
        CHECK(data.b == false);
      });
    }

    SECTION("Modify values") {
      accum.getForId(kGoodId[1])->a = true;

      bool functorCalled = false;
      accum.updateDataFor(kGoodId[2], [&](MyData &data) {
        functorCalled = true;
        data.b = true;
      });

      CHECK(functorCalled);

      {
        INFO("Verify that updateDataFor of "
             << kGoodId[2] << " affected only the desired field/structure");
        accum.constVisitAll([](IdType id, MyData const &data) {
          CAPTURE(id);
          if (id == kGoodId[2]) {
            CHECK(data.b);
          } else {
            CHECK(data.b == false);
          }
        });
      }
      {
        INFO("Verify that modification through getForId of "
             << kGoodId[1] << " affected only the desired field/structure");
        accum.constVisitAll([](IdType id, MyData const &data) {
          CAPTURE(id);
          if (id == kGoodId[1]) {
            CHECK(data.a);
          } else {
            CHECK(data.a == false);
          }
        });
      }
    }

    // we have good id's 0, 1, 2 right now.
    SECTION("Drop oldest value") {
      IdType toDrop = kGoodId[0];
      INFO("Dropping " << toDrop);
      bool anyLeft = accum.visitAll([=](IdType id, MyData &) {
        if (id == toDrop) {
          return em::id_data_accum::Command::Drop;
        }
        return em::id_data_accum::Command::Keep;
      });
      CHECK(anyLeft);
      CHECK(accum.size() == 2);
      CHECK_FALSE(accum.getConstForId(kGoodId[0]));
      CHECK(accum.getConstForId(kGoodId[1]));
      CHECK(accum.getConstForId(kGoodId[2]));

      CHECK_THAT(visitIds(accum),
                 Catch::Matchers::UnorderedRangeEquals(
                     std::initializer_list<IdType>{kGoodId[1], kGoodId[2]}));
      {
        // TODO do we want this behavior?
        INFO("Should be able to add old value now that we have space again");
        CHECK(accum.addDataFor(kOldId, {}));
      }
    }
    SECTION("Drop newest value") {
      IdType toDrop = kGoodId[2];
      INFO("Dropping " << toDrop);
      bool anyLeft = accum.visitAll([=](IdType id, MyData &) {
        if (id == toDrop) {
          return em::id_data_accum::Command::Drop;
        }
        return em::id_data_accum::Command::Keep;
      });
      CHECK(anyLeft);
      CHECK(accum.size() == 2);

      CHECK(accum.getConstForId(kGoodId[0]));
      CHECK(accum.getConstForId(kGoodId[1]));
      CHECK_FALSE(accum.getConstForId(kGoodId[2]));

      CHECK_THAT(visitIds(accum),
                 Catch::Matchers::UnorderedRangeEquals(
                     std::initializer_list<IdType>{kGoodId[0], kGoodId[1]}));

      {
        // TODO do we want this behavior?
        INFO("Should be able to add old value now that we have space again");
        CHECK(accum.addDataFor(kOldId, {}));
      }
    }

    SECTION("Drop all values") {
      INFO("Dropping all through visitAll return value");
      bool anyLeft = accum.visitAll(
          [](IdType id, MyData &) { return em::id_data_accum::Command::Drop; });
      CHECK_FALSE(anyLeft);
      CHECK(accum.size() == 0);
      {
        // TODO do we want this behavior?
        INFO("Should be able to add old value now that we have space again");
        CHECK(accum.addDataFor(kOldId, {}));
      }
    }
  }
}
