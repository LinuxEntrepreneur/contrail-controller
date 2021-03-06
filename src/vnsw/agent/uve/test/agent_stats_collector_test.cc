/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#include <db/db.h>
#include <cmn/agent_cmn.h>

#include <oper/interface_common.h>
#include <oper/interface.h>
#include <oper/mirror_table.h>

#include "vr_genetlink.h"
#include "vr_interface.h"
#include "vr_types.h"
#include "nl_util.h"

#include <uve/stats_collector.h>
#include <uve/vn_uve_table.h>
#include <uve/vm_uve_table.h>
#include <init/agent_param.h>
#include <uve/test/agent_stats_collector_test.h>
#include <uve/agent_uve_stats.h>

IoContext *AgentStatsCollectorTest::AllocateIoContext(char* buf, uint32_t buf_len,
                                                      StatsType type, uint32_t seq) {
    switch (type) {
        case InterfaceStatsType:
            return (new InterfaceStatsIoContextTest(buf_len, buf, seq, 
                                         intf_stats_sandesh_ctx_.get(),
                                         IoContext::UVE_Q_ID));
            break;
       case VrfStatsType:
            return (new VrfStatsIoContextTest(buf_len, buf, seq, 
                                        vrf_stats_sandesh_ctx_.get(), 
                                        IoContext::UVE_Q_ID));
            break;
       case DropStatsType:
            return (new DropStatsIoContextTest(buf_len, buf, seq, 
                                         drop_stats_sandesh_ctx_.get(), 
                                         IoContext::UVE_Q_ID));
            break;
       default:
            return NULL;
    }
}

void InterfaceStatsIoContextTest::Handler() {
    InterfaceStatsIoContext::Handler();
    AgentStatsSandeshContext *ctx =
        static_cast<AgentStatsSandeshContext *> (sandesh_context_);
    AgentStatsCollectorTest *collector = static_cast<AgentStatsCollectorTest *>
        (ctx->agent()->stats_collector());
    collector->interface_stats_responses_++;
}

void InterfaceStatsIoContextTest::ErrorHandler(int err) {
    InterfaceStatsIoContext::ErrorHandler(err);
    AgentStatsSandeshContext *ctx =
        static_cast<AgentStatsSandeshContext *> (sandesh_context_);
    AgentStatsCollectorTest *collector = static_cast<AgentStatsCollectorTest *>
        (ctx->agent()->stats_collector());
    collector->interface_stats_errors_++;
}

void VrfStatsIoContextTest::Handler() {
    VrfStatsIoContext::Handler();
    AgentStatsSandeshContext *ctx =
        static_cast<AgentStatsSandeshContext *> (sandesh_context_);
    AgentStatsCollectorTest *collector = static_cast<AgentStatsCollectorTest *>
        (ctx->agent()->stats_collector());
    collector->vrf_stats_responses_++;
}

void VrfStatsIoContextTest::ErrorHandler(int err) {
    VrfStatsIoContext::ErrorHandler(err);
    AgentStatsSandeshContext *ctx =
        static_cast<AgentStatsSandeshContext *> (sandesh_context_);
    AgentStatsCollectorTest *collector = static_cast<AgentStatsCollectorTest *>
        (ctx->agent()->stats_collector());
    collector->vrf_stats_errors_++;
}

void DropStatsIoContextTest::Handler() {
    DropStatsIoContext::Handler();
    AgentStatsSandeshContext *ctx =
        static_cast<AgentStatsSandeshContext *> (sandesh_context_);
    AgentStatsCollectorTest *collector = static_cast<AgentStatsCollectorTest *>
        (ctx->agent()->stats_collector());
    collector->drop_stats_responses_++;
}

void DropStatsIoContextTest::ErrorHandler(int err) {
    DropStatsIoContext::ErrorHandler(err);
    AgentStatsSandeshContext *ctx =
        static_cast<AgentStatsSandeshContext *> (sandesh_context_);
    AgentStatsCollectorTest *collector = static_cast<AgentStatsCollectorTest *>
        (ctx->agent()->stats_collector());
    collector->drop_stats_errors_++;
}

void AgentStatsCollectorTest::Test_DeleteVrfStatsEntry(int vrf_id) {
    AgentUveStats *uve = static_cast<AgentUveStats *>
        (Agent::GetInstance()->uve());
    StatsManager *sm = uve->stats_manager();
    StatsManager::VrfIdToVrfStatsTree::iterator it;
    it = sm->vrf_stats_tree_.find(vrf_id);
    if (it != sm->vrf_stats_tree_.end()) {
        sm->vrf_stats_tree_.erase(it);
    }
}
