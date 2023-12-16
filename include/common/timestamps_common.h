#ifndef TIMESTAMPS_COMMON_H
#define TIMESTAMPS_COMMON_H

#include <chrono>
#include <iostream>
#include <iomanip>
#include <cmath>

#define NODES_RTT_TRACK_SIZE 16
#define BASE_TIMEOUT_TIME std::chrono::milliseconds(5); // actual timeout time will be 5* this

//represent timestamps in nanoseconds
//sys_nanoseconds represents timepoint -> causes compile time error if used as time duration
//use std::chrono::nanoseconds for adding or subtracting with sys_nanoseconds
// pode-se mudar system_clock para steady_clock se der problemas em Windows (acho??)
template <class Duration>
    using sys_time = std::chrono::time_point<std::chrono::system_clock, Duration>;
    using sys_nanoseconds = sys_time<std::chrono::nanoseconds>;
    using sys_nano_diff = std::chrono::nanoseconds;
    using sys_milli_diff = std::chrono::milliseconds;

    struct NodesRTT {
        sys_nano_diff arr[NODES_RTT_TRACK_SIZE]; //Rtt
        uint32_t curr;
        uint32_t size;

        NodesRTT() : curr(0), size(0) {}

        void receive(const sys_nanoseconds& timeSent, const sys_nanoseconds& timeReceived){
            sys_nano_diff timeDiff = timeReceived - timeSent;
            this->arr[curr] = timeDiff;
            curr = (curr + 1) % NODES_RTT_TRACK_SIZE;
            if (size < NODES_RTT_TRACK_SIZE) size++; //para saber quais posições têm dados a sério
        }

        void receive2(const sys_nano_diff& timeDiff) {
            this->arr[curr] = timeDiff;
            curr = (curr + 1) % NODES_RTT_TRACK_SIZE;
            if (size < NODES_RTT_TRACK_SIZE) size++;
        }
        
        double RTT(){
            sys_nano_diff total(0); // se ainda n tiver pacotes vai dizer RTT 0
            for(uint32_t i = 0; i < size; i++){
                total += arr[i];
            }
            if (size > 0) total = total / size;
            else total = BASE_TIMEOUT_TIME;
            return std::chrono::duration_cast<std::chrono::duration<double>>(total).count(); 
        }

        //give timeframe, convert into a factor to be used on node priority assignments
        static double convertToPriorityFactor(const sys_nano_diff& timeDiff) {
            double scaleFactor = 1.0;
            double base = 50.0;

            double scaledDuration = std::chrono::duration_cast<std::chrono::duration<double>>(timeDiff).count();
            double scaledValue = scaleFactor / std::log2(scaledDuration * base + 1);

            return scaledValue;
        }

        //receives RTT in seconds
        static std::chrono::milliseconds calcTimeoutTime(double rtt) {
            auto total = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(rtt));

            //operações com o rtt (o que fazer ???)
            //total += std::chrono::seconds(5);
            total *= 5;
            auto rttInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(total);
            std::cout << "Updated Timeout time in milliseconds: " << rttInMilliseconds.count() << " ms" << std::endl;
            return rttInMilliseconds;
        }

        static void printTimePoint(const sys_nanoseconds& timePoint) {
        // Convert time point to std::tm
        auto sys_time_milliseconds = std::chrono::time_point_cast<std::chrono::milliseconds>(timePoint);
        std::time_t t_c = std::chrono::system_clock::to_time_t(sys_time_milliseconds);
        std::tm tm = *std::gmtime(&t_c);

        // Get the fractional seconds
        auto fractional_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(timePoint.time_since_epoch()).count();

        // Print the date with maximum precision
        std::cout << "Date: " << std::put_time(&tm, "%F %T") << "." << std::setprecision(9) 
            << fractional_seconds - static_cast<int>(fractional_seconds) << " UTC" << std::endl;
        }

        static void printTimeDiff(const sys_nano_diff& timeDiff) {
        // Convert nanoseconds to seconds for printing
        auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(timeDiff).count();
        std::cout.precision(15);
        // Print the duration in seconds with maximum precision
        std::cout << "Duration in nanoseconds: " << timeDiff.count() << "ns" << std::endl;
        std::cout << "Duration in seconds: " << seconds << " seconds" << std::endl;
        }
    };
    
#endif