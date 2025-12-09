//
// Created by Aweso on 11/29/2025.
//

#ifndef ECE463_PROJ3_SIMULATOR_H
#define ECE463_PROJ3_SIMULATOR_H
#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <queue>
#define ARCHITECTURAL_REGISTER_COUNT 67


struct ROBEntry {
    int dst;
    bool valid, ready,exec, miss;
    uint64_t pc;

    void Print_Header(FILE *out = stdout) {
        fprintf(out,"dst,valid,ready,exec,miss,pc\n");
    }
    void Print(FILE *out = stdout) {
        fprintf(out,"%d, %d, %d, %d, %d, %x\n",dst,valid,ready,exec,miss,pc);
    }
};


inline uint64_t g_trace_line = 0;
class Instruction{
public:
    uint64_t pc;
    int optype;
    int dst, src1, src2;
    bool src1_meta, src2_meta;
    uint64_t trace_line;
    bool valid;

    uint32_t fe_begin, fe_length;
    uint32_t de_begin, de_length;
    uint32_t rn_begin, rn_length;
    uint32_t rr_begin, rr_length;
    uint32_t di_begin, di_length;
    uint32_t iq_begin, iq_length;
    uint32_t ex_begin, ex_length;
    uint32_t wb_begin, wb_length;
    uint32_t rt_begin, rt_length;



    Instruction(FILE *trace_file) : valid(true),
    fe_begin(0), fe_length(0),
de_begin(0), de_length(0),
rn_begin(0), rn_length(0),
rr_begin(0), rr_length(0),
di_begin(0), di_length(0),
iq_begin(0), iq_length(0),
ex_begin(0), ex_length(0),
wb_begin(0), wb_length(0),
rt_begin(0), rt_length(0)
        {
        fscanf(trace_file, "%lx %d %d %d %d", &pc, &optype, &dst, &src1, &src2);
        trace_line = g_trace_line++;
    }

    Instruction() : valid(false) {
    }

    explicit operator bool() const {
        return optype!=-1; //invalid optype
    }

    int GetLatency() {
        switch (optype) {
            case 0: return 1;
            case 1: return 2;
            case 2: return 5;
            default: {
                printf("ERROR: Invalid optype: %uc\n",optype);
                return -1;
            }
        }
    }

    void Print(FILE *out = stdout) {
        fprintf(out,"%llx,%d,%d,%d,%d,%d,%d,%llu,%d\n",pc,optype,dst,src1,src2,src1_meta,src2_meta,trace_line,valid);
    }

    void Print_Header(FILE *out = stdout) {
        fprintf(out,"pc,optype,dst,src1,src2,src1_meta,src2_meta,timestamp,valid\n");
    }

    void Print_Timing() {
        printf("%llu fu{%d} src{%d,%d} dst{%d} FE{%d,%d} DE{%d,%d} RN{%d,%d} RR{%d,%d} DI{%d,%d} IS{%d,%d} EX{%d,%d} WB{%d,%d} RT{%d,%d}\n",
        trace_line, optype, src1, src2,
        fe_begin, fe_length,
de_begin, de_length,
rn_begin, rn_length,
rr_begin, rr_length,
di_begin, di_length,
iq_begin, iq_length,
ex_begin, ex_length,
wb_begin, wb_length,
rt_begin, rt_length);
    }

};

class Buffer {
    std::queue<Instruction> m_data;
public:

    size_t m_element_count;
    const size_t m_max_element_count;
    Buffer(size_t max_element_count)
        : m_max_element_count(max_element_count),
        m_element_count(0) {};

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (m_max_element_count != other.m_max_element_count) {
                printf("ERROR: Move-assign between Buffers of different max sizes\n");
                return *this;
            }
            m_data = std::move(other.m_data);
            m_element_count = other.m_element_count;
            other.m_element_count = 0;
        }
        return *this;
    }

    [[nodiscard]] bool empty() const {return m_data.empty();}
    [[nodiscard]] bool full() const {return m_element_count == m_max_element_count;}
    [[nodiscard]] int available() const {return m_max_element_count-m_element_count;}
    void push(Instruction entry) {
        if (full()) {
            printf("ERROR: Pushing to full buffer\n");
            return;
        }
        m_element_count++;
        m_data.push(entry);
    }
    Instruction pop() {
        if (empty()) {
            printf("ERROR: Popping from empty buffer\n");
            return m_data.front();
        }
        m_element_count--;
        auto val = m_data.front();
        m_data.pop();
        return val;
    }

    FILE *m_log_file;
    void StartLog(char *path) {
        m_log_file = fopen(path,"w");
        m_data.front().Print_Header(m_log_file);

    }

    void EndLog() {
        fclose(m_log_file);
    }

    void Log() {
        std::queue<Instruction> temp = m_data;
        while (!temp.empty()) {
            temp.front().Print(m_log_file);
            temp.pop();
        }
    }

    void Print(FILE *out = stdout) {
        fprintf(out,"%llu/%llu instructions\n",m_element_count,m_max_element_count);
        m_data.front().Print_Header(out);
        std::queue<Instruction> temp = m_data;
        while (!temp.empty()) {
            temp.front().Print(out);
            temp.pop();
        }
    }


};

template <typename T>
class RingBuffer {
    std::vector<T> m_data;
public:

    size_t m_head,m_tail;
    bool m_full;
    const size_t m_max_element_count;

    RingBuffer(size_t max_element_count)
        : m_max_element_count(max_element_count),
        m_head(0),
        m_tail(0),
        m_full(false){
        m_data.resize(m_max_element_count);
    };

    bool empty() {return m_head == m_tail && !m_full;}
    bool full() {return m_full;}
    int available() {
        auto delta = m_tail - m_head;
        if (delta < 0) return delta  + m_max_element_count;
        return m_max_element_count - delta;
    }
    void push(T entry) {
        if (full()) {
            printf("ERROR: Pushing to full buffer\n");
            return;
        }
        m_data[m_tail] = (entry);
        m_tail = (m_tail + 1)%m_max_element_count;
    }
    int getIndex() {
        return m_tail;
    }

    T& operator[](int index) {
        return m_data[index];
    }

    T pop() {
        if (empty()) {
            printf("ERROR: Popping from empty buffer\n");
            return m_data[0];
        }
        auto val = m_data[m_head];
        m_head = (m_head + 1)%m_max_element_count;
        return val;
    }

    T read(int index) {
        return m_data[index];
    }

};







struct ExecuteEntry {
    int counter;
    Instruction instr;
};


class ExecuteList {
    std::vector<ExecuteEntry> m_instructions;
    size_t m_max_element_count, m_element_count;
public:
    ExecuteList(int size) : m_max_element_count(size), m_element_count(0){
        m_instructions.resize(size);
    }

    void push(Instruction instr) {
        m_element_count++;
        for (auto &i:m_instructions) {
            if (!i.instr.valid) {
                i = {0,instr};
                i.instr.valid = false;
                return;
            }
        }
        printf("ERROR: Failed to insert issue into Execute List\n");
        m_element_count--;
    }

    bool full() {
        return m_max_element_count == m_element_count;
    }

    size_t available() {
        return m_max_element_count - m_element_count;
    }

    void Increment() {
        for (auto &exec : m_instructions) {
            exec.counter++;
            if (exec.counter >= exec.instr.GetLatency()) {
                exec.instr.valid = true;
            }
        }
    }

    Instruction GetOldest() {
        m_element_count--;
        uint64_t oldest_timestamp = UINT64_MAX;
        int oldest_index = 0;
        for (int i = 0; i < m_instructions.size(); i++) {
            if (i==0 || (m_instructions[i].instr.trace_line < oldest_timestamp && m_instructions[i].instr.valid)) {
                oldest_timestamp = m_instructions[i].instr.trace_line;
                oldest_index = i;
            }
        }
        auto val = m_instructions[oldest_index].instr;
        m_instructions[oldest_index].instr.valid = false;
        return val;
    }
};

class IssueQueue {
    std::vector<Instruction> m_instructions;
    size_t m_max_element_count, m_element_count;
public:
    IssueQueue(int iq_size) : m_max_element_count(iq_size), m_element_count(0){
        m_instructions.resize(iq_size);
    }

    void push(Instruction instr) {
        m_element_count++;
        for (auto &i:m_instructions) {
            if (!i.valid) {
                i = instr;
                return;
            }
        }
        printf("ERROR: Failed to insert issue into Issue Queue\n");
        m_element_count--;
    }

    bool full() {
        return m_max_element_count == m_element_count;
    }

    size_t available() {
        return m_max_element_count - m_element_count;
    }

    Instruction GetOldest() {
        m_element_count--;
        uint64_t oldest_timestamp = UINT64_MAX;
        int oldest_index = 0;
        for (int i = 0; i < m_instructions.size(); i++) {
            if (i==0 || (m_instructions[i].trace_line < oldest_timestamp && m_instructions[i].valid)) {
                oldest_timestamp = m_instructions[i].trace_line;
                oldest_index = i;
            }
        }
        auto val = m_instructions[oldest_index];
        m_instructions[oldest_index].valid = false;
        return val;
    }
};



class ReorderBuffer {
    std::vector<ROBEntry> m_rob;
    size_t m_max_element_count, m_element_count;
    size_t m_head,m_tail;
public:
    ReorderBuffer(int size) : m_max_element_count(size), m_element_count(0), m_head(0), m_tail(0){
        m_rob.resize(size);
    }

    ROBEntry& operator[](int index) {
        return m_rob[index];
    }

    size_t push(ROBEntry entry) {
        m_element_count++;
        m_rob[m_tail] = entry;
        m_tail++;
        return m_tail-1;
    }

    ROBEntry retire() {
        if (m_rob[m_head].ready == 1) {
            auto val = m_rob[m_head];
            m_rob[m_head].valid = 0;
            m_head++;
            return val;
        }else {
            return{0,false,false,false,false,0};
        }
    }

    bool full() {
        return m_max_element_count == m_element_count;
    }

    size_t available() {
        return m_max_element_count - m_element_count;
    }

    void Print(FILE *out = stdout) {
        m_rob[0].Print_Header(out);
        for (int i = m_head; i!=m_tail;i = (i + 1)%m_max_element_count) {
            m_rob[i].Print(out);
        }
    }


};

class Simulator {
    uint32_t m_rob_size, m_iq_size, m_width;
    FILE *m_trace_file;
    uint64_t m_cycle_count;

    ReorderBuffer m_rob;
    IssueQueue m_iq;

    bool m_done;

    ExecuteList m_execute_list;
    Buffer m_pipeline_de,m_pipeline_rn,m_pipeline_rr, m_pipeline_di, m_pipeline_wb, m_pipeline_rt;
    std::array<int,ARCHITECTURAL_REGISTER_COUNT> m_rmt, m_arf;
public:
    Simulator(int rob_size, int iq_size, int width, char* tracefile)
        :   m_rob_size(rob_size),
            m_iq_size(iq_size),
            m_width(width),
            m_pipeline_de(width),
            m_pipeline_rn(width),
            m_rob(rob_size),
            m_pipeline_rr(width),
            m_pipeline_di(width),
            m_iq(iq_size),
            m_execute_list(width * 5),
            m_pipeline_wb(width),
            m_done(false),
            m_cycle_count(0),
    m_pipeline_rt(width){
        m_trace_file = fopen(tracefile, "r");
        if (!m_trace_file) {
            printf("ERROR: Failed to open tracefile\n");
        }
        for (auto &r : m_rmt) r = -1; //invalidate rmt
    }


    void Run();

private:

    void Retire();
    void Writeback();
    void Execute();
    void Issue();
    void Dispatch();
    void RegRead();
    void Rename();
    void Decode();
    void Fetch();
    bool Advance_Cycle();

};


#endif //ECE463_PROJ3_SIMULATOR_H