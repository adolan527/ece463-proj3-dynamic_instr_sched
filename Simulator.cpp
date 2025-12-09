//
// Created by Aweso on 11/29/2025.
//

#include "Simulator.h"

#define DO_LOG_STAGE false
#define LOG_STAGE if(DO_LOG_STAGE) printf

#define DO_CYCLE false

#define DO_LOG_FILES false


void Simulator::Run() {
    int i = 0;
    if (DO_LOG_FILES) {
        m_pipeline_di.StartLog("debug/dispatch.csv");
        m_pipeline_rn.StartLog("debug/rename.csv");
        m_pipeline_de.StartLog("debug/decode.csv");
        m_pipeline_rr.StartLog("debug/regread.csv");
        m_pipeline_wb.StartLog("debug/writeback.csv");
    }
    do {
        //printf("%d\n",i);
        if (DO_CYCLE) {
            if (i == 10)break;
            printf("%d\n",i);
            printf("\tDecode\n");
            m_pipeline_de.Print();
            printf("\tRename\n");
            m_pipeline_rn.Print();
            printf("\tRegRead\n");
            m_pipeline_rr.Print();
            printf("\tDispatch\n");
            m_pipeline_di.Print();
            printf("\tROB\n");
            m_rob.Print();
            printf("\tRMT\n");
            for (auto &r : m_rmt) {
                printf("%d, ",r);
            }
            printf("\n\tARF\n");
            for (auto &r : m_arf) {
                printf("%d, ",r);
            }
        }
        if (DO_LOG_FILES) {
            m_pipeline_wb.Log();
            m_pipeline_rr.Log();
            m_pipeline_de.Log();
            m_pipeline_rn.Log();
            m_pipeline_di.Log();
        }
        i++;

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
    if (DO_LOG_FILES) {
        m_pipeline_di.EndLog();
        m_pipeline_rn.EndLog();
        m_pipeline_de.EndLog();
        m_pipeline_rr.EndLog();
        m_pipeline_wb.EndLog();
    }
}

void Simulator::Retire() {
    LOG_STAGE("Retire\n");
    int instructions_retired = 0;
    while (instructions_retired <= m_width) {
        auto retired = m_rob.retire();
        if (!retired.valid) break;
        auto instr = m_pipeline_rt.pop();
        instr.rt_length = m_cycle_count - instr.rt_begin;
        instr.Print_Timing();
    }



}


void Simulator::Writeback() {
    LOG_STAGE("Writeback\n");
    while (!m_pipeline_wb.empty() && !m_pipeline_rt.full()) {
        auto instr = m_pipeline_wb.pop();
        instr.wb_length = m_cycle_count - instr.wb_begin;
        instr.rt_begin = m_cycle_count;
        m_pipeline_rt.push(instr);

    }
}


void Simulator::Execute() {
    LOG_STAGE("Execute\n");
    while (!m_pipeline_wb.full()) {
        auto exec = m_execute_list.GetOldest();
        exec.wb_begin = m_cycle_count;
        exec.ex_length = m_cycle_count - exec.ex_length;
        if (!exec.valid) {
            break;
        }
        m_pipeline_wb.push(exec);

        // wake up
    }
}


void Simulator::Issue() {
    LOG_STAGE("Issue\n");
    int instructions_issued = 0;
    while (!m_execute_list.full() && instructions_issued < m_width) {
        auto instr = m_iq.GetOldest();
        instr.iq_length = m_cycle_count - instr.iq_begin;
        instr.ex_begin = m_cycle_count;

        if (instr.valid) {
            m_execute_list.push(instr);
            instructions_issued++;
        } else {
            break;
        }

    }
}

void Simulator::Dispatch() {
    LOG_STAGE("Dispatch\n");
    if (m_iq.available() <= m_pipeline_rr.m_element_count) {
        while (!m_pipeline_rr.empty()) {
            auto instr = m_pipeline_di.pop();
            instr.di_length = m_cycle_count - instr.di_begin;
            instr.iq_begin = m_cycle_count;
            m_iq.push(instr); //ready = meta
        }
    }
}


void Simulator::RegRead() {
    LOG_STAGE("RegRead\n");
    if (!m_pipeline_di.full()) {
        while (!m_pipeline_rr.empty()) {
            auto instr = m_pipeline_rr.pop();
            instr.rr_length = m_cycle_count - instr.rr_begin;
            instr.di_begin = m_cycle_count;
            instr.src1_meta = instr.src1_meta ? true : m_rob[instr.src1].ready; // if arf, ready, else, read rob
            instr.src2_meta = instr.src2_meta ? true : m_rob[instr.src2].ready; // if arf, ready, else, read rob
            m_pipeline_di.push(instr);
        }
    }
}


void Simulator::Rename() {
    LOG_STAGE("Rename\n");
    if (DO_CYCLE) {
        printf("m_pipeline_rr.available: %d\n",m_pipeline_rr.available());
        printf("m_rob.available: %d\n",m_rob.available());
        printf("m_pipeline_rn.m_element_count: %d\n",m_pipeline_rn.m_element_count);
    }
    if (m_pipeline_rr.available() >= m_pipeline_rn.m_element_count && m_rob.available() >= m_pipeline_rn.m_element_count) {
        while (!m_pipeline_rn.empty()) {
            auto instr = m_pipeline_rn.pop();
            instr.rn_length = m_cycle_count - instr.rn_begin;
            instr.rr_begin = m_cycle_count;
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

            auto index = m_rob.push({
                instr.dst,
                true,
            false,
            false,
            false,
            instr.pc});
            m_pipeline_rr.push(instr);
            if (instr.dst >= 0) {
                m_rmt[instr.dst] = index;
            }
        }
    }
}


void Simulator::Decode() {
    LOG_STAGE("Decode\n");
    if (m_pipeline_rn.empty()) {
        while (!m_pipeline_de.empty()) {
            auto instr = m_pipeline_de.pop();
            instr.de_length = m_cycle_count - instr.de_begin;
            instr.rn_begin = m_cycle_count;
            m_pipeline_rn.push(instr);
        }

    }
}


void Simulator::Fetch() {
    LOG_STAGE("Fetch\n");
    if (m_pipeline_de.empty() && !feof(m_trace_file)) {
        while (!m_pipeline_de.full() && !feof(m_trace_file)) {
            auto instr = Instruction(m_trace_file);
            instr.fe_begin = m_cycle_count-1;
            instr.fe_length = 1;
            instr.de_begin = m_cycle_count;
            m_pipeline_de.push(instr);
        }

    }
    if (feof(m_trace_file)) {
        fclose(m_trace_file);
        m_done = true;
    }
}


bool Simulator::Advance_Cycle() {
    m_execute_list.Increment();
    m_cycle_count++;
    return !m_done;
}


