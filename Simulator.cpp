//
// Created by Aweso on 11/29/2025.
//

#include "Simulator.h"

#define DEBUG_MODE false
#define DEBUG if(DEBUG_MODE) printf




void Simulator::Run() {
    do {
        Retire();
        Writeback();
        Execute();
        Issue();
        Dispatch();
        RegRead();
        Rename();
        Decode();
        Fetch();
    }while(Advance_Cycle());

}

void Simulator::Retire() {
    DEBUG("Retire\n");
    int instructions_retired = 0;
    while (!m_rob.empty() && instructions_retired < m_width) {
        for (int i = m_rob.m_head; i < m_rob.m_tail && instructions_retired < m_width; i++) {
            if (m_rob[i].ready) {
                m_rob[i].valid = 0;
                instructions_retired++;
            }
        }

    }
}


void Simulator::Writeback() {
    DEBUG("Writeback\n");
    while (!m_pipeline_wb.empty()) {
        auto instr = m_pipeline_wb.pop();
    }
}


void Simulator::Execute() {
    DEBUG("Execute\n");
    while (!m_pipeline_wb.full()) {
        auto exec = m_execute_list.GetOldest();
        if (!exec.valid) {
            break;
        }
        m_pipeline_wb.push(exec);
        // wake up
    }
}


void Simulator::Issue() {
    DEBUG("Issue\n");
    int instructions_issued = 0;
    while (!m_execute_list.full() && instructions_issued < m_width) {
        auto instr = m_iq.GetOldest();
        if (instr.valid) {
            m_execute_list.push(instr);
            instructions_issued++;
        } else {
            break;
        }

    }
}

void Simulator::Dispatch() {
    DEBUG("Dispatch\n");
    if (m_iq.available() <= m_pipeline_rr.m_element_count) {
        while (!m_pipeline_rr.empty()) {
            m_iq.push(m_pipeline_rr.pop()); //ready = meta
        }
    }
}


void Simulator::RegRead() {
    DEBUG("RegRead\n");
    if (!m_pipeline_di.empty()) {
        while (!m_pipeline_rr.empty()) {
            auto instr = m_pipeline_rr.pop();
            instr.src1_meta = instr.src1_meta ? true : m_rob.read(instr.src1).ready; // if arf, ready, else, read rob
            instr.src2_meta = instr.src2_meta ? true : m_rob.read(instr.src2).ready; // if arf, ready, else, read rob
            m_pipeline_di.push(instr);
        }
    }
}


void Simulator::Rename() {
    DEBUG("Rename\n");
    if (m_pipeline_rr.available() <= m_pipeline_rn.m_element_count && m_rob.available() >= m_pipeline_rn.m_element_count) {
        while (!m_pipeline_rn.empty()) {
            auto instr = m_pipeline_rn.pop();

            if (m_rmt[instr.src1] >= 0) {
                instr.src1 = m_rmt[instr.src1];
                instr.src1_meta = false;
            }else {
                instr.src1_meta = true; //arf
            }

            if (m_rmt[instr.src2] >= 0) {
                instr.src2 = m_rmt[instr.src2];
                instr.src2_meta = false;
            }else {
                instr.src2_meta = true; //arf
            }

            auto dst_renamed = m_rmt[instr.dst];
            if (instr.dst < 0) {
                m_rmt[instr.dst] = m_rob.getIndex();
            }

            m_rob.push({
                instr.dst,
                true,
            false,
            false,
            false,
            instr.pc});
            m_pipeline_rr.push(instr);
        }
    }
}


void Simulator::Decode() {
    DEBUG("Decode\n");
    if (m_pipeline_rn.empty()) {
        m_pipeline_rn = std::move(m_pipeline_de);
    }
}


void Simulator::Fetch() {
    DEBUG("Fetch\n");
    if (m_pipeline_de.empty() && !feof(m_trace_file)) {
        while (!m_pipeline_de.full() && !feof(m_trace_file))
            m_pipeline_de.push(Instruction(m_trace_file));
    }
    if (feof(m_trace_file)) {
        fclose(m_trace_file);
        m_done = true;
    }
}


bool Simulator::Advance_Cycle() {
    m_execute_list.Increment();
    return !m_done;
}


