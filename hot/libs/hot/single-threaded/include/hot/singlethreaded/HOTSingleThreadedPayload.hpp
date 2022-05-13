#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <utility>
#include <set>
#include <map>
#include <numeric>
#include <cstring>

#include <hot/commons/Algorithms.hpp>
#include <hot/commons/BiNode.hpp>
#include <hot/commons/DiscriminativeBit.hpp>
#include <hot/commons/InsertInformation.hpp>
#include <hot/commons/TwoEntriesNode.hpp>

#include "hot/singlethreaded/HOTSingleThreadedNode.hpp"

//Helper Data Structures
#include "HOTSingleThreadedInsertStackEntry.hpp"

#include "hot/singlethreaded/HOTSingleThreadedDeletionInformation.hpp"

#include "idx/contenthelpers/KeyUtilities.hpp"
#include "idx/contenthelpers/TidConverters.hpp"
#include "idx/contenthelpers/ContentEquals.hpp"
#include "idx/contenthelpers/KeyComparator.hpp"
#include "idx/contenthelpers/OptionalValue.hpp"
#include "TIDSpan.hpp"
#include "HOTSingleThreadedChildPointerInterface.hpp"
#include "HOTSingleThreadedIterator.hpp"


namespace hot {
    namespace singlethreaded {

        //constexpr uint32_t MAXIMUM_NUMBER_NODE_ENTRIES = 32u;

/**
 * HOTSingleThreaded represents a single threaded height optimized trie.
 *
 * The overall algorithm and the implementation details are described by Binna et al in the paper
 * "HOT: A Height Optimized Trie Index for Main-Memory Database Systems" in the proceedings of Sigmod 2018.
 *
 * @tparam ValueType The type of the value to index. The ValueType must no exceed 8 bytes and may only use the less significant 63 bits. It is therefore perfectly suited to use tuple identifiers as values.
 * @tparam KeyExtractor A Function given the ValueType returns a key, which by using the corresponding functions in idx::contenthelpers can be converted to a big endian ordered byte array.
 */
        template<typename ValueType, typename TIDType>
        class
        HOTSingleThreadedPayloadIterator;

        template<typename ValueType, typename TIDType>
        struct HOTSingleThreadedPayload {
            static_assert(sizeof(ValueType) <= 8, "Only value types which can be stored in a pointer are allowed");
            using ResultType = TIDSpan<ValueType, TIDType>;
            using const_iterator = HOTSingleThreadedPayloadIterator<ValueType, TIDType>;
            const_iterator END_ITERATOR{};

            HOTSingleThreadedChildPointer mRoot;
            std::vector<ResultType> tidList;

            static constexpr const ValueType &extractKey(const ResultType &res) {
                return res.value;
            }

            constexpr ResultType &tidToValue(intptr_t tid) {
                return tidList[tid];
            }

            constexpr intptr_t valueToTID(const ValueType value, const TIDType tid) {
                tidList.emplace_back(value, tid);
                return tidList.size() - 1;
            }


            //actually more a contains
            //TODO: test if val for comparison is actually needed
            static bool contentEquals(ValueType val, TIDType tid, ResultType res) {
                return extractKey(res) == val && res.contains(tid);
            }

        public:

            /**
             * Creates an empty order preserving index structure based on the HOT algorithm
             */
            HOTSingleThreadedPayload() : mRoot{}, tidList{} {
            }

            HOTSingleThreadedPayload(HOTSingleThreadedPayload const &other) = delete;

            HOTSingleThreadedPayload(HOTSingleThreadedPayload &&other) noexcept {
                mRoot = other.mRoot;
                other.mRoot = {};
                std::swap(tidList, other.tidList);
            }

            HOTSingleThreadedPayload &operator=(HOTSingleThreadedPayload const &other) = delete;

            HOTSingleThreadedPayload &operator=(HOTSingleThreadedPayload &&other) noexcept {
                mRoot = other.mRoot;
                other.mRoot = {};
                std::swap(tidList, other.tidList);
                return *this;
            }

            ~HOTSingleThreadedPayload() {
                mRoot.deleteSubtree();
            }

            inline bool isEmpty() const {
                return !mRoot.isLeaf() && mRoot.getNode() == nullptr;
            }

            inline bool isRootANode() const {
                return mRoot.isNode() && mRoot.getNode() != nullptr;
            }

            /**
             * For a given key it looks up the stored value
             *
             * @param key the key to lookup
             * @return the looked up value. The result is valid, if a matching record was found.
             */
            inline __attribute__((always_inline)) idx::contenthelpers::OptionalValue<ResultType>
            lookup(ValueType const &key) {
                auto const &fixedSizeKey = idx::contenthelpers::toFixSizedKey(
                        idx::contenthelpers::toBigEndianByteOrder(key));
                uint8_t const *byteKey = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);

                HOTSingleThreadedChildPointer current = mRoot;
                while (!current.isLeaf() && current.getNode() != nullptr) {
                    HOTSingleThreadedChildPointer const *const &currentChildPointer = current.search(byteKey);
                    current = *currentChildPointer;
                }
                return current.isLeaf() ? extractAndMatchLeafValue(current, key)
                                        : idx::contenthelpers::OptionalValue<ResultType>();
            }

            idx::contenthelpers::OptionalValue<ResultType>
            extractAndMatchLeafValue(HOTSingleThreadedChildPointer const &current, ValueType const &key) {
                ResultType const &value = tidToValue(current.getTid());
                return {idx::contenthelpers::contentEquals(extractKey(value), key), value};
            }

            /**
             * Scans a given number of values and returns the value at the end of the scan operation
             *
             * @param key the key to start the scanning operation at
             * @param numberValues the number of values to scan in sequential order
             * @return the record after scanning n values starting at the given key. If not the given number of values can be traversed the resulting value is invalid.
             */
            inline idx::contenthelpers::OptionalValue<ResultType>
            scan(ValueType const &key, size_t numberValues) const {
                const_iterator iterator = lower_bound(key);
                for (size_t i = 0u; i < numberValues && iterator != end(); ++i) {
                    ++iterator;
                }
                return iterator == end() ? idx::contenthelpers::OptionalValue<ResultType>({})
                                         : idx::contenthelpers::OptionalValue<ResultType>({true, *iterator});
            }

            inline unsigned int
            searchForInsert(uint8_t const *keyBytes, std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack) {
                HOTSingleThreadedChildPointer *current = &mRoot;
                unsigned int currentDepth = 0;
                while (!current->isLeaf()) {
                    insertStack[currentDepth].mChildPointer = current;
                    current = current->executeForSpecificNodeType(true, [&](auto &node) {
                        HOTSingleThreadedInsertStackEntry &currentStackEntry = insertStack[currentDepth];
                        return node.searchForInsert(currentStackEntry.mSearchResultForInsert, keyBytes);
                    });
                    ++currentDepth;
                }
                insertStack[currentDepth].initLeaf(current);
                return currentDepth;
            }

            inline bool remove(std::pair<ValueType, TIDType> const &toRemove) {
                auto &key = toRemove.first;
                auto const &fixedSizeKey = idx::contenthelpers::toFixSizedKey(
                        idx::contenthelpers::toBigEndianByteOrder(key));
                uint8_t const *keyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);
                bool wasContained = false;

                if (isRootANode()) {
                    std::array<HOTSingleThreadedInsertStackEntry, 64> insertStack;
                    unsigned int leafDepth = searchForInsert(keyBytes, insertStack);
                    intptr_t tid = insertStack[leafDepth].mChildPointer->getTid();
                    wasContained = contentEquals(key, tid, tidToValue(tid));
                    if (wasContained) {
                        removeWithStack(insertStack, leafDepth - 1);
                    }
                } else if (mRoot.isLeaf() && hasTheSameKey(mRoot.getTid(), key)) {
                    mRoot = HOTSingleThreadedChildPointer();
                    wasContained = true;
                }

                return wasContained;
            }

            /**
             * Inserts the given value into the index. The value is inserted according to its keys value.
             * In case the index already contains a value for the corresponding key, the value is not inserted.
             *
             * @param value the value to insert.
             * @return true if the value can be inserted, false if the index already contains a value for the corresponding key
             */
            inline bool insert(ValueType const &value, TIDType &tid) {
                bool inserted = true;
                auto const &fixedSizeKey = idx::contenthelpers::toFixSizedKey(
                        idx::contenthelpers::toBigEndianByteOrder(value));
                uint8_t const *keyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);

                if (isRootANode()) {
                    std::array<HOTSingleThreadedInsertStackEntry, 64> insertStack;
                    auto leafDepth = searchForInsert(keyBytes, insertStack);
                    auto tidPtr = insertStack[leafDepth].mChildPointer->getTid();
                    auto const &existingKey = extractKey(tidToValue(tidPtr));
                    inserted = insertWithInsertStack(insertStack, leafDepth, existingKey, keyBytes, value, tid);
                } else if (mRoot.isLeaf()) {

                    auto &currentLeafValue = tidToValue(mRoot.getTid());
                    auto const &existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(
                            idx::contenthelpers::toBigEndianByteOrder(extractKey(currentLeafValue)));
                    uint8_t const *existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);

                    inserted = hot::commons::executeForDiffingKeys(existingKeyBytes, keyBytes,
                                                                   idx::contenthelpers::getMaxKeyLength<ValueType>(),
                                                                   [&](hot::commons::DiscriminativeBit const &significantKeyInformation) {
                                                                       HOTSingleThreadedChildPointer valueToInsert(
                                                                               valueToTID(value, tid));
                                                                       hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(
                                                                               significantKeyInformation, mRoot,
                                                                               valueToInsert);
                                                                       mRoot = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(
                                                                               binaryNode)->toChildPointer();
                                                                   },
                                                                   [&]() {
                                                                       currentLeafValue.push_back(tid);
                                                                   });

                } else {
                    mRoot = HOTSingleThreadedChildPointer(valueToTID(value, tid));
                }
                return inserted;
            }

            inline void integrateBiNodeIntoTree(
                    std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack, unsigned int currentDepth,
                    hot::commons::BiNode<HOTSingleThreadedChildPointer> const &splitEntries, bool const newIsRight
            ) {
                if (currentDepth == 0) {
                    *insertStack[0].mChildPointer = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(
                            splitEntries)->toChildPointer();
                } else {
                    unsigned int parentDepth = currentDepth - 1;
                    HOTSingleThreadedInsertStackEntry const &parentInsertStackEntry = insertStack[parentDepth];
                    HOTSingleThreadedChildPointer parentNodePointer = *parentInsertStackEntry.mChildPointer;

                    HOTSingleThreadedNodeBase *existingParentNode = parentNodePointer.getNode();
                    if (existingParentNode->mHeight >
                        splitEntries.mHeight) { //create intermediate partition if height(partition) + 1 < height(parentPartition)
                        *insertStack[currentDepth].mChildPointer = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(
                                splitEntries)->toChildPointer();
                    } else { //integrate nodes into parent partition
                        hot::commons::DiscriminativeBit const significantKeyInformation{
                                splitEntries.mDiscriminativeBitIndex, newIsRight};

                        parentNodePointer.executeForSpecificNodeType(false, [&](auto &parentNode) -> void {
                            hot::commons::InsertInformation const &insertInformation = parentNode.getInsertInformation(
                                    parentInsertStackEntry.mSearchResultForInsert.mEntryIndex, significantKeyInformation
                            );

                            unsigned int entryOffset = (newIsRight) ? 0 : 1;
                            HOTSingleThreadedChildPointer valueToInsert{
                                    (newIsRight) ? splitEntries.mRight : splitEntries.mLeft};
                            HOTSingleThreadedChildPointer valueToReplace{
                                    (newIsRight) ? splitEntries.mLeft : splitEntries.mRight};

                            if (!parentNode.isFull()) {
                                HOTSingleThreadedChildPointer newNodePointer = parentNode.addEntry(insertInformation,
                                                                                                   valueToInsert);
                                newNodePointer.getNode()->getPointers()[
                                        parentInsertStackEntry.mSearchResultForInsert.mEntryIndex +
                                        entryOffset] = valueToReplace;
                                *parentInsertStackEntry.mChildPointer = newNodePointer;
                            } else {
                                //The diffing Bit index cannot be larger as the parents mostSignificantBitIndex. the reason is that otherwise
                                //the trie condition would be violated
                                assert(parentInsertStackEntry.mSearchResultForInsert.mMostSignificantBitIndex <
                                       splitEntries.mDiscriminativeBitIndex);
                                //Furthermore due to the trie condition it is safe to assume that both the existing entry and the new entry will be part of the same subtree
                                hot::commons::BiNode<HOTSingleThreadedChildPointer> const &newSplitEntries = parentNode.split(
                                        insertInformation, valueToInsert);
                                //Detect subtree side
                                //This newSplitEntries.mLeft.getHeight() == parentNodePointer.getHeight() check is important because in case of a split with 1:31 it can happend that if
                                //the 1 entry is not a leaf node the node it is pointing to will be pulled up, which implies that the numberEntriesInLowerPart are not correct anymore.
                                unsigned int numberEntriesInLowerPart =
                                        newSplitEntries.mLeft.getHeight() == parentNode.mHeight
                                        ? newSplitEntries.mLeft.getNumberEntries() : 1;
                                bool isInUpperPart = numberEntriesInLowerPart <=
                                                     parentInsertStackEntry.mSearchResultForInsert.mEntryIndex;
                                //Here is problem because of parentInsertstackEntry
                                unsigned int correspondingEntryIndexInPart =
                                        parentInsertStackEntry.mSearchResultForInsert.mEntryIndex -
                                        (isInUpperPart * numberEntriesInLowerPart) + entryOffset;
                                HOTSingleThreadedChildPointer nodePointerContainingSplitEntries = (isInUpperPart)
                                                                                                  ? newSplitEntries.mRight
                                                                                                  : newSplitEntries.mLeft;
                                nodePointerContainingSplitEntries.getNode()->getPointers()[correspondingEntryIndexInPart] = valueToReplace;
                                this->integrateBiNodeIntoTree(insertStack, parentDepth, newSplitEntries, true);
                            }
                            delete &parentNode;
                        });
                        //Order because branch prediction might choose this case in first place

                    }
                }
            }

            inline void insertNewValueIntoNode(
                    std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack,
                    hot::commons::DiscriminativeBit const &significantKeyInformation,
                    unsigned int insertDepth, unsigned int leafDepth, HOTSingleThreadedChildPointer const &valueToInsert
            ) {
                HOTSingleThreadedInsertStackEntry const &insertStackEntry = insertStack[insertDepth];

                insertStackEntry.mChildPointer->executeForSpecificNodeType(false,
                                                                           [&](auto const &existingNode) -> void {
                                                                               uint32_t entryIndex = insertStackEntry.mSearchResultForInsert.mEntryIndex;
                                                                               hot::commons::InsertInformation const &insertInformation = existingNode.getInsertInformation(
                                                                                       entryIndex,
                                                                                       significantKeyInformation
                                                                               );

                                                                               //As entryMask hasOnly a single bit set insertInformation.mAffectedSubtreeMask == entryMask checks whether the entry bit is the only bit set in the affectedSubtreeMask
                                                                               bool isSingleEntry = (
                                                                                       insertInformation.getNumberEntriesInAffectedSubtree() ==
                                                                                       1);
                                                                               unsigned int nextInsertDepth =
                                                                                       insertDepth + 1u;

                                                                               bool isLeafEntry = (nextInsertDepth ==
                                                                                                   leafDepth);

                                                                               if (isSingleEntry & isLeafEntry) {
                                                                                   HOTSingleThreadedChildPointer const &leafEntry = *insertStack[nextInsertDepth].mChildPointer;
                                                                                   //in case the current partition is a leaf partition add it to the partition
                                                                                   //otherwise create new leaf partition containing the existing leaf node and the new value
                                                                                   this->integrateBiNodeIntoTree(
                                                                                           insertStack,
                                                                                           nextInsertDepth,
                                                                                           hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(
                                                                                                   insertInformation.mKeyInformation,
                                                                                                   leafEntry,
                                                                                                   valueToInsert
                                                                                           ), true);
                                                                               } else if (isSingleEntry) { //in this case the single entry is a boundary node -> insert the value into the child partition
                                                                                   insertStack[nextInsertDepth].mChildPointer->executeForSpecificNodeType(
                                                                                           false,
                                                                                           [&](auto &childPartition) -> void {
                                                                                               this->insertNewValueResultingInNewPartitionRoot(
                                                                                                       childPartition,
                                                                                                       insertStack,
                                                                                                       significantKeyInformation,
                                                                                                       nextInsertDepth,
                                                                                                       valueToInsert);
                                                                                           });
                                                                               } else {
                                                                                   this->insertNewValue(existingNode,
                                                                                                        insertStack,
                                                                                                        insertInformation,
                                                                                                        insertDepth,
                                                                                                        valueToInsert);
                                                                               }
                                                                           });
            }

            inline bool insertWithInsertStack(std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack,
                                              unsigned int leafDepth,
                                              ValueType const &existingKey, uint8_t const *newKeyBytes,
                                              ValueType const &newValue, TIDType &tid) {
                auto const &existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(
                        idx::contenthelpers::toBigEndianByteOrder(existingKey));
                uint8_t const *existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);
                return hot::commons::executeForDiffingKeys(existingKeyBytes, newKeyBytes,
                                                           idx::contenthelpers::getMaxKeyLength<ValueType>(),
                                                           [&](hot::commons::DiscriminativeBit const &significantKeyInformation) {
                                                               unsigned int insertDepth = 0;
                                                               //searches for the node to insert the new value into.
                                                               //Be aware that this can result in a false positive. Therefor in case only a single entry is affected and it has a child node it must be inserted into the child node
                                                               //this is an alternative approach to using getLeastSignificantDiscriminativeBitForEntry
                                                               while (significantKeyInformation.mAbsoluteBitIndex >
                                                                      insertStack[insertDepth +
                                                                                  1].mSearchResultForInsert.mMostSignificantBitIndex) {
                                                                   ++insertDepth;
                                                               }
                                                               //this is ensured because mMostSignificantDiscriminativeBitIndex is set to MAX_INT16 for the leaf entry
                                                               assert(insertDepth < leafDepth);

                                                               HOTSingleThreadedChildPointer valueToInsert(
                                                                       valueToTID(newValue, tid));
                                                               insertNewValueIntoNode(insertStack,
                                                                                      significantKeyInformation,
                                                                                      insertDepth, leafDepth,
                                                                                      valueToInsert);
                                                           }, [&]() {
                            tidList[insertStack[leafDepth].mChildPointer->getTid()].push_back(tid);
                        });
            }

            /**
             * Executes an upsert for the given value.
             * If the index does not contain a value for the value's key, the upsert operation executes an insert.
             * It the index already contains a value for the value's key, this previously contained value is replaced and returned
             *
             * @param newValue the value to upsert.
             * @return the value of a previously contained value for the same key or an invalid result otherwise
             */
            /*inline idx::contenthelpers::OptionalValue<ValueType> upsert(ValueType newValue) {
                KeyType newKey = extractKey(newValue);
                auto const &fixedSizeKey = idx::contenthelpers::toFixSizedKey(
                        idx::contenthelpers::toBigEndianByteOrder(extractKey(newValue)));
                uint8_t const *keyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);

                if (isRootANode()) {
                    std::array<HOTSingleThreadedInsertStackEntry, 64> insertStack;
                    unsigned int leafDepth = searchForInsert(keyBytes, insertStack);
                    intptr_t tid = insertStack[leafDepth].mChildPointer->getTid();
                    ValueType const &existingValue = idx::contenthelpers::tidToValue<ValueType>(tid);

                    if (insertWithInsertStack(insertStack, leafDepth, extractKey(existingValue), keyBytes, newValue)) {
                        return idx::contenthelpers::OptionalValue<ValueType>();
                    } else {
                        *insertStack[leafDepth].mChildPointer = HOTSingleThreadedChildPointer(
                                idx::contenthelpers::valueToTID(newValue));
                        return idx::contenthelpers::OptionalValue<ValueType>(true, existingValue);;
                    }
                } else if (mRoot.isLeaf()) {
                    ValueType existingValue = idx::contenthelpers::tidToValue<ValueType>(mRoot.getTid());
                    if (idx::contenthelpers::contentEquals(extractKey(existingValue), newKey)) {
                        mRoot = HOTSingleThreadedChildPointer(idx::contenthelpers::valueToTID(newValue));
                        return {true, existingValue};
                    } else {
                        insert(newValue);
                        return {};
                    }
                } else {
                    mRoot = HOTSingleThreadedChildPointer(idx::contenthelpers::valueToTID(newValue));
                    return {};
                }
            }*/

            /**
             * @return an iterator to the first value according to the key order.
             */
            inline const_iterator begin() {
                return isEmpty() ? END_ITERATOR : const_iterator(this);
            }

            /**
             * @return an iterator which is positioned after the last element.
             */
            inline const_iterator end() const {
                return END_ITERATOR;
            }

            /**
             * searches an entry for the given key. In case an entry is found, an iterator for this entry is returned.
             * If no matching entry is found the { @link #end() } iterator is returned.
             *
             * @param searchKey the key to search a matching entry for
             * @return either an iterator pointing to the matching entry or the end iterator
             */
            inline const_iterator find(ValueType const &searchKey) const {
                return isRootANode() ? findForNonEmptyTrie(searchKey) : END_ITERATOR;
            }

        private:
            inline const_iterator findForNonEmptyTrie(ValueType const &searchKey) const {
                HOTSingleThreadedChildPointer const *current = &mRoot;

                auto const &fixedSizedSearchKey = idx::contenthelpers::toFixSizedKey(
                        idx::contenthelpers::toBigEndianByteOrder(searchKey));
                uint8_t const *searchKeyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizedSearchKey);

                const_iterator it(this, current, current + 1);
                while (!current->isLeaf()) {
                    current = it.descend(current->executeForSpecificNodeType(true, [&](auto &node) {
                        return node.search(searchKeyBytes);
                    }), current->getNode()->end());
                }

                auto const &leafValue = tidToValue(current->getTid());

                return idx::contenthelpers::contentEquals(extractKey(leafValue), searchKey) ? it : END_ITERATOR;
            }

        public:
            /**
             * returns an iterator to the first entry which has a key, which is not smaller than the given search key.
             * This is either an iterator to the matching entry itself or the first value contained in the index which has a key which is larger than the search key.
             *
             * @param searchKey the search key to determine the lower bound for
             * @return either the first entry which has a key, which is not smaller than the given search key or the end iterator if no entry fullfills the lower bound condition
             */
            inline const_iterator lower_bound(ValueType const &searchKey) {
                return lower_or_upper_bound(searchKey, true);
            }

            /**
             * returns an iterator to the first entry which has a key which is larger than the given search key
             *
             * @param searchKey the search key to determine the upper bound for
             * @return either the first entry which has a key larger than the search key or the end iterator if no entry fulfills the uper bound condition
             */
            inline const_iterator upper_bound(ValueType const &searchKey) {
                return lower_or_upper_bound(searchKey, false);
            }

        private:
            inline const_iterator lower_or_upper_bound(ValueType const &searchKey, bool is_lower_bound = true) {
                if (isEmpty()) {
                    return END_ITERATOR;
                }

                const_iterator it(this, &this->mRoot, &this->mRoot + 1);

                if (mRoot.isLeaf()) {
                    auto const &existingValue = tidToValue(mRoot.getTid());
                    auto const &existingKey = extractKey(existingValue);

                    return (idx::contenthelpers::contentEquals(searchKey, existingKey) ||
                            existingKey < searchKey) ? it : END_ITERATOR;
                } else {
                    auto const &fixedSizeKey = idx::contenthelpers::toFixSizedKey(
                            idx::contenthelpers::toBigEndianByteOrder(searchKey));
                    uint8_t const *keyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);

                    HOTSingleThreadedChildPointer const *current = &mRoot;
                    std::array<uint16_t, 64> mostSignificantBitIndexes{};

                    while (!current->isLeaf()) {
                        current = it.descend(current->executeForSpecificNodeType(true, [&](auto &node) {
                            mostSignificantBitIndexes[it.mCurrentDepth] = node.mDiscriminativeBitsRepresentation.mMostSignificantDiscriminativeBitIndex;
                            return node.search(keyBytes);
                        }), current->getNode()->end());
                    }

                    auto const &existingValue = *it;
                    auto const &existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(
                            idx::contenthelpers::toBigEndianByteOrder(extractKey(existingValue)));
                    uint8_t const *existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);

                    bool keysDiff = hot::commons::executeForDiffingKeys(existingKeyBytes, keyBytes,
                                                                        idx::contenthelpers::getMaxKeyLength<ValueType>(),
                                                                        [&](hot::commons::DiscriminativeBit const &significantKeyInformation) {
                                                                            //searches for the node to insert the new value into.
                                                                            //Be aware that this can result in a false positive. Therefor in case only a single entry is affected and it has a child node it must be inserted into the child node
                                                                            //this is an alternative approach to using getLeastSignificantDiscriminativeBitForEntry
                                                                            HOTSingleThreadedChildPointer const *child = it.mNodeStack[it.mCurrentDepth].getCurrent();
                                                                            unsigned int entryIndex = child -
                                                                                                      it.mNodeStack[--it.mCurrentDepth].getCurrent()->getNode()->getPointers();
                                                                            while (it.mCurrentDepth > 0 &&
                                                                                   significantKeyInformation.mAbsoluteBitIndex <
                                                                                   mostSignificantBitIndexes[it.mCurrentDepth]) {
                                                                                child = it.mNodeStack[it.mCurrentDepth].getCurrent();
                                                                                entryIndex = child -
                                                                                             it.mNodeStack[--it.mCurrentDepth].getCurrent()->getNode()->getPointers();
                                                                            }

                                                                            HOTSingleThreadedChildPointer const *currentNode = it.mNodeStack[it.mCurrentDepth].getCurrent();
                                                                            currentNode->executeForSpecificNodeType(
                                                                                    false,
                                                                                    [&](auto const &existingNode) -> void {
                                                                                        hot::commons::InsertInformation const &insertInformation = existingNode.getInsertInformation(
                                                                                                entryIndex,
                                                                                                significantKeyInformation);

                                                                                        unsigned int nextEntryIndex = insertInformation.mKeyInformation.mValue
                                                                                                                      ? (insertInformation.getFirstIndexInAffectedSubtree() +
                                                                                                                         insertInformation.getNumberEntriesInAffectedSubtree())
                                                                                                                      : insertInformation.getFirstIndexInAffectedSubtree();

                                                                                        HOTSingleThreadedChildPointer const *nextEntry =
                                                                                                existingNode.getPointers() +
                                                                                                nextEntryIndex;
                                                                                        HOTSingleThreadedChildPointer const *endPointer = existingNode.end();

                                                                                        it.descend(
                                                                                                nextEntry,
                                                                                                endPointer);

                                                                                        if (nextEntry ==
                                                                                            endPointer) {
                                                                                            ++it;
                                                                                        } else {
                                                                                            it.descend();
                                                                                        }
                                                                                    });
                                                                        });

                    if (!keysDiff && !is_lower_bound) {
                        ++it;
                    }

                    return it;
                }
            }

        public:
            /**
             * helper function for debuggin purposes only, which for a given path returns the child pointer stored at this location.
             * A path to a childpointer consists of a top down ordered list of indexes, where each index determines the position of the pointer to follow.
             * The first index is applied to the root node, the next to the resulting first level node and so forth.
             *
             * @param path the sequence of indexes encoding the path to the corresponding child pointer
             * @return the matching child pointer of an unused node-type childpointer
             */
            inline HOTSingleThreadedChildPointer getNodeAtPath(std::initializer_list<unsigned int> path) {
                HOTSingleThreadedChildPointer current = mRoot;
                for (unsigned int entryIndex: path) {
                    assert(!current.isLeaf());
                    current = current.getNode()->getPointers()[entryIndex];
                }
                return current;
            }

            /**
             * collects a set of statistics for this index instance.
             * The first entry in the resulting pair contains the total size of the index in bytes.
             * The second entry contains a map of statistical values containing the following entries:
             *
             * - height: the overall height of the tree
             * - leafNodesOnDepth_<current_depth> how many leaf entries are contained in nodes on the current_depth
             * - leafNodesOnBinaryDepth_<current_binary_depth> ho many leaf entries would be contained in an equivalent binary patricia trie on depth current_binary_depth
             * - numberValues the overall number of values stored
             * - <NODE_TYPE>: the number of nodes of type <NODE_TYPE> possible values for node type are:
             * 		+ SINGLE_MASK_8_BIT_PARTIAL_KEYS
             * 		+ SINGLE_MASK_16_BIT_PARTIAL_KEYS
             * 		+ SINGLE_MASK_32_BIT_PARTIAL_KEYS
             * 		+ MULTI_MASK_8_BYTES_AND_8_BIT_PARTIAL_KEYS
             * 		+ MULTI_MASK_8_BYTES_AND_16_BIT_PARTIAL_KEYS
             * 		+ MULTI_MASK_8_BYTES_AND_32_BIT_PARTIAL_KEYS
             * 		+ MULTI_MASK_16_BYTES_AND_16_BIT_PARTIAL_KEYS
             * 		+ MULTI_MASK_32_BYTES_AND_32_BIT_PARTIAL_KEYS
             * 	- numberAllocations the actual number of allocations which where executd by the underlying memory pool
             *
             * @return the collected statistical values
             */
            std::pair<size_t, std::map<std::string, double>> getStatistics() const {
                std::map<size_t, size_t> leafNodesPerDepth;
                getValueDistribution(mRoot, 0, leafNodesPerDepth);

                std::map<size_t, size_t> leafNodesPerBinaryDepth;
                getBinaryTrieValueDistribution(mRoot, 0, leafNodesPerBinaryDepth);

                std::map<std::string, double> statistics;
                statistics["height"] = mRoot.getHeight();
                statistics["numberAllocations"] = HOTSingleThreadedNodeBase::getNumberAllocations();

                size_t overallLeafNodeCount = 0;
                for (auto leafNodesOnDepth: leafNodesPerDepth) {
                    std::string statisticsKey{"leafNodesOnDepth_"};
                    std::string levelString = std::to_string(leafNodesOnDepth.first);
                    statisticsKey += std::string(2 - levelString.length(), '0') + levelString;
                    statistics[statisticsKey] = leafNodesOnDepth.second;
                    overallLeafNodeCount += leafNodesOnDepth.second;
                }

                for (auto leafNodesOnBinaryDepth: leafNodesPerBinaryDepth) {
                    std::string statisticsKey{"leafNodesOnBinaryDepth_"};
                    std::string levelString = std::to_string(leafNodesOnBinaryDepth.first);
                    statisticsKey += std::string(3 - levelString.length(), '0') + levelString;
                    statistics[statisticsKey] = leafNodesOnBinaryDepth.second;
                }

                statistics["numberValues"] = overallLeafNodeCount;
                collectStatsForSubtree(mRoot, statistics);

                size_t totalSize = statistics["total"];
                statistics.erase("total");

                return {totalSize, statistics};
            }

        private:
            inline void collectStatsForSubtree(HOTSingleThreadedChildPointer const &subTreeRoot,
                                               std::map<std::string, double> &stats) const {
                if (!subTreeRoot.isLeaf()) {
                    subTreeRoot.executeForSpecificNodeType(true, [&, this](auto &node) -> void {
                        std::string nodeType = nodeAlgorithmToString(node.mNodeType);
                        stats["total"] += node.getNodeSizeInBytes();
                        stats[nodeType] += 1.0;
                        for (HOTSingleThreadedChildPointer const &childPointer: node) {
                            this->collectStatsForSubtree(childPointer, stats);
                        }
                    });
                }
            }

            /**
             * root has depth 0
             * first Level has depth 1...
             * @param leafNodesPerDepth an output parameter for collecting the number of values aggregated by depth
             * @param currentDepth the current depth to process
             */
            inline void getValueDistribution(HOTSingleThreadedChildPointer const &childPointer, size_t depth,
                                             std::map<size_t, size_t> &leafNodesPerDepth) const {
                if (childPointer.isLeaf()) {
                    ++leafNodesPerDepth[depth];
                } else {
                    for (HOTSingleThreadedChildPointer const &pointer: (*childPointer.getNode())) {
                        getValueDistribution(pointer, depth + 1, leafNodesPerDepth);
                    }
                }
            }

            /**
             * root has depth 0
             * first Level has depth 1...
             * @param leafNodesPerDepth an output parameter for collecting the number of values aggregated by depth in a virtual cobtrie
             * @param currentDepth the current depth to process
             */
            inline void
            getBinaryTrieValueDistribution(HOTSingleThreadedChildPointer const &childPointer, size_t binaryTrieDepth,
                                           std::map<size_t, size_t> &leafNodesPerDepth) const {
                if (childPointer.isLeaf()) {
                    ++leafNodesPerDepth[binaryTrieDepth];
                } else {
                    childPointer.executeForSpecificNodeType(true, [&, this](auto &node) {
                        std::array<uint8_t, 32> binaryEntryDepthsInNode = node.getEntryDepths();
                        size_t i = 0;
                        for (HOTSingleThreadedChildPointer const &pointer: node) {
                            this->getBinaryTrieValueDistribution(pointer, binaryTrieDepth + binaryEntryDepthsInNode[i],
                                                                 leafNodesPerDepth);
                            ++i;
                        }
                    });
                }
            }

            bool hasTheSameKey(intptr_t tid, ValueType const &key) {
                auto const &storedKey = extractKey(tidToValue(tid));
                //TODO: contentEquals without tid?
                return idx::contenthelpers::contentEquals(storedKey, key);
            }

            void removeWithStack(std::array<HOTSingleThreadedInsertStackEntry, 64> const &searchStack,
                                 unsigned int currentDepth) {
                removeAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(searchStack, currentDepth,
                                                                                     determineDeletionInformation(
                                                                                             searchStack, currentDepth),
                                                                                     [](HOTSingleThreadedChildPointer const &newNode,
                                                                                        size_t /* offset */) {
                                                                                         return newNode;
                                                                                     });
            }

            void removeRecurseUp(std::array<HOTSingleThreadedInsertStackEntry, 64> const &searchStack,
                                 unsigned int currentDepth,
                                 HOTSingleThreadedDeletionInformation const &deletionInformation,
                                 HOTSingleThreadedChildPointer const &replacement) {
                if (deletionInformation.getContainingNode().getNumberEntries() == 2) {
                    HOTSingleThreadedChildPointer previous = *searchStack[currentDepth].mChildPointer;
                    *searchStack[currentDepth].mChildPointer = replacement;
                    previous.free();
                } else {
                    removeAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(searchStack, currentDepth,
                                                                                         deletionInformation,
                                                                                         [&](HOTSingleThreadedChildPointer const &newNode,
                                                                                             size_t offset) {
                                                                                             newNode.getNode()->getPointers()[
                                                                                                     offset +
                                                                                                     deletionInformation.getIndexOfEntryToReplace()] = replacement;
                                                                                             return newNode;
                                                                                         });
                }
            }

            template<typename Operation>
            void removeAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(
                    std::array<HOTSingleThreadedInsertStackEntry, 64> const &searchStack, unsigned int currentDepth,
                    HOTSingleThreadedDeletionInformation const &deletionInformation, Operation const &operation
            ) {
                HOTSingleThreadedChildPointer *current = searchStack[currentDepth].mChildPointer;
                bool isRoot = currentDepth == 0;
                if (isRoot) {
                    removeEntryAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(current,
                                                                                              deletionInformation,
                                                                                              operation);
                } else {
                    unsigned int parentDepth = currentDepth - 1;
                    HOTSingleThreadedDeletionInformation const &parentDeletionInformation = determineDeletionInformation(
                            searchStack, parentDepth);
                    bool hasDirectNeighbour = parentDeletionInformation.hasDirectNeighbour();
                    if (hasDirectNeighbour) {
                        HOTSingleThreadedChildPointer *potentialDirectNeighbour = parentDeletionInformation.getDirectNeighbourIfAvailable();
                        if ((potentialDirectNeighbour->getHeight() == current->getHeight())) {
                            size_t totalNumberEntries =
                                    potentialDirectNeighbour->getNumberEntries() + current->getNumberEntries() - 1;
                            if (totalNumberEntries <= hot::commons::MAXIMUM_NUMBER_NODE_ENTRIES) {
                                HOTSingleThreadedNodeBase *parentNode = searchStack[parentDepth].mChildPointer->getNode();
                                HOTSingleThreadedChildPointer left = parentNode->getPointers()[parentDeletionInformation.getAffectedBiNode().mLeft.mFirstIndexInRange];
                                HOTSingleThreadedChildPointer right = parentNode->getPointers()[parentDeletionInformation.getAffectedBiNode().mRight.mFirstIndexInRange];
                                HOTSingleThreadedChildPointer mergedNode = operation(
                                        mergeNodesAndRemoveEntryIfPossible(
                                                parentDeletionInformation.getAffectedBiNode().mDiscriminativeBitIndex,
                                                left,
                                                right,
                                                deletionInformation,
                                                parentDeletionInformation.getDiscriminativeBitValueForEntry()
                                        ),
                                        //offset in case the deleted entry is in the right side
                                        left.getNumberEntries() *
                                        parentDeletionInformation.getDiscriminativeBitValueForEntry()
                                );
                                assert(!mergedNode.isUnused() && mergedNode.isNode());
                                removeRecurseUp(searchStack, parentDepth, parentDeletionInformation, mergedNode);
                                left.free();
                                right.free();
                            } else {
                                removeEntryAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(current,
                                                                                                          deletionInformation,
                                                                                                          operation);
                            }
                        } else if ((potentialDirectNeighbour->getHeight() < current->getHeight())) {
                            //this is required in case for the creation of this tree a node split happened, resulting in a link to a leaf or a node of smaller height
                            //move directNeighbour into current and remove
                            //removeAndAdd
                            hot::commons::DiscriminativeBit keyInformation(
                                    parentDeletionInformation.getAffectedBiNode().mDiscriminativeBitIndex,
                                    !parentDeletionInformation.getDiscriminativeBitValueForEntry());
                            HOTSingleThreadedChildPointer previousNode = *current;
                            HOTSingleThreadedChildPointer newNode = operation(
                                    current->executeForSpecificNodeType(false, [&](auto const &currentNode) {
                                        return currentNode.removeAndAddEntry(deletionInformation, keyInformation,
                                                                             *potentialDirectNeighbour);
                                    }), parentDeletionInformation.getDiscriminativeBitValueForEntry());
                            removeRecurseUp(searchStack, parentDepth, parentDeletionInformation, newNode);
                            previousNode.free();
                        } else {
                            removeEntryAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(current,
                                                                                                      deletionInformation,
                                                                                                      operation);
                        }
                    } else {
                        removeEntryAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(current,
                                                                                                  deletionInformation,
                                                                                                  operation);
                    }
                }
            }

            template<typename Operation>
            static void removeEntryAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(
                    HOTSingleThreadedChildPointer *const currentNodePointer,
                    HOTSingleThreadedDeletionInformation const &deletionInformation, Operation const &operation
            ) {
                HOTSingleThreadedChildPointer previous = *currentNodePointer;
                *currentNodePointer = operation(
                        currentNodePointer->executeForSpecificNodeType(false, [&](auto const &currentNode) {
                            return currentNode.removeEntry(deletionInformation);
                        }),
                        0
                );
                previous.free();
            }

            HOTSingleThreadedDeletionInformation determineDeletionInformation(
                    const std::array<HOTSingleThreadedInsertStackEntry, 64> &searchStack, unsigned int currentDepth) {
                HOTSingleThreadedInsertStackEntry const &currentEntry = searchStack[currentDepth];
                uint32_t indexOfEntryToRemove = currentEntry.mSearchResultForInsert.mEntryIndex;
                return currentEntry.mChildPointer->executeForSpecificNodeType(false, [&](auto const &currentNode) {
                    return currentNode.getDeletionInformation(indexOfEntryToRemove);
                });

            }

        public:
            /**
             * @return the overall tree height
             */
            size_t getHeight() const {
                return isEmpty() ? 0 : mRoot.getHeight();

            }

        private:
            template<typename NodeType>
            inline void insertNewValue(
                    NodeType const &existingNode, std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack,
                    hot::commons::InsertInformation const &insertInformation, unsigned int insertDepth,
                    HOTSingleThreadedChildPointer const &valueToInsert
            ) {
                HOTSingleThreadedInsertStackEntry const &insertStackEntry = insertStack[insertDepth];

                if (!existingNode.isFull()) {
                    HOTSingleThreadedChildPointer newNodePointer = existingNode.addEntry(insertInformation,
                                                                                         valueToInsert);
                    *(insertStackEntry.mChildPointer) = newNodePointer;
                    delete &existingNode;
                } else {
                    assert(insertInformation.mKeyInformation.mAbsoluteBitIndex !=
                           insertStackEntry.mSearchResultForInsert.mMostSignificantBitIndex);
                    if (insertInformation.mKeyInformation.mAbsoluteBitIndex >
                        insertStackEntry.mSearchResultForInsert.mMostSignificantBitIndex) {
                        hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = existingNode.split(
                                insertInformation, valueToInsert);
                        integrateBiNodeIntoTree(insertStack, insertDepth, binaryNode, true);
                        delete &existingNode;
                    } else {
                        hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(
                                insertInformation.mKeyInformation, *insertStackEntry.mChildPointer, valueToInsert);
                        integrateBiNodeIntoTree(insertStack, insertDepth, binaryNode, true);
                    }
                }
            }

            template<typename NodeType>
            inline void insertNewValueResultingInNewPartitionRoot(
                    NodeType const &existingNode, std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack,
                    const hot::commons::DiscriminativeBit &keyInformation,
                    unsigned int insertDepth, HOTSingleThreadedChildPointer const &valueToInsert
            ) {
                HOTSingleThreadedInsertStackEntry const &insertStackEntry = insertStack[insertDepth];
                if (!existingNode.isFull()) {
                    //As the insert results in a new partition root, no prefix bits are set and all entries in the partition are affected
                    hot::commons::InsertInformation insertInformation{0, 0,
                                                                      static_cast<uint32_t>(existingNode.getNumberEntries()),
                                                                      keyInformation};
                    *(insertStackEntry.mChildPointer) = existingNode.addEntry(insertInformation, valueToInsert);
                    delete &existingNode;
                } else {
                    assert(keyInformation.mAbsoluteBitIndex !=
                           insertStackEntry.mSearchResultForInsert.mMostSignificantBitIndex);
                    hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(
                            keyInformation, *insertStackEntry.mChildPointer, valueToInsert);
                    integrateBiNodeIntoTree(insertStack, insertDepth, binaryNode, true);
                }
            }
        };

        template<typename ValueType, typename TIDType>
        class HOTSingleThreadedPayloadIterator {
            friend struct HOTSingleThreadedPayload<ValueType, TIDType>;
            using iterator = HOTSingleThreadedPayloadIterator<ValueType, TIDType>;
            static HOTSingleThreadedChildPointer END_TOKEN;
            using ResultType = typename HOTSingleThreadedPayload<ValueType, TIDType>::ResultType;

            alignas(std::alignment_of<HOTSingleThreadedIteratorStackEntry>()) char mRawNodeStack[
                    sizeof(HOTSingleThreadedIteratorStackEntry) * 64];
            HOTSingleThreadedIteratorStackEntry *mNodeStack;
            size_t mCurrentDepth = 0;
            HOTSingleThreadedPayload<ValueType, TIDType> *const hot;

        public:
            HOTSingleThreadedPayloadIterator(HOTSingleThreadedPayload<ValueType, TIDType> *const hot)
                    : HOTSingleThreadedPayloadIterator(hot, &hot->mRoot, &hot->mRoot + 1) {
                descend();
            }

            HOTSingleThreadedPayloadIterator(HOTSingleThreadedPayloadIterator const &other) : mNodeStack(
                    reinterpret_cast<HOTSingleThreadedIteratorStackEntry *>(mRawNodeStack)), hot(other.hot) {
                std::memcpy(this->mRawNodeStack, other.mRawNodeStack,
                            sizeof(HOTSingleThreadedIteratorStackEntry) * (other.mCurrentDepth + 1));
                mCurrentDepth = other.mCurrentDepth;
            }

            HOTSingleThreadedPayloadIterator() : mNodeStack(
                    reinterpret_cast<HOTSingleThreadedIteratorStackEntry *>(mRawNodeStack)), hot(
                    nullptr) {
                mNodeStack[0].init(&END_TOKEN, &END_TOKEN);
            }

        public:
            ResultType &operator*() {
                return hot->tidToValue(mNodeStack[mCurrentDepth].getCurrent()->getTid());
            }

            iterator &operator++() {
                mNodeStack[mCurrentDepth].advance();
                while (mCurrentDepth > 0 && mNodeStack[mCurrentDepth].isExhausted()) {
                    --mCurrentDepth;
                    mNodeStack[mCurrentDepth].advance();
                }
                if (mNodeStack[0].isExhausted()) {
                    mNodeStack[0].init(&END_TOKEN, &END_TOKEN);
                } else {
                    descend();
                }
                return *this;
            }

            bool operator==(iterator const &other) const {
                return *mNodeStack[mCurrentDepth].getCurrent() ==
                       *other.mNodeStack[other.mCurrentDepth].getCurrent();
            }

            bool operator!=(iterator const &other) const {
                return *mNodeStack[mCurrentDepth].getCurrent() !=
                       *other.mNodeStack[other.mCurrentDepth].getCurrent();
            }

        private:
            HOTSingleThreadedPayloadIterator(HOTSingleThreadedPayload<ValueType, TIDType> *const hot,
                                             HOTSingleThreadedChildPointer const *currentRoot,
                                             HOTSingleThreadedChildPointer const *rootEnd) : mNodeStack(
                    reinterpret_cast<HOTSingleThreadedIteratorStackEntry *>(mRawNodeStack)), hot(hot) {
                mNodeStack[0].init(currentRoot, rootEnd);
            }

            void descend() {
                HOTSingleThreadedChildPointer const *currentSubtreeRoot = mNodeStack[mCurrentDepth].getCurrent();
                while (currentSubtreeRoot->isAValidNode()) {
                    HOTSingleThreadedNodeBase *currentSubtreeRootNode = currentSubtreeRoot->getNode();
                    currentSubtreeRoot = descend(currentSubtreeRootNode->begin(), currentSubtreeRootNode->end());
                }
            }

            HOTSingleThreadedChildPointer const *
            descend(HOTSingleThreadedChildPointer const *current, HOTSingleThreadedChildPointer const *end) {
                return mNodeStack[++mCurrentDepth].init(current, end);
            }
        };

        template<typename ValueType, typename TIDType> HOTSingleThreadedChildPointer HOTSingleThreadedPayloadIterator<ValueType, TIDType>::END_TOKEN;
    }
}