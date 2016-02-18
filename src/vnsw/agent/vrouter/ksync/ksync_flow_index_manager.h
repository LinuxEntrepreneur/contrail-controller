/*
 * Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
 */
#ifndef __VNSW_AGENT_VROUTER_KSYNC_KSYNC_FLOW_INDEX_MANAGER_H__
#define __VNSW_AGENT_VROUTER_KSYNC_KSYNC_FLOW_INDEX_MANAGER_H__

#include <tbb/mutex.h>
#include <pkt/flow_entry.h>

////////////////////////////////////////////////////////////////////////////
// The module is responsible to manage assignment of vrouter flow-table index
// to the flow.
//
// The module maintains following information,
//
// KSyncFlowIndexManager::IndexTree
//     Common table containing information about which flow owns the index.
//     Each entry in tree is of type KSyncFlowIndexManager::IndexEntry.
//     It contains,
//     owner_      : Flow owning the index
//     wait_entry_ : Flow waiting for index to be available.
//                   VRouter does not evict flows in Hold state. Hence, the
//                   assumtion is, there can atmost be one flow waiting
//                   on an index
//
// KSyncFlowIndexEntry
//     Entry allocated per flow-entry. Important fields are,
//     index_       : The flow index seen by index manager
//     own_index_   : Does the flow own index_
//     state_       : State for the flow (Documented below)
//     evicted_     : Flow evicted and index_ is not owned by flow anymore
////////////////////////////////////////////////////////////////////////////
class FlowEntry;
class FlowProto;
class KSyncFlowIndexManager;
class FlowTableKSyncObject;

class KSyncFlowIndexEntry {
public:
    enum State {
        //  Initial state for entry on creation. A temporary state
        //  index_ set to -1, flow_handle_ may be -1 or a valid index
        INIT,
        //  Index allocated for the flow. This is steady state with
        //  flow_handle and index_ having same values
        INDEX_SET,
        // Index changed for a flow. The old index is in process of eviction
        // and new index allocated present in flow_handle_ will be acquired
        // after eviction
        INDEX_CHANGE,
        // Flow evicted. Waiting for the states to be cleaned up
        INDEX_EVICT,
        // Index not yet assigned for the flow. A Flow add message is sent
        // to VRouter
        INDEX_UNASSIGNED,
        // Failed to get index from vrouter will happen for reverse flow only
        INDEX_FAILED,
        INVALID
    };

    enum Event {
        // Add new flow. Most likely generated by flow-add request from VRouter
        ADD,
        // Flow attributes changed. Does not handle index-change
        CHANGE,
        // Delete a flow
        DELETE,
        // VRouter assigned an index to the flow
        INDEX_ASSIGN,
        // VRouter reported error for the flow
        VROUTER_ERROR,
        // KSync entry for flow is deleted
        KSYNC_FREE,
        INVALID_EVENT
    };

    KSyncFlowIndexEntry();
    virtual ~KSyncFlowIndexEntry();
    void Reset();

    uint32_t index() const { return index_; }
    FlowEntry *index_owner() const { return index_owner_.get(); }
    FlowTableKSyncEntry *ksync_entry() const { return ksync_entry_; }
    State state() const { return state_; }
    bool evicted() const { return evicted_; }
    bool skip_delete() const { return skip_delete_; }
    uint32_t evict_count() const { return evict_count_; }

    void HandleEvent(KSyncFlowIndexManager *manager, FlowEntry *flow,
                     Event event, uint32_t index);
    void EvictFlow(KSyncFlowIndexManager *manager, FlowEntry *flow,
                   State next_state, bool skip_del);
    void InitSm(KSyncFlowIndexManager *manager, FlowEntry *flow, Event event,
                uint32_t index);
    void IndexSetSm(KSyncFlowIndexManager *manager, FlowEntry *flow,
                    Event event, uint32_t index);
    void IndexChangeSm(KSyncFlowIndexManager *manager, FlowEntry *flow,
                       Event event, uint32_t index);
    void IndexEvictSm(KSyncFlowIndexManager *manager, FlowEntry *flow,
                      Event event, uint32_t index);
    void IndexUnassignedSm(KSyncFlowIndexManager *manager, FlowEntry *flow,
                           Event event, uint32_t index);
    void IndexFailedSm(KSyncFlowIndexManager *manager, FlowEntry *flow,
                       Event event, uint32_t index);
    void KSyncAddChange(KSyncFlowIndexManager *manager, FlowEntry *flow);
    void KSyncDelete(KSyncFlowIndexManager *manager, FlowEntry *flow);
    void KSyncUpdateFlowHandle(KSyncFlowIndexManager *manager, FlowEntry *flow);
    void Log(KSyncFlowIndexManager *manager, FlowEntry *flow, Event event,
             uint32_t index);
    void EvictLog(KSyncFlowIndexManager *manager, FlowEntry *flow,
                  uint32_t index);
private:
    void AcquireIndex(KSyncFlowIndexManager *manager, FlowEntry *flow,
                      uint32_t index);
    friend class KSyncFlowIndexManager;

    State state_;
    // Index currently owned by the flow
    uint32_t index_;
    FlowTableKSyncEntry *ksync_entry_;
    // Flow owning index_
    FlowEntryPtr index_owner_;
    // Is flow evicted
    bool evicted_;
    // Skip sending delete message
    bool skip_delete_;
    // Number of times flow is evicted
    uint32_t evict_count_;
    // Delete initiated for the flow
    bool delete_in_progress_;
};

class KSyncFlowIndexManager {
public:
    struct IndexEntry {
        IndexEntry() : owner_(NULL), wait_entry_() { }
        virtual ~IndexEntry() {
            assert(owner_.get() == NULL);
            assert(wait_entry_.get() == NULL);
        }

        tbb::mutex mutex_;
        FlowEntryPtr owner_;
        FlowEntryPtr wait_entry_;
    };
    typedef std::vector<IndexEntry> IndexList;

    KSyncFlowIndexManager(KSync *ksync);
    virtual ~KSyncFlowIndexManager();
    void InitDone(uint32_t count);

    void Release(FlowEntry *flow);
    FlowEntryPtr FindByIndex(uint32_t idx);

    void Add(FlowEntry *flow);
    void Change(FlowEntry *flow);
    void Delete(FlowEntry *flow);
    void UpdateFlowHandle(FlowEntry *flow, uint32_t index);
    void UpdateKSyncError(FlowEntry *flow);
    void KSyncFree(FlowEntry *flow);

    void AcquireIndex(FlowEntry *flow, uint32_t index);
    void EvictIndex(FlowEntry *flow, uint32_t index, bool skip_del);
private:
    void HandleEvent(FlowEntry *flow, KSyncFlowIndexEntry::Event event);
    void HandleEvent(FlowEntry *flow, KSyncFlowIndexEntry::Event event,
                     uint32_t index);
    void EvictIndexUnlocked(FlowEntry *flow, uint32_t index, bool skip_del);
    void EvictRequest(FlowEntryPtr &flow, uint32_t index);

private:
    KSync *ksync_;
    FlowProto *proto_;
    uint32_t count_;
    IndexList index_list_;
};

#endif //  __VNSW_AGENT_VROUTER_KSYNC_KSYNC_FLOW_INDEX_MANAGER_H__
