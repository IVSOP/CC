#ifndef TIMESTAMPS_COMMON_H
#define TIMESTAMPS_COMMON_H

#include <chrono>

#define NODES_RTT_TRACK_SIZE 16

//represent timestamps in nanoseconds
//sys_nanoseconds represents timepoint -> causes compile time error if used as time duration
//use std::chrono::nanoseconds for adding or subtracting with sys_nanoseconds
// pode-se mudar system_clock para steady_clock se der problemas em Windows (acho??)
template <class Duration>
    using sys_time = std::chrono::time_point<std::chrono::system_clock, Duration>;
    using sys_nanoseconds = sys_time<std::chrono::nanoseconds>;
    using sys_nano_diff = std::chrono::nanoseconds;


    struct NodesRTT {
        sys_nano_diff arr[NODES_RTT_TRACK_SIZE]; //Rtt
        uint32_t curr;
        uint32_t size;

        NodesRTT() {
            curr = 0;
            size = 0;
        }

        void receive(const sys_nanoseconds& timeSent, const sys_nanoseconds& timeReceived){
            sys_nano_diff timeDiff = timeReceived - timeSent;
            this->arr[curr] = timeDiff;
            curr = (curr + 1) % NODES_RTT_TRACK_SIZE;
            if (size < NODES_RTT_TRACK_SIZE) size++; //para saber quais posições têm dados a sério
        }


        double RTT(){
            sys_nano_diff total(0); // se ainda n tiver pacotes vai dizer RTT 0 é suposto???
            for(uint32_t i = 0; i < size; i++){
                total += arr[i];
            }
            if (size > 0) total = total / size;
            //converti em double para manter a estrutura que já estava, se for preciso mudar?
            return std::chrono::duration_cast<std::chrono::duration<double>>(total).count(); 
        }
    };
    
#endif